#include "server.h"
int main(int argc, char *argv[])
{
    try
    {
        Server server(9527);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}