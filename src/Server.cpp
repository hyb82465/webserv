#include "Server.hpp"
#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <utility>
#include <unistd.h>
#include <string>
#include <cstring>
#include <cstddef>
#include <fcntl.h>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <sstream>

Server::Server(const std::vector<ServerConfig> &configs) 
    : _listenFd(-1), _clients(), _cgiFds(), _pollFds(), _configs(configs)
{
    if (_configs.empty())
        throw std::runtime_error("No server configuration");
}

Server::~Server()
{
    for (std::vector<CgiHandler *>::iterator it =
             _cgiHandlers.begin();
         it != _cgiHandlers.end();
         ++it)
    {
        delete *it;
    }

    _cgiHandlers.clear();

    _cgiFds.clear();
    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd != -1)
            close(_pollFds[i].fd);
    }
}

bool Server::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        return false;
    return true;
}

bool Server::setupServer()
{
    for (std::size_t i = 0; i < _configs.size(); ++i)
    {
        const std::vector<ListenConfig> &listens =
            _configs[i].getListens();
        for (std::size_t j = 0; j < listens.size(); ++j)
        {
            std::cout << "server " << i
                      << " host="
                      << listens[j].getHost()
                      << " port="
                      << listens[j].getPort()
                      << std::endl;
        }
    }
    // socket
    // int socket(int domain, int type, int protocol);
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd == -1)
    {
        std::cerr << "socket failed" << std::endl;
        return false;
    }
    struct pollfd listenPollFd;
    listenPollFd.fd = _listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;
    _pollFds.push_back(listenPollFd);
    std::cout << "socket created, fd = " << _listenFd << std::endl;
    
    // non-blocking
    if (!setNonBlocking(_listenFd))
    {
        std::cerr << "failed to set listen socket non-blocking" << std::endl;
        return false;
    }

    // bind
    /* int bind(int socketFd,
                const struct sockaddr *address,
                socklen_t addressLength); */
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if (bind(
            _listenFd,
            reinterpret_cast<struct sockaddr *>(&addr),
            sizeof(addr)
        ) == -1)
    {
        std::cerr << "bind failed" << std::endl;
        return false;
    }

    // listen
    // int listen(int socketFd, int backlog);
    if (listen(_listenFd, SOMAXCONN) == -1)
    {
        std::cerr << "listen failed" << std::endl;
        return false;
    }
    return true;
}

void Server::removeClient(int fd, std::size_t i)
{
    close(fd);
    _clients.erase(fd);
    _pollFds.erase(_pollFds.begin() + i);
}

void Server::acceptClient()
{
    // accept
    /* int accept(int socketFd,
                struct sockaddr *clientAddress,
                socklen_t *clientAddressLength); */
    // struct sockaddr_in clientAddr;
    // socklen_t clientAddrLen = sizeof(clientAddr);
    // std::memset(&clientAddr, 0, sizeof(clientAddr));
    // int clientFd = accept(
    //     _listenFd,
    //     reinterpret_cast<struct sockaddr *>(&clientAddr),
    //     &clientAddrLen
    // );
    int clientFd = accept(_listenFd, NULL, NULL);
    if (clientFd == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "accept failed" << std::endl;
        return ;
    }

    // non-blocking
    if (!setNonBlocking(clientFd))
    {
        std::cerr << "failed to set client socket non-blocking" << std::endl;
        close(clientFd);
        return ;
    }

    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;

    _pollFds.push_back(clientPollFd);
    _clients.insert(std::make_pair(clientFd, Client(clientFd)));
     std::cout << "client connected, fd = " << clientFd << std::endl;
}

