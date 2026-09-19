#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "HttpRequest.hpp"

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <ctime>

class CgiHandler
{
private:
    int _clientFd;
    pid_t _pid;
    int _stdinFd;
    int _stdoutFd;
    bool _stdinOpen;
    bool _stdoutOpen;
    std::string _executable;
    std::string _scriptPath;
    int  _serverPort;
    std::string _remoteAddr;
    std::string _serverName;  
    std::string _requestBody;
    std::size_t _bodyOffset; // number of bytes written to CGI
    std::string _output;
    std::vector<std::string> _environment;
    int _exitStatus;
    bool _childFinished;
    bool _ioFailed;
    std::time_t _lastActivity;

    CgiHandler(const CgiHandler &other);
    CgiHandler &operator=(const CgiHandler &other);

    void setNonBlocking(int fd);
    void buildEnvironment(const HttpRequest &request);
    char **createEnvp() const;
    void freeEnvp(char **envp) const;

    std::string getDirectory(const std::string &path) const;
    std::string getFileName(const std::string &path) const;

public:
    CgiHandler(int clientFd,
               const std::string &executable,
               const std::string &scriptPath,
               int serverPort,
               const std::string &remoteAddr,
               const std::string &serverName);
    ~CgiHandler();

    void start(HttpRequest &request, const std::vector<int> &listenFds);
    bool writeBody();
    bool readOutput();
    void closeInput();
    void closeOutput();

    int getClientFd() const;
    int getStdinFd() const;
    int getStdoutFd() const;
    pid_t getPid() const;

    bool isStdinOpen() const;
    bool isStdoutOpen() const;
    bool waitForChild();
    bool isChildSuccess() const;
    bool hasTimedOut(int timeoutSeconds) const;
    bool hasIoFailed() const;
    void markIoFailed();

    void killChild();
    void swapOutput(std::string &output);
};

#endif