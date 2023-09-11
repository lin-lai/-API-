#include "../include/myhead.h"

/**
 * 上传文件 服务器
 *      //开始写入 服务器检测STF标志位
 *      //结束写入 服务器检测EOF标志位
 *      直接写入
 *
 *
 * 解决粘包
 *  粘包
 *      发生端发送2个小数据包,接收端会一次性接收到这两个字符串，并将它们合并成一个较大的数据包
 *  解决方法
 *      消息前缀是一个4字节的整数，用于表示消息的长度
 *      每次取数据步骤
 *          1.先取4字节的消息前缀
 *          2.按照消息前缀去取数据,根据长度来准确切割消息
 *
 */
int UpLoadFile(int clientSocket, char *strFileName)
{
    printf("客户端%d 上传任务\n", clientSocket);
    // 参数判断
    if (clientSocket < 0 || NULL == strFileName || strlen(strFileName) <= 0)
    {
        return -1;
    }

    // 从传输开始标志位STF起写入文件
    // 连接-绝对路径
    char str_file[512];
    const char *base_path = DATA_PATH;
    strcpy(str_file, base_path);
    strcat(str_file, strFileName);
    printf("客户端%d 服务器保存位置：%s\n", clientSocket, str_file);

    // 打开本地文件以保存下载内容
    FILE *file = fopen(str_file, "wb");
    if (!file)
    {
        printf("客户端%d ", clientSocket);
        perror("打开本地文件失败");
        return -1;
    }

    // recv and fwrite
    int iRet = -1;
    char *buf = new char[BUF_SIZE_VIDEO]();
    memset(buf, 0, BUF_SIZE_VIDEO);
    while ((iRet = recv(clientSocket, buf, BUF_SIZE_VIDEO, 0)) > 0)
    {
        fwrite(buf, 1, iRet, file);
        memset(buf, 0, iRet);
    }


    // 检测
    if (iRet==0)
    {
        printf("客户端%d 文件上传成功\n", clientSocket);
    }
    else
    {
        printf("客户端%d 接收数据失败\n",clientSocket);
        fclose(file);
        delete[] buf;
        return -1;
    }

    // 关闭资源
    fclose(file);
    delete[] buf;
    return 0;
}

/**
 * 下载文件 服务器
 *      开始传输 服务器发送STF标志位
 *      结束传输 服务器发送EOF标志位
 */
int DownLoadFile(int clientSocket, char *strFileName)
{
    const char *stfFlag = "STF"; // 定义文件开始传输标志位
    const char *eofFlag = "EOF"; // 定义文件结束传输标志位

    printf("客户端%d 下载任务\n", clientSocket);
    if (clientSocket < 0 || NULL == strFileName || strlen(strFileName) <= 0)
    {
        perror("非法参数\n");
        return -1;
    }
    // send file size to client

    // 查找并删除末尾的换行符--客户端处理
    size_t len = strlen(strFileName);
    if (len > 0 && strFileName[len - 1] == '\n')
    {
        strFileName[len - 1] = '\0'; // 将换行符替换为字符串终止符
    }

    // 绝对路径
    char str_file[512];
    const char *base_path = DATA_PATH;
    strcpy(str_file, base_path);
    strcat(str_file, strFileName);

    struct stat stBuf;
    memset(&stBuf, 0, sizeof(struct stat));
    if (-1 == stat(str_file, &stBuf))
    {
        perror("错误");
        printf("客户端%d 数据检测失败:%s\n", clientSocket, str_file);
        return -1;
    }

    char *buf = new char[BUF_SIZE_VIDEO]();
    memset(buf, 0, BUF_SIZE_VIDEO);

    // 发送一个传输开始标志位STF
    if (send(clientSocket, stfFlag, strlen(stfFlag), 0) == -1)
    {
        perror("发送 STF 标志位失败");
        // 处理发送失败的情况——断开连接
        delete[] buf;
        return -1;
    }
    printf("客户端%d 发送传输标志位 %s\n", clientSocket, stfFlag);

    // 打开文件
    FILE *file = fopen(str_file, "rb");
    if (!file)
    {
        printf("客户端%d 打开 %s 文件失败\n", clientSocket, str_file);
        fclose(file);
        delete[] buf;
        return -1;
    }
    // read and send
    int iRet = -1;
    printf("客户端%d 开始发送...\n", clientSocket);

    while ((iRet = fread(buf, 1, BUF_SIZE_VIDEO, file)) > 0)
    {
        // send
        if (-1 == send(clientSocket, buf, iRet, 0))
        {
            perror("Error sending data");
            break;
        }
        // printf("客户端%d:发送数据 %s\n", clientSocket, buf);
        memset(buf, 0, BUF_SIZE_VIDEO);
    }
    printf("客户端%d 发送成功\n", clientSocket);

    // 发送一个传输结束标志位EOF
    if (send(clientSocket, eofFlag, strlen(eofFlag), 0) == -1)
    {
        perror("发送 EOF 标志位失败");
        // 处理发送失败的情况——断开连接
        fclose(file);
        delete[] buf;
        return -1;
    }

    fclose(file);
    delete[] buf;
    return 0;
}
