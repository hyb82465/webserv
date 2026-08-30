#include "Server.hpp"
#include "HttpRequest.hpp"
#include "RequestParser.hpp"
#include "HttpResponse.hpp"
#include "RequestHandler.hpp"
#include "Signal.hpp"
#include "Utils.hpp"
#include <sys/socket.h>
#include <netdb.h>
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

Server::Server(const std::vector<ServerConfig>& configs)
    : _configs(configs)
{
    if (_configs.empty())
        throw std::runtime_error("No server configuration");
}

Server::~Server()
{
    for (std::vector<CgiHandler*>::iterator it =
        _cgiHandlers.begin();
        it != _cgiHandlers.end();
        ++it)
    {
        delete* it;
    }

    _cgiHandlers.clear();
    _cgiFds.clear();
    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd != -1)
            close(_pollFds[i].fd);
    }
    std::cout << "Server destructor called" << std::endl;
}

bool Server::isListenFd(int fd) const
{
    for (std::size_t i = 0; i < _listenFds.size(); ++i)
    {
        if (_listenFds[i] == fd)
            return true;
    }
    return false;
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

int Server::setupListenSocket(const ListenConfig& listenConfig)
{
    // socket
    // int socket(int domain, int type, int protocol);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cerr << "socket failed" << std::endl;
        return -1;
    }

    // non-blocking
    if (!setNonBlocking(fd))
    {
        std::cerr << "failed to set listen socket non-blocking" << std::endl;
        close(fd);
        return -1;
    }

    // bind
    /* int bind(int socketFd,
                const struct sockaddr *address,
                socklen_t addressLength);
    */
    struct addrinfo hints;
    struct addrinfo* result = NULL;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // server bind() address

    std::stringstream ss;
    ss << listenConfig.getPort();
    std::string port = ss.str();
    std::string host = listenConfig.getHost();
    if (getaddrinfo(host.c_str(), port.c_str(), &hints, &result) != 0)
    {
        std::cerr << "getaddrinfo failed" << std::endl;
        close(fd);
        return -1;
    }
    if (bind(fd, result->ai_addr, result->ai_addrlen) == -1)
    {
        std::cerr << "bind failed" << std::endl;
        freeaddrinfo(result);
        close(fd);
        return -1;
    }
    freeaddrinfo(result);

    // listen
    // int listen(int socketFd, int backlog);
    if (listen(fd, SOMAXCONN) == -1)
    {
        std::cerr << "listen failed" << std::endl;
        close(fd);
        return -1;
    }

    _listenFds.push_back(fd);

    struct pollfd listenPollFd;
    listenPollFd.fd = fd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;
    _pollFds.push_back(listenPollFd);

    std::cout << "listening on "
        << host << ":" << port
        << ", fd = " << fd << std::endl;

    return fd;
}

void Server::removeClient(int fd, std::size_t i)
{
    close(fd);
    _clients.erase(fd);
    _pollFds.erase(_pollFds.begin() + i);
}