void Server::handleRead(int fd, std::size_t &i)
{
    // recv
    /* ssize_t recv(int socketFd,
                    void *buffer,
                    size_t length,
                    int flags); */
    char buffer[4096];
    ssize_t byteRead = recv(fd, buffer, sizeof(buffer), 0);
    if (byteRead == -1)
    {
        std::cerr << "recv failed" << std::endl;
        removeClient(fd, i);
        return ;
    }
    else if (byteRead == 0)
    {
        std::cout << "client disconnected" << std::endl;
        removeClient(fd, i);
        return ;
    }
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) // should not happen
    {
        std::cerr << "client not found" << std::endl;
        removeClient(fd, i);
        return ;
    }
    it->second.appendToReadBuffer(buffer, static_cast<std::size_t>(byteRead));
    HttpRequest request;
    RequestParser parser;
    ParseResult result = parser.parse(it->second.getReadBuffer(), request);
    if (result == PARSE_INCOMPLETE)
    {
        std::cout << "request incomplete" << std::endl;
        ++i;
        return ;
    }
    else if (result == PARSE_ERROR)
    {
        std::cout << "parse error, status = "
                  << request.getStatus()
                  << std::endl;
        ++i;
        return ;
    }
    
    std::cout << "method: "
              << request.getMethod()
              << std::endl;
    std::cout << "path: "
              << request.getPath()
              << std::endl;
    std::cout << "body: "
              << request.getBody()
              << std::endl;

    std::cout << it->second.getReadBuffer() << std::endl;

    RequestHandler handler(_configs[0]);
    HttpResponse response = handler.handle(request);
    it->second.setWriteBuffer(response.getResponse());

    _pollFds[i].events = POLLOUT;
    ++i;
}

void Server::handleWrite(int fd, std::size_t &i)
{
    // send
    /* ssize_t send(int socketFd,
                    const void *buffer,
                    size_t length,
                    int flags); */
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) // should not happen
    {
        std::cerr << "client not found" << std::endl;
        removeClient(fd, i);
        return ;
    }
    const std::string &response = it->second.getWriteBuffer();
    ssize_t bytesSent = send(
        fd,
        response.c_str() + it->second.getBytesSent(),
        response.size() - it->second.getBytesSent(),
        0
    );
    if (bytesSent == -1)
    {
        std::cerr << "send failed" << std::endl;
        removeClient(fd, i);
        return ;
    }
    it->second.addBytesSent(static_cast<std::size_t>(bytesSent));
    if (it->second.getBytesSent() >= response.size())
    {
        std::cout << "totally sent " << it->second.getBytesSent() << " bytes" << std::endl;
        removeClient(fd, i);
        return ;
    }
    ++i;
}

void Server::startCgi(int clientFd, const HttpRequest &request,
    const LocationConfig &location, const std::string &executable)
{
    std::string scriptPath = buildCgiScriptPath(request.getPath(), location);

        CgiHandler *cgi = new CgiHandler(clientFd, executable, scriptPath);
        try
        {
            cgi->start(request);
        }
        catch (...)
        {
            delete cgi;
            throw;
        }

        addCgi(cgi);
        addCgiPollFds(cgi);
}
void Server::handleCgiWrite(int fd, std::size_t &i)
{
    CgiHandler *cgi = getCgiByFd(fd);

    if (cgi == NULL)
    {
        removePollFd(fd);
        return;
    }

    cgi->writeBody();

    // Request body completely written.CgiHandler already closed stdin.

    if (!cgi->isStdinOpen())
    {
        _cgiFds.erase(fd);
        removePollFd(fd);
        //do NOT delete CGI here. stdout is still needed.
        return;
    }

    ++i;
}

void Server::handleCgiRead(int fd, std::size_t &i)
{
    CgiHandler *cgi = getCgiByFd(fd);

    if (cgi == NULL)
    {
        removePollFd(fd);
        return;
    }

    cgi->readOutput();

    if (!cgi->isStdoutOpen())
    {
        _cgiFds.erase(fd);
        removePollFd(fd);

      // stdout EOF doesn't necessarily mean waitpid() already succeeded.
        if (cgi->waitForChild())
        {
            finishCgi(cgi);
        }

        return;
    }

    ++i;
}
CgiHandler *Server::getCgiByFd(int fd)
{
     std::map<int, CgiHandler *>::iterator it =
        _cgiFds.find(fd);

    if (it == _cgiFds.end())
        return NULL;

    return it->second;
}

