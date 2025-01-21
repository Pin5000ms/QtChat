
// C++ 標準函式庫
#include <vector>        // 用於儲存客戶端 socket (clnt_socks)
#include <queue>         // 用於檔案傳輸佇列
#include <unordered_map> // 用於儲存客戶端 ID 和名稱的對應 (id_name)
#include <iostream>      // 用於輸出偵錯資訊 (cout, cerr)
#include <fstream>       // 用於檔案讀寫操作 (receiveFile, sendFileThread)
#include <memory>        // 用於智慧型指標 (unique_ptr)
#include <thread>        // 用於建立檔案傳輸執行緒 (sendFileThread)
#include <algorithm>     // 用於尋找和刪除客戶端 (deleteClient)

// C 標準函式庫（使用 C++ 版本）
#include <cstdio>  // 用來列印偵錯資訊 (printf)
#include <cstring> // 用於記憶體操作 (memset)

// 系統相關
#include <sys/epoll.h>  // 用於 epoll 事件處理
#include <sys/socket.h> // 用於 socket 通信
#include <netinet/in.h> // 用於網路位址結構
#include <arpa/inet.h>  // 用於位址轉換
#include <unistd.h>     // 用於檔案描述符操作 (close, read, write)
#include <fcntl.h>      // 用於檔案控制 (非阻塞模式設定)
#include <errno.h>      // 用於錯誤處理

// 第三方函式庫
#include <jsoncpp/json/json.h> // 用於 JSON 資料處理 (process)

using namespace std;

class Responser
{
private:
    vector<int> clnt_socks;
    unordered_map<int, string> id_name;

    int file_src;
    int file_dst;
    string file_name;
    int64_t file_size = 0;
    int64_t offset = 0;

    bool fileTranferMode = false;
    void receiveFile(ssize_t bytesRead, char *buf);

    void sendFileThread(int source_sock, string file_name);

    void notifyFileDownload();

    void sendJSON(int socket, Json::Value response);

public:
    void deleteClient(int c);
    void addClient(int c);
    void process(int client, int len, char *buf);

    Responser(/* args */);
    ~Responser();
};