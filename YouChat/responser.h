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

using namespace std;

class Responser
{
private:
    vector<int> clnt_socks;
    unordered_map<int, string> id_name;

public:
    void deleteClient(int c);
    void addClient(int c);

    void process(int client, char *buf);
    Responser(/* args */);
    ~Responser();
};


