#include "server.h"

Server::Server(uint16_t port) : port_(port)
{
    initializeServer();
}

void Server::run()
{
    try
    {
        eventLoop();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

void Server::initializeServer()
{
    createSocket();
    setSocketOptions();
    bindSocket();
    startListening();
    initializeEpoll();
    responser_ = std::make_unique<Responser>();
}

void Server::createSocket()
{
    serverSocket_ = socket(PF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1)
    {
        throw std::system_error(errno, std::system_category(), "Failed to create socket");
    }
}

void Server::setSocketOptions()
{
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        throw std::system_error(errno, std::system_category(), "Failed to set socket options");
    }
}

void Server::bindSocket()
{
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port_);

    if (bind(serverSocket_, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1)
    {
        throw std::system_error(errno, std::system_category(), "Failed to bind socket");
    }
}

void Server::startListening()
{
    if (listen(serverSocket_, 5) == -1)
    {
        throw std::system_error(errno, std::system_category(), "Failed to listen on socket");
    }
}

void Server::Server::initializeEpoll()
{
    epollFd_ = epoll_create(1);
    if (epollFd_ == -1)
    {
        throw std::system_error(errno, std::system_category(), "Failed to create epoll instance");
    }
    event_.events = EPOLLIN;
    event_.data.fd = serverSocket_;

    epoll_ctl(epollFd_, EPOLL_CTL_ADD, serverSocket_, &event_);
    events_.resize(MAX_EVENTS);
}

void Server::eventLoop()
{

    char buf[1024 * 1024] = ""; // 用於儲存接收到的數據的緩衝區

    while (true)
    {
        int numEvents = epoll_wait(epollFd_, events_.data(), MAX_EVENTS, 1000);

        // if (numEvents == -1)
        // {
        //     if (errno == EINTR)
        //         continue;
        //     throw std::system_error(errno, std::system_category(), "Epoll wait failed");
        // }

        if (numEvents == -1)
        {
            printf("epoll_wait error %d %s", errno, strerror(errno));
            break;
        }
        if (numEvents == 0) // 如果沒有事件發生，繼續等待
            continue;

        for (int i = 0; i < numEvents; ++i)
        {
            if (events_[i].data.fd == serverSocket_)
            {
                sockaddr_in clnt_adr;
                // handleNewConnection();
                socklen_t clnt_sz = sizeof(clnt_adr);
                int clnt_sock = accept(serverSocket_, (sockaddr *)&clnt_adr, &clnt_sz);

                // 把連接到的客戶端加入epoll，ET代表邊緣觸發
                event_.events = EPOLLIN | EPOLLET;
                event_.data.fd = clnt_sock;

                // 將客戶端socket設置為非阻塞模式
                int flag = fcntl(clnt_sock, F_GETFL, 0);
                fcntl(clnt_sock, F_SETFL, flag | O_NONBLOCK);

                // 將客戶端socket添加到epoll中監聽
                epoll_ctl(epollFd_, EPOLL_CTL_ADD, clnt_sock, &event_);
                printf("client is connected! clnt_sock:%d\n", clnt_sock);

                responser_->addClient(clnt_sock);
            }
            else
            {
                // handleClientData(events_[i].data.fd);
                while (true) // 循環讀取buf
                {
                    memset(buf, 0, sizeof(buf)); // 清空 buf

                    //  讀取客戶端發送的數據
                    ssize_t len = read(events_[i].data.fd, buf, sizeof(buf));

                    if (len < 0)
                    {
                        if (errno == EAGAIN) // 非阻塞模式下，沒有數據可讀，會返回EAGAIN，不會卡在read()那行
                            break;
                        printf("epoll_wait error %d %s", errno, strerror(errno));
                        close(events_[i].data.fd);
                        break;
                    }
                    else if (len == 0) // 客戶端斷開連接
                    {
                        epoll_ctl(epollFd_, EPOLL_CTL_DEL, events_[i].data.fd, NULL);
                        close(events_[i].data.fd);
                        printf("client is closed! clnt_sock:%d\n", events_[i].data.fd);
                        responser_->deleteClient(events_[i].data.fd);
                        break;
                    }
                    else
                    {
                        responser_->process(events_[i].data.fd, len, buf);
                    }
                }
            }
        }
    }
}