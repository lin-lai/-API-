#include "./include/myhead.h"

/**
 * 设置回收信号处理函数
 * 进程间信号触发设置——异步通信
 */
void signal_handler(int signal_num)
{
    if (signal_num == SIGUSR1)
    {
        printf("系统：检测到回收信号，开始回收子进程...\n");
        int status;
        // 等待与当前进程组ID相同的任何子进程退出并回收资源
        // int ret = waitpid(0, &status, WNOHANG); // WNOHANG:非阻塞形式
        int ret = waitpid(0, &status, 0); // 0:阻塞形式
        if (ret > 0)
        {
            printf("系统：回收子进程，进程ID为 %d，退出状态为 %d，已回收.\n", ret, WEXITSTATUS(status));
        }
        else if (0 == ret)
        {
            // 子进程状态发生改变，但未退出
            printf("系统：没有子进程退出\n");
        }
        else
        {
            perror("系统：waitpid发送错误");
        }
    }
}

/*
    服务器入口
*/
int main(int argc, char const *argv[])
{
    /*
        IO多路复用并发服务器
    */
    int ret;
    char buf[BUF_SIZE] = {0};

    // 注册回收信号并绑定回调函数
    signal(SIGUSR1, signal_handler); // 用于子传父

    // 1 socket 创建流式套接字
    int fd_Server = socket(AF_INET, SOCK_STREAM, 0); // 返回服务端文件描述符
    if (-1 == fd_Server)
    {
        perror("socket error!\r\n");
        return -1;
    }

    // 2 bind 绑定协议端口IP
    struct sockaddr_in stServer; // 网络配置结构体
    memset(&stServer, 0, sizeof(stServer));

    stServer.sin_family = AF_INET;   // 指定要使用 IPv4 地址族
    stServer.sin_port = htons(8888); // 指定端口
    // stServer.sin_addr.s_addr = inet_addr("192.168.12.111"); // 绑定ip
    // stServer.sin_addr.s_addr = inet_addr("192.168.10.61"); // 绑定ip
    stServer.sin_addr.s_addr = inet_addr("0.0.0.0"); // 绑定ip

    ret = bind(fd_Server, (const struct sockaddr *)&stServer, sizeof(const struct sockaddr));
    if (-1 == ret)
    {
        perror("bind error!\r\n");
        return -1;
    }

    // 3 listen
    ret = listen(fd_Server, 10);
    if (-1 == ret)
    {
        perror("listen error!\r\n");
        return -1;
    }

    printf("系统：服务器启动\n");

    struct sockaddr_in stClient; // 用于存放客户端的容器
    socklen_t len = sizeof(struct sockaddr_in);
    /**
     * 使用epoll检测请求
     */
    // 创建epoll实例
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
    {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fd_Server;

    // 将监听套接字添加到epoll实例中
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_Server, &event) == -1)
    {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];

    int max = fd_Server; // 设置当前最大检测文件符--动态更改

    // 开始检测
    while (1)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1)
        {
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < num_events; i++)
        {
            int fd = events[i].data.fd;

            // 服务器IO活跃，表示新客户端连接,接入连接，加入检测表,更新max
            if (fd == fd_Server)
            {
                memset(&stClient, 0, sizeof(struct sockaddr_in));
                int fd_newCli = accept(fd_Server, (struct sockaddr *)&stClient, &len);
                if (-1 == fd_newCli)
                {
                    continue;
                }
                // 加入检测表
                event.events = EPOLLIN;
                event.data.fd = fd_newCli;

                // 将新连接的套接字添加到epoll实例中
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_newCli, &event) == -1)
                {
                    perror("epoll_ctl");
                    continue;
                }

                printf("系统：新客户端连接 分配fd号:%d\n", fd_newCli);
            }
            else
            {
                /*
                    bug1:
                        非阻塞IO,第二次接收到的是传输内容,没有命令,会当成传输命令处理
                        开始和结束标志位都由发送文件方发出,不需要等待,因为是TCP连接,没有丢包,只需要发数据和拿数据
                        原因是由于服务器接受错误退出,但客户端还在上传引发的问题
                        解决办法：直接关闭
                    bug2:
                        出现粘包---2个小数据包被合并成一个数据包
                        解决方法---消息前缀
                */
                memset(buf, 0, BUF_SIZE);

                // 客户端IO活跃，接入连接，检测指令,阻塞接收

                // 消息前缀解决粘包
                int message_length = 0;
                int bytes_received = recv(fd, &message_length, sizeof(int), 0);
                printf("客户端%d 获取指令的消息前缀 %d\n", fd, message_length);
                if (bytes_received != sizeof(int))
                {
                    printf("客户端%d 非法指令,断开连接\n", fd);
                    // 关闭文件描述符并停止检测该IO
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

                    printf("客户端%d 关闭 fd=%d\n\n", fd, fd);
                    continue;
                }

                ret = recv(fd, buf, message_length, 0);
                if (ret > 0)
                {
                    printf("客户端%d 指令:%s\n", fd, buf);
                    // 解析指令
                    if (0 == strncmp(buf, "put", 3))
                    {
                        // 上传:put test.txt
                        ret = UpLoadFile(fd, buf + 4);
                        if (0 == ret)
                        {
                            if ((strstr(buf, ".mp4") != NULL) || (strstr(buf, ".avi") != NULL) || (strstr(buf, ".ts") != NULL))
                            {
                                // 处理视频,返回处理后的m3u8文件名
                                printf("系统：检测到视频文件 进入API分析模块\n");
                                char file_m3u8_path[100] = {0};
                                ret = handle_video(buf + 4, file_m3u8_path);
                            }
                            printf("客户端%d 上传成功\n", fd);
                            // 关闭文件描述符并停止检测该IO
                            close(fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                            printf("客户端%d 关闭 fd=%d\n\n", fd, fd);
                            continue;
                        }
                        else
                        {
                            printf("客户端%d 上传失败\n", fd);

                            // 关闭文件描述符并停止检测该IO
                            close(fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                            printf("客户端%d 关闭 fd=%d\n\n", fd, fd);
                            continue;
                        }
                    }
                    else if (0 == strncmp(buf, "get", 3))
                    {
                        // 下载:get test.txt
                        ret = DownLoadFile(fd, buf + 4);
                        if (ret < 0)
                        {
                            printf("客户端%d 下载失败\n", fd);

                            // 关闭文件描述符并停止检测该IO
                            close(fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

                            printf("客户端%d 关闭 fd=%d\n\n", fd, fd);
                            continue;
                        }
                        else
                        {
                            printf("客户端%d 下载成功\n", fd);
                            // 关闭文件描述符并停止检测该IO
                            close(fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

                            printf("客户端%d 关闭 fd=%d\n\n", fd, fd);
                            continue;
                        }
                    }
                    else
                    {
                        printf("客户端%d 非法指令,断开连接\n", fd);
                        // 关闭文件描述符并停止检测该IO
                        close(fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

                        printf("客户端%d 关闭 fd=%d\n\n", fd, fd);
                        continue;
                    }
                }
                else
                {
                    // 关闭文件描述符并停止检测该IO
                    close(fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);

                    printf("系统：关闭 fd=%d\n\n", fd);
                    continue;
                }
            }
        }
    }

    return 0;
}