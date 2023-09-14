#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

const char *stfFlag = "STF"; // 定义文件开始传输标志位

char buffer[BUF_SIZE];

/**
 * 下载文件 客户端
 *      开始传输 客户端接受STF标志位
 *      结束传输 客户端接受EOF标志位
 */

int Downfile(int clientSocket)
{
    // 发送下载指令和文件名
    char command[BUF_SIZE];
    printf("请输入要下载的文件名: ");
    fgets(command, sizeof(command), stdin);
    command[strcspn(command, "\n")] = '\0'; // 去掉末尾的换行符

    char fullCommand[BUF_SIZE];
    snprintf(fullCommand, sizeof(fullCommand), "get %s", command); // 添加 "get"
    // 使用消息前缀解决粘包问题
    int message_length = strlen(fullCommand); // 计算消息的长度
    char send_buffer[BUF_SIZE];
    memcpy(send_buffer, &message_length, sizeof(int));              // 将消息长度前缀复制到发送缓冲区
    memcpy(send_buffer + sizeof(int), fullCommand, message_length); // 将消息内容复制到发送缓冲区

    if (send(clientSocket, send_buffer, sizeof(int) + message_length, 0) == -1)
    {
        perror("发送指令失败");
        return -1;
    }

    /*
        接受一个传输开始标志位STF
    */
    if (recv(clientSocket, buffer, 3, 0) == -1)
    {
        perror("服务器未响应");
        return -1;
    }
    else
    {
        printf("接收到标志位：%s\n", buffer);
        if (0 == strncmp(buffer, stfFlag, 3)) // 检查是否接收到文件传输完成标志位
        {
            printf("服务器成功响应\n");
        }
        else
        {
            printf("无效响应\n");
            return -1;
        }
    }

    // 打开本地文件以保存下载内容
    FILE *file = fopen(command, "wb");
    if (!file)
    {
        perror("打开本地文件失败");
        return -1;
    }

    // 接收服务器发送的文件数据并保存到本地文件
    char buffer[BUF_SIZE];
    ssize_t bytesRead;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        printf("%s\n", buffer);
        fwrite(buffer, 1, bytesRead, file);
        memset(buffer, 0, BUF_SIZE);

    }

    if (bytesRead == -1)
    {
        perror("接收数据失败");
        fclose(file);
        return -1;
    }
    else
    {
        printf("文件下载成功\n");

    }

    fclose(file);

    return 0;
}

char IP[] = "0.0.0.0";
int PORT = 8888;

int main()
{
    int clientSocket;
    int ret;
    struct sockaddr_in serverAddr;

    // 创建套接字
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("套接字创建失败");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT); // 服务器端口
    // serverAddr.sin_addr.s_addr = inet_addr("192.168.12.111"); // 服务器IP地址
    serverAddr.sin_addr.s_addr = inet_addr(IP); // 服务器IP地址

    // 连接到服务器
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("连接服务器失败");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    printf("已连接到服务器\n");

    // 下载
    ret = Downfile(clientSocket);


    // 关闭套接字
    close(clientSocket);

    return 0;
}
