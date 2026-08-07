#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <string>
# include <cstddef>

class Client
{
private:
    int _fd;
    std::string _readBuffer;
    // std::string _writeBuffer;
    // std::size_t _bytesSent;
public:
    Client();
    Client(int fd);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client();

    int getFd() const;

    const std::string &getReadBuffer() const;
    void appendToReadBuffer(const char *data, std::size_t length);
};

#endif