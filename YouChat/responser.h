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

using namespace std;

class Responser
{
private:
    vector<int> clnt_socks;
    unordered_map<int, string> id_name;

    void AskFileDownload();

    int file_src;
    int file_dst;
    string file_name;
    int64_t file_size = 0;
    int64_t offset = 0;

    bool fileTranferMode = false;
    void ReceiveFile(ssize_t bytesRead, char *buf);

    void SendFileThread(int source_sock, string file_name);

    void SendJSON(int socket, Json::Value response);

public:
    void deleteClient(int c);
    void addClient(int c);
    void process(int client, int len, char *buf);

    Responser(/* args */);
    ~Responser();
};
