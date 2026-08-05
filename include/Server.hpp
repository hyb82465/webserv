#ifndef SERVER_HPP
# define SERVER_HPP

class Server
{
private:
    int _listenFd;
public:
    Server();
    ~Server();

    void run();
};

#endif