void Server::addCgi(CgiHandler *cgi)
{
    if (cgi == NULL)
        return;
    _cgiHandlers.push_back(cgi);

    if (cgi->getStdinFd() != -1)
    {
        _cgiFds[cgi->getStdinFd()] = cgi;
    }

    if (cgi->getStdoutFd() != -1)
    {
        _cgiFds[cgi->getStdoutFd()] = cgi;
    }
}

void Server::removeCgi(CgiHandler *cgi)
{
    if (cgi == NULL)
        return;

    int stdinFd =
        cgi->getStdinFd();

    int stdoutFd =
        cgi->getStdoutFd();

    if (stdinFd != -1)
    {
        _cgiFds.erase(stdinFd);
        removePollFd(stdinFd);
    }

    if (stdoutFd != -1)
    {
        _cgiFds.erase(stdoutFd);
        removePollFd(stdoutFd);
    }

    for (std::vector<CgiHandler *>::iterator it = _cgiHandlers.begin();
         it != _cgiHandlers.end(); ++it)
    {
        if (*it == cgi)
        {
            delete *it;

            _cgiHandlers.erase(it);

            return;
        }
    }
}
void Server::finishCgi(CgiHandler *cgi)
{
    if (cgi == NULL)
        return;

    int clientFd = cgi->getClientFd();

    std::map<int, Client>::iterator clientIt = _clients.find(clientFd);

    /*
     * Client disappeared while CGI
     * was running.
     */
    if (clientIt == _clients.end())
    {
        removeCgi(cgi);
        return;
    }

    /*
     * CGI output
     *
     * CGI headers/body
     *       ↓
     * HTTP response
     */
    std::string response = buildCgiResponse(cgi->getOutput());

    clientIt->second.setWriteBuffer(response);

    /*
     * Client now waits for POLLOUT.
     */
    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd == clientFd)
        {
            _pollFds[i].events = POLLOUT;

            break;
        }
    }

    /*
     * Now CGI is completely finished.
     */
    removeCgi(cgi);
}
void Server::removePollFd(int fd)
{
    for (std::size_t i = 0;
         i < _pollFds.size();
         ++i)
    {
        if (_pollFds[i].fd == fd)
        {
            _pollFds.erase(
                _pollFds.begin() + i);
            return;
        }
    }
}
void Server::addCgiPollFds(CgiHandler *cgi)
{
if (cgi == NULL)
        return;

    if (cgi->getStdinFd() != -1)
    {
        struct pollfd stdinPoll;

        stdinPoll.fd =
            cgi->getStdinFd();

        stdinPoll.events =
            POLLOUT;

        stdinPoll.revents = 0;

        _pollFds.push_back(
            stdinPoll);
    }

    if (cgi->getStdoutFd() != -1)
    {
        struct pollfd stdoutPoll;

        stdoutPoll.fd =
            cgi->getStdoutFd();

        stdoutPoll.events =
            POLLIN;

        stdoutPoll.revents = 0;

        _pollFds.push_back(
            stdoutPoll);
    }
}

void Server::checkCgiChildren()
{
    for (std::size_t i = 0;
         i < _cgiHandlers.size();)
    {
        CgiHandler *cgi =
            _cgiHandlers[i];

        if (!cgi->isStdoutOpen())
        {
            if (cgi->waitForChild())
            {
                finishCgi(cgi);
                 //finishCgi() will remove the CGI from _cgiHandlers. Don't increment i.
                continue;
            }
        }

        ++i;
    }
}

