#include "../include/myhead.h"

using namespace std;
using json = nlohmann::json;

/*
    视频处理模块
*/

// 编写hls的index.m3u8文件---等待编写
void write_m3u8(const std::string &directoryPath)
{
    printf("子进程%d write_m3u8：编写index.m3u8文件，文件保存路径 %s\n", getpid(), directoryPath.c_str());
}

// 创建文件夹
bool createDirRecs(const std::string &path)
{
    // 如果路径为空或已经存在，返回 true
    // 0644:对于所有者具有读和写权限，对于组和其他用户，只有读权限
    int ret = mkdir(path.c_str(), 0755);
    if (ret == 0)
    {
        // 如果创建成功或目录已经存在，返回 true
        return true;
    }
    else
    {
        // 已存在
        if (errno == EEXIST)
        {
            return true;
        }
        // 创建失败
        return false;
    }
}

// 去除文件后缀
std::string removeFileExtension(const std::string &fileName)
{
    size_t lastDotPos = fileName.rfind('.');
    if (lastDotPos != std::string::npos)
    {
        return fileName.substr(0, lastDotPos);
    }
    else
    {
        return fileName; // 如果没有找到点，返回原始文件名
    }
}

// 函数用于获取目录下所有的.ts文件
std::vector<std::string> getTsFiles(const std::string &dirPath)
{
    std::vector<std::string> tsFiles;
    DIR *dp = opendir(dirPath.c_str()); // 打开目录

    if (dp)
    {
        struct dirent *entry;

        // 遍历目录中的文件和子目录
        while ((entry = readdir(dp)))
        {
            std::string filename = entry->d_name;

            // 检查文件扩展名是否是 .ts
            if (filename.length() >= 3 && filename.substr(filename.length() - 3) == ".ts")
            {
                tsFiles.push_back(filename);
            }
        }

        closedir(dp); // 关闭目录
    }

    return tsFiles;
}
// curl回调函数，用于处理接收到的响应数据
size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append(static_cast<char *>(contents), totalSize);
    return totalSize;
}

int curl_request_file(CURL *curl, const std::string &input_dir, std::string &response)
{
    // 设置要上传的文件
    const char *file_path = input_dir.c_str();
    // 创建一个文件读取句柄
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        curl_easy_cleanup(curl);
        return 1;
    }
    curl_easy_setopt(curl, CURLOPT_READDATA, file);
    // 执行HTTP请求
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return -1;
    }
    return 0;
}

// 处理视频并添加目标框
int processVideoBox(const string &inputVideoPath, const string &outputVideoPath, const json &targetData)
{
    cv::VideoCapture videoCapture(inputVideoPath);
    if (!videoCapture.isOpened())
    {
        cerr << "Error: Failed to open input video." << endl;
        return -1;
    }

    double fps = videoCapture.get(CV_CAP_PROP_FPS);
    int frameWidth = static_cast<int>(videoCapture.get(CV_CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT));

    cv::VideoWriter videoWriter(outputVideoPath, CV_FOURCC('X', 'V', 'I', 'D'), fps, cv::Size(frameWidth, frameHeight));

    if (!videoWriter.isOpened())
    {
        cerr << "Error: Failed to open output video for writing." << endl;
        return -1;
    }

    json::const_iterator emotionData = targetData.begin();
    cv::Mat frame;

    int index = 0;
    while (true)
    {
        videoCapture >> frame;
        if (frame.empty())
        {
            break;
        }
        const string &emotion = (*emotionData)[0].get<string>();
        const json &boundingBoxes = (*emotionData)[1];
        // const auto &boundingBox = boundingBoxes[index];
        const auto &boundingBox = boundingBoxes.at(index);

        // 如果已经处理完所有数据，不画框但写入
        if (index >= boundingBoxes.size())
        {
            videoWriter.write(frame);
            continue;
        }

        vector<int> bbox = boundingBox.get<vector<int>>();
        int left = bbox[0];
        int right = bbox[1];
        int top = bbox[2];
        int bottom = bbox[3];

        cv::rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), cv::Scalar(0, 255, 0), 2);
        cv::putText(frame, emotion, cv::Point(left + 120, top - 10), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);

        index++;

        videoWriter.write(frame);
    }

    videoCapture.release();
    videoWriter.release();

    return 0;
}

