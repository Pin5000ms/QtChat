#include "responser.h"

class Server
{
private:
    void initializeServer();
    void createSocket();
    void setSocketOptions();
    void bindSocket();
    void startListening();
    void initializeEpoll();
    void eventLoop();

    uint16_t port_;
    int serverSocket_;
    int epollFd_;
    std::unique_ptr<Responser> responser_;

    static constexpr size_t MAX_EVENTS = 100;
    std::vector<epoll_event> events_;
    epoll_event event_;

public:
    Server(uint16_t port);
    void run();
};