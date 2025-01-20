#include <stdio.h> // printf()、perror()
#include <vector>
#include <unordered_map>
#include <iostream>
#include <stdlib.h>
#include <jsoncpp/json/json.h>

#include <algorithm>
#include <sys/epoll.h>
#include <cstdio>
#include <ctype.h>

#include <sys/socket.h> // 基本 socket 函數和結構
#include <netinet/in.h> // 網路地址結構
#include <arpa/inet.h>  // 地址轉換函數
#include <unistd.h>     // close() 函數
#include <string.h>     // memset()

#include <errno.h> // errno 和錯誤代碼
#include <wait.h>
#include <fcntl.h>

#include <fstream>

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

class Responser
{
private:
    vector<int> clnt_socks;
    unordered_map<int, string> id_name;

    // 用來儲存檔案區塊
    std::queue<std::pair<int, std::string>> fileChunksQueue; // pair: 客戶端socket & 檔案區塊
    std::mutex queueMutex;
    std::condition_variable queueCondVar;
    void enqueueFileChunk(int clientSock, const std::string &chunk);
    void processFileChunks();
    void sendFileChunkToClient(int target_sock, const std::string file_name, const std::string &chunk);

    int receive_file_src;
    string receive_file_name;
    int64_t file_size = 0;
    int64_t offset = 0;

public:
    bool filetranfermode = false;
    void deleteClient(int c);
    void addClient(int c);

    void process(int client, char *buf);
    void receiveFile(ssize_t bytesRead, char *buf);

    Responser(/* args */);
    ~Responser();
};
