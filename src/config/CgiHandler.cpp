#include "CgiHandler.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

 CgiHandler::CgiHandler(int clientFd, const std::string &executable, const std::string &scriptPath)
        :_clientFd(clientFd), _pid(-1), 
         _stdinFd(-1),_stdoutFd(-1),
         _stdinOpen(false), _stdoutOpen(-1),
         _executable(executable), _scriptPath(scriptPath),
         _requestBody(""),
         _bodyOffset(0),
         _output(""),
         _environment()
{

}

    CgiHandler::~CgiHandler()
    {
        if(_stdinFd != -1)
            close(_stdinFd);
        if (_stdoutFd != -1)
            close(_stdoutFd);
        if (_pid > 0)
        {
            int status;

            waitpid(_pid, &status, WNOHANG);
        }
    }

    void start(const HttpRequest &request)
    {
        int inputPipe[2];
        int outputPipe[2];

        if (pipe(inputPipe) == -1)
            throw std::runtime_error
    }

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
 
    const std::string &getOutput() const;

        CgiHandler(const CgiHandler &other);
    CgiHandler &operator=(const CgiHandler &other);

    void setNonBlocking(int fd);
    void CgiHandler::buildEnvironment(const HttpRequest &request)
    {
        _environment.clear();
        _environment.push_back("GATEWAY_INTERFACE=CGI/1.1");
        _environment.push_back("REQUEST_METHOD=" + request.getMethod());
        _environment.push_back("QUERY_STRING=" + request.getQuery());
        _environment.push_back("CONTENT_TYPE=" + request.getHeader("Content-Type"));
        
        std::ostringstream length;
        length << request.getBody().size();

        _environment.push_back("CONTENT_LENGTH=" + length.str());
        _environment.push_back("SERVER_PROTOCOL=" + request.getVersion());
        _environment.push_back("SCRIPT_NAME" + request.getPath());
        _environment.push_back("SCRIPT_FILENAME=" + _scriptPath);

        _environment.push_back("SERVER_NAME=localhost");
        _environment.push_back("SERVER_PORT=8080");
        _environment.push_back("REMOTE_ADDR=127.0.0.1");
        _environment.push_back("PATH_INFO=");
        _environment.push_back("REDIRECT_STATUS=200");
    }
    char **CgiHandler::creatEnvp() const
    {
        char **envp = new char *[_environment.size() + 1];
        for (std::size_t i = 0; i < _environment.size(); ++i)
        {
            envp[i] = new char[_environment[i].size() + 1];

            std::strcpy(envp[i], _environment[i].c_str());
        }
        envp[_environment.size()] = NULL;

        return envp;
    }
    void CgiHandler::freeEnvp(char **envp) const
    {
        if (envp == NULL)
            return;
        for (std::size_t i = 0; envp[i] != NULL; ++i)
            delete[] envp[i];
        delete [] envp;
    }

    std::string CgiHandler::getDirectory(const std::string &path) const
    {
        std::string::size_type pos = path.find_last_of('/');
        if (pos == std::string::npos)
            return "."; //meancurrent directory.
        if (pos == 0)
            return "/";//mean root directory;
        return path.substr(0, pos);
    }

    std::string CgiHandler::getFileName(const std::string &path) const
    {
        std::string::size_type pos = path.find_last_of('/');

        if (pos == std::string::npos)
            return path;
        return path.substr(pos + 1);
    }