// 完全拷贝
int cp_file(const std::string& old_file, const std::string& new_file) {
    std::ifstream source(old_file, std::ios::binary);
    if (!source) {
        std::cerr << "无法打开源文件：" << old_file << std::endl;
        return -1;
    }

    std::ofstream destination(new_file, std::ios::binary);
    if (!destination) {
        std::cerr << "无法创建目标文件：" << new_file << std::endl;
        return -1;
    }

    destination << source.rdbuf();

    if (source.fail() || destination.fail()) {
        std::cerr << "文件拷贝失败" << std::endl;
        return -1;
    }

    source.close();
    destination.close();

    std::cout << "文件拷贝成功" << std::endl;
    return 0;
}

// 发送API+合成视频---等待编写
int curl_API_all(const std::string &input_dir, const std::string &output_dir)
{
    printf("子进程%d curl_API_all:发送API与合成视频\n", getpid());
    printf("子进程%d curl_API_all:input_dir:%s\n", getpid(), input_dir.c_str());
    printf("子进程%d curl_API_all:output_dir:%s\n", getpid(), output_dir.c_str());

    // 创建合成视频文件夹
    if (createDirRecs(output_dir))
    {
        std::cout << "子进程" << getpid() << " 存放合成视频文件夹创建成功或已存在:" << output_dir.c_str() << std::endl;
    }
    else
    {
        std::cerr << "子进程" << getpid() << " 创建合成视频文件夹失败" << std::endl;
        return -1;
    }

    // 获取源文件夹下所有ts文件组成一个列表
    std::vector<std::string> tsFiles = getTsFiles(input_dir);

    // 打印所有找到的.ts文件
    std::cout << "子进程" << getpid() << " 生成的ts数量：" << tsFiles.size() << std::endl;

    /*
        API
    */
    // 初始化libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT); // 启用线程安全（支持多线程环境）和设置一些库的全局状态
    CURL *curl = curl_easy_init();

    if (!curl)
    {
        std::cout << "子进程" << getpid() << " 初始化libcurl失败" << std::endl;
        curl_global_cleanup();
        return -1;
    }
    // 设置要访问的URL
    // const char *url = "http://192.168.12.41:30088/predictions/IDB-emo-video";
    const char *url = "http://192.168.3.108:30088/predictions/IDB-emo-video";

    // 获取的响应
    std::string response;

    // 设置libcurl选项
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // 设置写回调函数，用于接收响应数据
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    std::string old_ts_file;
    std::string new_ts_file;

    // 循环作为参数发送
    for (const std::string &tsFile : tsFiles)
    {
        old_ts_file = input_dir + tsFile;
        new_ts_file = output_dir + tsFile;

        int ret = curl_request_file(curl, old_ts_file, response);

        if (ret == 0)
        {
            std::cout << "子进程" << getpid() << " " << tsFile << " 请求成功" << std::endl;
        }
        else
        {
            std::cout << "子进程" << getpid() << " " << tsFile << " 请求失败" << std::endl;
        }

        // 解析 JSON 字符串
        json data = json::parse(response);

        // 打印解析结果
        // std::cout << "Emotion: " << data[0][0].get<std::string>() << std::endl;
        // std::cout << "Coordinates:" << std::endl;
        // for (const auto &item : data[0][1])
        // {
        //     std::cout << "  [" << item[0] << ", " << item[1] << ", " << item[2] << ", " << item[3] << "]" << std::endl;
        // }

        // 生成视频
        try
        {
            int result = processVideoBox(old_ts_file, new_ts_file, data);
            if (result == 0)
            {
                std::cout << "子进程" << getpid() << " " << tsFile << " 合成视频成功 "
                          << "新视频路径：" << new_ts_file.c_str() << std::endl;
            }
            else
            {
                std::cout << "子进程" << getpid() << " " << tsFile << " 合成视频失败" << std::endl;
            }

            // 清空响应数据以准备下一次请求
            response.clear();
        }
        catch (const std::exception &e)
        {
            int ret = cp_file(old_ts_file, new_ts_file);//完全拷贝
            std::cout << "子进程" << getpid() << " " << tsFile << " 合成视频失败,完全拷贝成功" << std::endl;

        }

        // 先只执行一个
        // break;
    }

    // 清理和关闭 CURL
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}

