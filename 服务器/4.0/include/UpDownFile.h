#ifndef _UPDOWNFILE_H_
#define _UPDOWNFILE_H_

/*
    上传下载模块
*/


extern int UpLoadFile(int iClient, char *strFileName);
extern int DownLoadFile(int iClient, char *strFileName);


#endif