#include "responser.h"

void client101()
{
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_adr;
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_adr.sin_port = htons(9527);

    if (connect(sock, (sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        printf("connect error %d %s", errno, strerror(errno));
        close(sock);
        return;
    }
    char message[256] = "";
    while (true)
    {
        printf("Input message(q to quit):");
        fgets(message, sizeof(message), stdin);
        if (!strcmp(message, "q\n") || !strcmp(message, "q\n"))
            break;
        write(sock, message, strlen(message));
        memset(message, 0, strlen(message));
        read(sock, message, sizeof(message));
        printf("server:%s\n", message);
    }
    close(sock);
}

void server103()
{
    Responser *rs = new Responser();
    // 宣告伺服器和客戶端的socket描述符
    int serv_sock, clnt_sock;
    // 宣告伺服器和客戶端的地址結構
    sockaddr_in serv_adr, clnt_adr;
    // 客戶端地址結構的大小
    socklen_t clnt_sz;
    // 用於儲存接收到的數據的緩衝區
    char buf[1024 * 1024] = "";
    // 創建TCP socket
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    // 設置 SO_REUSEADDR 選項
    int opt = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 設置伺服器地址結構
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); // 接受任意IP的連接
    serv_adr.sin_port = htons(9527);              // 設置端口號

    // 將socket綁定到指定的IP和端口
    if (bind(serv_sock, (sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        printf("bind error %d %s", errno, strerror(errno));
        close(serv_sock);
        return;
    }

    // 開始監聽連接請求，最大等待隊列長度為5
    if (listen(serv_sock, 5) == -1)
    {
        printf("listen error %d %s", errno, strerror(errno));
        close(serv_sock);
        return;
    }

    // 創建epoll實例
    epoll_event event;
    int epfd, event_cnt;
    epfd = epoll_create(1);
    if (epfd == -1)
    {
        printf("epoll_create error %d %s", errno, strerror(errno));
        close(serv_sock);
        return;
    }
    // 創建事件數組，用於存儲epoll事件
    epoll_event *all_events = new epoll_event[100];

    // 將伺服器socket添加到epoll中監聽
    event.events = EPOLLIN; // 監聽輸入事件
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event);

    while (true)
    {
        // 等待事件發生，超時時間為1秒
        event_cnt = epoll_wait(epfd, all_events, 100, 1000);
        if (event_cnt == -1)
        {
            printf("epoll_wait error %d %s", errno, strerror(errno));
            break;
        }
        if (event_cnt == 0) // 如果沒有事件發生，繼續等待
            continue;

        // event_cnt代表有幾個事件觸發了
        for (int i = 0; i < event_cnt; i++)
        {
            if (all_events[i].data.fd == serv_sock) // 如果是新的連接請求
            {
                clnt_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (sockaddr *)&clnt_adr, &clnt_sz);

                // 把連接到的客戶端加入epoll，ET代表邊緣觸發
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = clnt_sock;

                // 將客戶端socket設置為非阻塞模式
                int flag = fcntl(clnt_sock, F_GETFL, 0);
                fcntl(clnt_sock, F_SETFL, flag | O_NONBLOCK);

                // 將客戶端socket添加到epoll中監聽
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("client is connected! clnt_sock:%d\n", clnt_sock);

                rs->addClient(clnt_sock);
            }
            else // 如果是已連接客戶端的數據
            {
                while (true) // 循環讀取buf
                {
                    memset(buf, 0, sizeof(buf)); // 清空 buf

                    //  讀取客戶端發送的數據
                    ssize_t len = read(all_events[i].data.fd, buf, sizeof(buf));

                    if (rs->filetranfermode)
                    {
                        if (len < 0)
                        {
                            if (errno == EAGAIN)
                            { // 非阻塞模式下，沒有數據可讀，會返回EAGAIN，不會卡在read()那行
                                break;
                            }
                        }
                        else
                            rs->receiveFile(len, buf);
                    }
                    else
                    {
                        if (len < 0)
                        {
                            if (errno == EAGAIN) // 非阻塞模式下，沒有數據可讀，會返回EAGAIN，不會卡在read()那行
                                break;
                            printf("epoll_wait error %d %s", errno, strerror(errno));
                            close(all_events[i].data.fd);
                            break;
                        }
                        else if (len == 0) // 客戶端斷開連接
                        {
                            epoll_ctl(epfd, EPOLL_CTL_DEL, all_events[i].data.fd, NULL);
                            close(all_events[i].data.fd);
                            printf("client is closed! clnt_sock:%d\n", all_events[i].data.fd);

                            rs->deleteClient(all_events[i].data.fd);

                            break;
                        }
                        else
                        {
                            rs->process(all_events[i].data.fd, buf);
                            // write(all_events[i].data.fd, buf, len);
                        }
                    }
                }
            }
        }
        // printf("%s(%d):%s \n", __FILE__, __LINE__, __FUNCTION__);
    }

    // 清理資源
    delete[] all_events;
    close(serv_sock);
    close(epfd);
}

void lession103(const char *arg)
{

    server103();
    // if (strcmp(arg, "s") == 0)
    // {
    //     server103();
    // }
    // else
    // {
    //     client101();
    // }
}

int main(int argc, char *argv[])
{
    lession103(argv[1]);
    return 0;
}