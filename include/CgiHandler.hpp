#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include "HttpRequest.hpp"

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>

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

    std::string _requestBody;
    std::size_t _bodyOffset;

    std::string _output;

    std::vector<std::string> _environment;

    CgiHandler(const CgiHandler &other);
    CgiHandler &operator=(const CgiHandler &other);

    void setNonBlocking(int fd);
    void buildEnvironment(const HttpRequest &request);
    char **creatEnvp() const;
    void freeEnvp(char **envp) const;

    std::string getDirectory(const std::string &path) const;

    std::string getFileName(const std::string &path) const;
public:
    CgiHandler(int clientFd, const std::string &executable, const std::string &scriptPath);
    ~CgiHandler();

    void start(const HttpRequest &request);

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
    
    const std::string &getOutput() const;
};


#endif