void Server::processRequest(int fd, std::size_t& i)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
    {
        std::cerr << "client not found" << std::endl;
        removeClient(fd, i);
        return;
    }
    std::size_t serverIndex = it->second.getServerIndex();
    if (serverIndex >= _configs.size())
    {
        std::cerr << "invalid server index" << std::endl;
        removeClient(fd, i);
        return;
    }
    const ServerConfig& config = _configs[serverIndex];
    RequestState& state = it->second.getRequestState();
    HttpRequest& request = state.request;
    RequestParser parser;
    std::size_t maxBodySize = config.getClientMaxBodySize();
    ParseResult result = parser.parse(it->second.getReadBuffer(), state, maxBodySize);
    if (result == PARSE_INCOMPLETE)
    {
        // std::cout << "request incomplete" << std::endl;
        _pollFds[i].events = POLLIN;
        ++i;
        return;
    }
    else if (result == PARSE_ERROR)
    {
        std::cout << "parse error, status = "
            << request.getStatus()
            << std::endl;
        state.keepAlive = false;
        RequestHandler handler(config);
        HttpResponse response = handler.handleError(request.getStatus());
        response.setHeader("Connection", "close");
        it->second.setWriteBuffer(response.getResponse());
        _pollFds[i].events = POLLOUT;
        ++i;
        return;
    }
    RequestHandler handler(config);
    const LocationConfig* location = handler.getLocation(request);
    if (location != NULL && !location->getCgi().empty())
    {
        std::string executable = findCgiExecutable(request.getPath(), *location);
        if (!executable.empty())
        {
            std::string scriptPath = handler.buildPath(location, request.getPath());
            if (scriptPath.empty())
            {
                state.keepAlive = false;
                HttpResponse response = handler.handleError(HTTP_INTERNAL_SERVER_ERROR);
                response.setHeader("Connection", "close");
                it->second.setWriteBuffer(response.getResponse());
                _pollFds[i].events = POLLOUT;
                ++i;
                return;
            }
            try
            {
                startCgi(fd, request, scriptPath, executable);
                std::cout << "[CGI START]"
                    << " fd=" << fd
                    << " method=" << request.getMethod()
                    << " path=" << request.getPath()
                    << " body=" << request.getBody().size()
                    << std::endl;
                _pollFds[i].events = 0;
            }
            catch (const std::exception& e)
            {
                std::cerr << "CGI failed: " << e.what() << std::endl;
                state.keepAlive = false;
                HttpResponse response = handler.handleError(HTTP_INTERNAL_SERVER_ERROR);
                response.setHeader("Connection", "close");
                it->second.setWriteBuffer(response.getResponse());
                _pollFds[i].events = POLLOUT;
            }
            ++i;
            return;
        }
    }
    HttpResponse response = handler.handle(request);
    if (request.getMethod() != "GET" && request.getMethod() != "POST" && request.getMethod() != "DELETE")
    {
        state.keepAlive = false;
    }
    if (state.keepAlive)
        response.setHeader("Connection", "keep-alive");
    else
        response.setHeader("Connection", "close");
    it->second.setWriteBuffer(response.getResponse());
    _pollFds[i].events = POLLOUT;
    ++i;
}

void Server::acceptClient(int listenFd)
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
    int clientFd = accept(listenFd, NULL, NULL);
    if (clientFd == -1)
    {
        std::cerr << "accept failed" << std::endl;
        return;
    }

    // non-blocking
    if (!setNonBlocking(clientFd))
    {
        std::cerr << "failed to set client socket non-blocking" << std::endl;
        close(clientFd);
        return;
    }

    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;

    _pollFds.push_back(clientPollFd);
    std::map<int, std::size_t>::const_iterator it = _listenServerMap.find(listenFd);
    if (it == _listenServerMap.end())
    {
        std::cerr << "listen fd has no server config" << std::endl;
        return;
    }
    std::size_t serverIndex = it->second;
    _clients.insert(std::make_pair(clientFd, Client(clientFd, serverIndex)));
    std::cout << "client connected, fd = " << clientFd << std::endl;
}

void Server::handleRead(int fd, std::size_t& i)
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
        return;
    }
    else if (byteRead == 0)
    {
        std::cout << "client disconnected" << std::endl;
        removeClient(fd, i);
        return;
    }
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end()) // should not happen
    {
        std::cerr << "client not found" << std::endl;
        removeClient(fd, i);
        return;
    }
    it->second.appendToReadBuffer(buffer, static_cast<std::size_t>(byteRead));
    processRequest(fd, i);
}

void Server::handleWrite(int fd, std::size_t& i)
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
        return;
    }
    const std::string& response = it->second.getWriteBuffer();
    if (it->second.getBytesSent() == 0)
    {
        RequestState& state = it->second.getRequestState();

        std::string::size_type end = response.find("\r\n");

        std::cout << "[RESPONSE]"
            << " fd=" << fd
            << " method=" << state.request.getMethod()
            << " path=" << state.request.getPath()
            << " status="
            << response.substr(0, end)
            << std::endl;
    }
    ssize_t bytesSent = send(
        fd,
        response.c_str() + it->second.getBytesSent(),
        response.size() - it->second.getBytesSent(),
        0);
    if (bytesSent <= 0)
    {
        std::cerr << "send failed" << std::endl;
        removeClient(fd, i);
        return;
    }
    it->second.addBytesSent(static_cast<std::size_t>(bytesSent));
    if (it->second.getBytesSent() >= response.size())
    {
        std::cout << "totally sent " << it->second.getBytesSent() << " bytes" << std::endl;
        RequestState& state = it->second.getRequestState();
        if (!state.keepAlive)
        {
            removeClient(fd, i);
            return;
        }
        it->second.consumeReadBuffer(state.pos);
        it->second.clearWriteBuffer();
        it->second.resetBytesSent();
        it->second.resetRequestState();
        if (!it->second.getReadBuffer().empty())
        {
            processRequest(fd, i);
            return;
        }
        _pollFds[i].events = POLLIN;
        ++i;
        return;
    }
    ++i;
}