const LocationConfig *Server::findLocation(const std::string &path, const ServerConfig &config) const
{
    const LocationConfig *best = NULL;
    std::size_t bestLength = 0;

    const std::vector<LocationConfig> &locations = config.getLocations();

    for (std::size_t i = 0; i < locations.size(); ++i)
    {
        const std::string &locationPath = locations[i].getPath();

        if (locationPath.empty())
            continue;

        if (path.compare(
                0,
                locationPath.length(),
                locationPath) != 0)
        {
            continue;
        }

        /*
         * /cgi should not match /cgi123
         */
        if (path.length() >
            locationPath.length())
        {
            if (locationPath[locationPath.length() - 1] != '/' &&
                path[locationPath.length()] != '/')
                continue;
        }

        /*
         * location /cgi
         * location /cgi-bin.
         * request /cgi-bin/test.py
         * get /cgi-bin
         */
        if (locationPath.length() > bestLength)
        {
            best = &locations[i];

            bestLength = locationPath.length();
        }
    }

    return best;
}

std::string Server::findCgiExecutable(const std::string &path, const LocationConfig &location) const
{
    std::string::size_type pos = path.find_last_of('.');

    if (pos == std::string::npos)
        return "";

    std::string extension = path.substr(pos);

    const std::map<std::string, std::string> &cgi = location.getCgi();

    std::map<std::string, std::string>::const_iterator it = cgi.find(extension);

    if (it == cgi.end())
        return "";

    return it->second;
}

// request /cgi-bin/test.py    get: ./www/cgi-bin/test.py
std::string Server::buildCgiScriptPath(const std::string &path, const LocationConfig &location) const
{
     std::string root = location.getRoot();

    if (root.empty())
        return "";

    if (path.empty())
        return root;

    if (root[root.length() - 1] == '/' && path[0] == '/')
    {
        return root + path.substr(1);
    }

    if (root[root.length() - 1] != '/' && path[0] != '/')
    {
        return root +"/" +path;
    }

    return root + path;
}


std::string Server::buildCgiResponse(const std::string &output) const
{
    std::string::size_type pos = output.find("\r\n\r\n");

    std::size_t separatorLength = 4;

    if (pos == std::string::npos)
    {
        pos = output.find("\n\n");
        separatorLength = 2;
    }

    /*
     * CGI returned no headers.
     */
    if (pos == std::string::npos)
    {
        std::string response;

        response += "HTTP/1.1 200 OK\r\n";

        response += "Content-Type: text/html\r\n";

        response += "Content-Length: " + toString(output.size()) + "\r\n";

        response += "Connection: close\r\n";

        response += "\r\n";

        response += output;

        return response;
    }

    std::string headers = output.substr(0, pos);

    std::string body = output.substr(pos + separatorLength);

    std::string response;

    response += "HTTP/1.1 200 OK\r\n";

    response += headers;

    /*
     * Add Content-Length.
     */
    response += "\r\nContent-Length: " + toString(body.size());

    response += "\r\nConnection: close\r\n";

    response += "\r\n";

    response += body;

    return response;
}

std::string Server::toString(std::size_t value) const
{
    std::ostringstream stream;

    stream << value;

    return stream.str();
}

void Server::run()
{
    if (!setupServer())
        return ;
    while (true)
    {
        // poll
        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
        int readyCount = poll(&_pollFds[0], _pollFds.size(), -1);
        if (readyCount == -1)
        {
            std::cerr << "poll failed" << std::endl;
            return ;
        }
        for (std::size_t i = 0; i < _pollFds.size(); )
        {
            if (_pollFds[i].revents == 0)
            {
                ++i;
                continue ;
            }    
            int fd = _pollFds[i].fd;
            if (fd == _listenFd
                && _pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "listen socket error" << std::endl;
                return ;
            }
            if (fd != _listenFd
                && _pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "client connection closed or invalid" << std::endl;
                removeClient(fd, i);
                continue ;
            }
            if (fd == _listenFd
                && (_pollFds[i].revents & POLLIN))
            {
                acceptClient();
                ++i;
            }
            else if (_pollFds[i].revents & POLLIN)
                handleRead(fd, i);
            else if (_pollFds[i].revents & POLLOUT)
                handleWrite(fd, i);
            else
                ++i;
        }
    }
}