/**
 * 当前模块主函数
 */
int handle_video(char *strFileName, char *file_m3u8_path)
{
    printf("系统：开始处理视频\n");

    // if (strFileName == NULL || file_m3u8_path == NULL || strlen(strFileName) == 0 || strlen(file_m3u8_path) == 0)
    if (strFileName == NULL || strlen(strFileName) == 0)
    {
        // 一个或多个参数不能为空 或 空字符串
        printf("系统：视频处理模块：无效参数\n");
        return -1;
    }

    // 文件的绝对路径
    std::string data_path = DATA_PATH;
    std::string fileName(strFileName);
    std::string input_file_path = data_path + fileName; // 输入文件路径
    std::string new_dir = data_path + removeFileExtension(fileName) + "/";
    std::string output_file_path = new_dir + "index.m3u8"; // 输出文件路径

    // 创建新文件夹
    if (createDirRecs(new_dir))
    {
        std::cout << "系统：存放m3u8文件夹创建成功或已存在:" << new_dir.c_str() << std::endl;
    }
    else
    {
        std::cerr << "系统：创建文件夹失败" << std::endl;
        return -1;
    }

    printf("系统：待处理视频路径 %s\n", input_file_path.c_str());
    printf("系统：输出原视频m3u8文件路径 %s\n", output_file_path.c_str());

    // return 0;

    // 创建子线程，用execlp()调用ffmpeg处理视频
    pid_t child_pid = fork();
    if (child_pid == 0)
    {
        // 子进程
        printf("子进程%d：开始视频转HLS流文件...\n", getpid());

        // 获取程序开始执行的时间点
        auto start_time = std::chrono::high_resolution_clock::now();

        // 需要linux的FFmpeg环境
        // execlp("ffmpeg", "ffmpeg", "-i", input_file_path.c_str(),
        //        "-force_key_frames", "expr:gte(t,n_forced*1)", "-strict", "-1",
        //        "-c:a", "aac", "-c:v", "libx264", "-hls_time", "1",
        //        "-f", "hls", output_file_path.c_str(), NULL);
        sleep(3);

        printf("子进程%d：转化完成\n", getpid());

        // 生成index.m3u8文件
        write_m3u8(output_file_path);

        // 使用循环发送API获取识别结果
        std::string output_dir = DATA_PATH; // 合成视频的结果目录
        output_dir.append("output_video/");
        createDirRecs(output_dir);
        output_dir += removeFileExtension(fileName) + "/";

        // API
        curl_API_all(new_dir, output_dir);

        printf("子进程%d 输出识别后视频m3u8文件路径 %s\n", getpid(), output_dir.c_str());
        strcpy(file_m3u8_path, output_dir.c_str());

        // 子进程执行完命令后，退出，并发送给父进程回收信号
        printf("子进程%d 已退出，发送回收信号\n", getpid());
        kill(getppid(), SIGUSR1);

        // 获取程序执行完毕的时间点
        auto end_time = std::chrono::high_resolution_clock::now();

        // 计算时间差
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

        // 输出程序执行时间（以秒为单位）
        std::cout << "子进程 " << getpid() << " 程序执行时间: " << duration.count() << " 秒" << std::endl;

        exit(0);
    }

    return 0;
}