void Server::startCgi(int clientFd, const HttpRequest& request,
    const std::string& scriptPath, const std::string& executable)
{
    CgiHandler* cgi = new CgiHandler(clientFd, executable, scriptPath);
    try
    {
        std::cout << "[CGI DEBUG]"
            << " executable=" << executable
            << " scriptPath=" << scriptPath
            << std::endl;
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
void Server::handleCgiWrite(int fd, std::size_t& i)
{
    CgiHandler* cgi = getCgiByFd(fd);

    if (cgi == NULL)
    {
        _cgiFds.erase(fd);
        removePollFd(fd);
        return;
    }

    cgi->writeBody();

    // Request body completely written.CgiHandler already closed stdin.

    if (!cgi->isStdinOpen())
    {
        _cgiFds.erase(fd);
        removePollFd(fd);
        // do NOT delete CGI here. stdout is still needed.
        return;
    }

    ++i;
}

void Server::handleCgiRead(int fd, std::size_t& i)
{
    CgiHandler* cgi = getCgiByFd(fd);

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
CgiHandler* Server::getCgiByFd(int fd)
{
    std::map<int, CgiHandler*>::iterator it = _cgiFds.find(fd);

    if (it == _cgiFds.end())
        return NULL;

    return it->second;
}

void Server::addCgi(CgiHandler* cgi)
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

void Server::removeCgi(CgiHandler* cgi)
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

    for (std::vector<CgiHandler*>::iterator it = _cgiHandlers.begin();
        it != _cgiHandlers.end(); ++it)
    {
        if (*it == cgi)
        {
            delete* it;

            _cgiHandlers.erase(it);

            return;
        }
    }
}
void Server::finishCgi(CgiHandler* cgi)
{
    if (cgi == NULL)
        return;
    int clientFd = cgi->getClientFd();
    std::map<int, Client>::iterator clientIt = _clients.find(clientFd);
    // Client disappeared while CGI was running.
    if (clientIt == _clients.end())
    {
        removeCgi(cgi);
        return;
    }
    RequestState& state = clientIt->second.getRequestState();
    std::string response;
    if (cgi->isChildSuccess())
        response = buildCgiResponse(cgi->getOutput(), state.keepAlive);
    else
    {
        state.keepAlive = false;
        RequestHandler handler(_configs[clientIt->second.getServerIndex()]);
        HttpResponse error = handler.handleError(HTTP_INTERNAL_SERVER_ERROR);
        error.setHeader("Connection", "close");
        response = error.getResponse();
    }
    clientIt->second.setWriteBuffer(response);
    // Client now waits for POLLOUT.
    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd == clientFd)
        {
            _pollFds[i].events = POLLOUT;
            break;
        }
    }
    removeCgi(cgi);
}
void Server::removePollFd(int fd)
{
    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd == fd)
        {
            _pollFds.erase(_pollFds.begin() + i);
            return;
        }
    }
}

void Server::addCgiPollFds(CgiHandler* cgi)
{
    if (cgi == NULL)
        return;

    if (cgi->getStdinFd() != -1)
    {
        struct pollfd stdinPoll;

        stdinPoll.fd = cgi->getStdinFd();

        stdinPoll.events = POLLOUT;

        stdinPoll.revents = 0;

        _pollFds.push_back(stdinPoll);
    }

    if (cgi->getStdoutFd() != -1)
    {
        struct pollfd stdoutPoll;

        stdoutPoll.fd = cgi->getStdoutFd();

        stdoutPoll.events = POLLIN;

        stdoutPoll.revents = 0;

        _pollFds.push_back(stdoutPoll);
    }
}

void Server::checkCgiChildren()
{
    for (std::size_t i = 0;
        i < _cgiHandlers.size();)
    {
        CgiHandler* cgi = _cgiHandlers[i];

        if (!cgi->isStdoutOpen())
        {
            if (cgi->waitForChild())
            {
                finishCgi(cgi);
                // finishCgi() will remove the CGI from _cgiHandlers. Don't increment i.
                continue;
            }
        }

        ++i;
    }
}

