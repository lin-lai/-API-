#ifndef _MYHEAD_H_
#define _MYHEAD_H_

// 错误码
enum ErrorCode
{
    ERROR = -1,
    OK,

};


#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/wait.h>
#include <fstream>
#include <vector>
#include <chrono>
#include <dirent.h>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

#include <sys/epoll.h>

#ifdef __cplusplus
extern "C"
{
    #include <curl/curl.h>
}
#endif


#include "UpDownFile.h"
#include "VideoHandle.h"

// #define BUF_SIZE_VIDEO (1024 * 20)
#define BUF_SIZE_VIDEO (1024)
#define BUF_SIZE (500)

//同时检测客户端数量
#define MAX_EVENTS 4090


// 统一使用末尾加/,前面不加/
#define DATA_PATH ("data/")

#endif