std::string Server::findCgiExecutable(const std::string& path, const LocationConfig& location) const
{
    std::string::size_type pos = path.find_last_of('.');

    if (pos == std::string::npos)
        return "";

    std::string extension = path.substr(pos);

    const std::map<std::string, std::string>& cgi = location.getCgi();

    std::map<std::string, std::string>::const_iterator it = cgi.find(extension);

    if (it == cgi.end())
        return "";

    return it->second;
}

std::string Server::buildCgiResponse(const std::string& output, bool keepAlive) const
{
    std::string::size_type pos = output.find("\r\n\r\n");
    std::size_t separatorLength = 4;
    if (pos == std::string::npos)
    {
        pos = output.find("\n\n");
        separatorLength = 2;
    }
    // CGI returned no headers.
    if (pos == std::string::npos)
    {
        std::string response;
        response += "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: "
            + Utils::sizetToString(output.size())
            + "\r\n";
        if (keepAlive)
            response += "Connection: keep-alive\r\n";
        else
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
    // Add Content-Length
    response += "\r\nContent-Length: "
        + Utils::sizetToString(body.size())
        + "\r\n";
    if (keepAlive)
        response += "Connection: keep-alive\r\n";
    else
        response += "Connection: close\r\n";
    response += "\r\n";
    response += body;
    return response;
}

void Server::checkCgiTimeouts()
{
    const int CGI_TIMEOUT = 5;
    std::size_t i = 0;
    while (i < _cgiHandlers.size())
    {
        CgiHandler* cgi = _cgiHandlers[i];
        if (!cgi->hasTimedOut(CGI_TIMEOUT))
        {
            ++i;
            continue;
        }
        std::cout << "CGI timeout" << std::endl;
        cgi->killChild();
        finishCgi(cgi);
    }
}

void Server::run()
{
    for (std::size_t i = 0; i < _configs.size(); ++i)
    {
        const std::vector<ListenConfig>& listens =
            _configs[i].getListens();
        for (std::size_t j = 0; j < listens.size(); ++j)
        {
            std::cout << "server " << i
                << " host="
                << listens[j].getHost()
                << " port="
                << listens[j].getPort()
                << std::endl;
            int listenFd = setupListenSocket(listens[j]);
            if (listenFd == -1)
                continue;
            else
                _listenServerMap[listenFd] = i;
        }
    }
    if (_listenFds.empty())
    {
        std::cerr << "listen socket failed" << std::endl;
        return;
    }
    while (g_running)
    {
        // poll
        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);
        checkCgiTimeouts();
        checkCgiChildren();
        int readyCount = poll(&_pollFds[0], _pollFds.size(), 100);
        if (readyCount == -1)
        {
            if (!g_running)
                break;
            std::cerr << "poll failed" << std::endl;
            return;
        }

        checkCgiTimeouts();

        if (readyCount == 0)
            continue;

        for (std::size_t i = 0; i < _pollFds.size();)
        {
            if (_pollFds[i].revents == 0)
            {
                ++i;
                continue;
            }
            int fd = _pollFds[i].fd;
            // CGI： check whether it is CGI pipe or client socket
            if (_cgiFds.find(fd) != _cgiFds.end())
            {
                if (_pollFds[i].revents & POLLOUT)
                {
                    handleCgiWrite(fd, i);
                    continue;
                }
                if (_pollFds[i].revents & POLLIN)
                {
                    handleCgiRead(fd, i);
                    continue;
                }
                if (_pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
                {
                    CgiHandler* cgi = getCgiByFd(fd);
                    if (cgi != NULL && cgi->getStdoutFd() == fd)
                    {
                        cgi->readOutput();
                        if (!cgi->isStdoutOpen())
                        {
                            _cgiFds.erase(fd);
                            removePollFd(fd);
                            if (cgi->waitForChild())
                                finishCgi(cgi);
                            continue;
                        }
                    }
                    ++i;
                    continue;
                }
                ++i;
                continue;
            }
            // Listen Socket
            if (isListenFd(fd) && _pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "listen socket error" << std::endl;
                return;
            }
            if (!isListenFd(fd) && _pollFds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                std::cerr << "client connection closed or invalid" << std::endl;
                removeClient(fd, i);
                continue;
            }
            if (isListenFd(fd) && (_pollFds[i].revents & POLLIN))
            {
                acceptClient(fd);
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