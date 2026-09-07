#include "CgiHandler.hpp"

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <signal.h>

CgiHandler::CgiHandler(
    int clientFd,
    const std::string &executable,
    const std::string &scriptPath,
    int serverPort,
    const std::string &remoteAddr,
    const std::string &serverName)
    : _clientFd(clientFd), _pid(-1),
      _stdinFd(-1), _stdoutFd(-1),
      _stdinOpen(false), _stdoutOpen(false),
      _executable(executable), _scriptPath(scriptPath),
      _serverPort(serverPort), 
      _remoteAddr(remoteAddr),_serverName(serverName),
      _requestBody(""),
      _bodyOffset(0),
      _output(""),
      _environment(),
      _exitStatus(-1),
      _childFinished(false),
      _lastActivity(0)
{}

CgiHandler::~CgiHandler()
{
    // if (_stdinFd != -1)
    //     close(_stdinFd);
    // if (_stdoutFd != -1)
    //     close(_stdoutFd);
    // if (_pid > 0)
    // {
    //     kill(_pid, SIGKILL);
    //     int status;

    //     waitpid(_pid, &status, 0);
    //     _pid = -1;
    // }
    killChild();
    closeInput();
    closeOutput();
}

void CgiHandler::start(HttpRequest &request)
{
    int inputPipe[2];
    int outputPipe[2];

    if (pipe(inputPipe) == -1)
        throw std::runtime_error("inputpipe pipe failed");
    if (pipe(outputPipe) == -1)
    {
        close(inputPipe[1]);
        close(inputPipe[0]);
        throw std::runtime_error("outputpipe pipe failed");
    }

    buildEnvironment(request);

    request.swapBody(_requestBody);
    _bodyOffset = 0; // no data to CGI.

    setNonBlocking(inputPipe[1]);  // use to write data to CGI
    setNonBlocking(outputPipe[0]); // to read data from CGI

    _pid = fork();

    if (_pid == -1)
    {
        close(inputPipe[1]);
        close(inputPipe[0]);
        close(outputPipe[1]);
        close(outputPipe[0]);
        throw std::runtime_error("fork failed");
    }

    if (_pid == 0) // child
    {
        close(inputPipe[1]);
        close(outputPipe[0]);

        if (dup2(inputPipe[0], STDIN_FILENO) == -1) // data read form pipe as CGI stdin
            _exit(1);
        if (dup2(outputPipe[1], STDOUT_FILENO) == -1)
            _exit(1);
        close(inputPipe[0]);
        close(outputPipe[1]);

        std::string directory = getDirectory(_scriptPath);
        if (chdir(directory.c_str()) == -1)
            _exit(1);
        char **envp = creatEnvp();
        char *argv[3]; // CGI run arguments
        argv[0] = const_cast<char *>(_executable.c_str());
        std::string fileName = getFileName(_scriptPath);
        argv[1] = const_cast<char *>(fileName.c_str());
        argv[2] = NULL;
        execve(_executable.c_str(), argv, envp);

        freeEnvp(envp);
        _exit(1);
    }
    // father
    _lastActivity = std::time(NULL);
    close(inputPipe[0]);
    close(outputPipe[1]);

    _stdinFd = inputPipe[1];
    _stdoutFd = outputPipe[0];

    _stdinOpen = true;
    _stdoutOpen = true;
}

bool CgiHandler::writeBody()
{
    if (!_stdinOpen)
        return true;
    if (_bodyOffset >= _requestBody.size())
    {
        closeInput();
        return true;
    }

    ssize_t bytes = write(_stdinFd, _requestBody.c_str() + _bodyOffset, _requestBody.size() - _bodyOffset);
    if (bytes > 0)
    {
        std::time_t now = std::time(NULL);
        if (now != static_cast<std::time_t>(-1))
            _lastActivity = now;
        _bodyOffset += static_cast<std::size_t>(bytes);
        if (_bodyOffset >= _requestBody.size())
        {
            closeInput();
            return true;
        }
        return false;
    }
    else if (bytes == 0)
    {
        closeInput();
        return true;
    }
    // real write error
    else
    {
        closeInput();
        return true;
    }
}

bool CgiHandler::readOutput()
{
    if (!_stdoutOpen)
        return true;
    char buffer[4096];
    ssize_t bytes = read(_stdoutFd, buffer, sizeof(buffer));
    if (bytes > 0)
    {
        std::time_t now = std::time(NULL);
        if (now != static_cast<std::time_t>(-1))
            _lastActivity = now;
        _output.append(buffer, static_cast<std::size_t>(bytes));
        return false;
    }
    else if (bytes == 0)
    {
        closeOutput();
        return true;
    }
    else
    {
        closeOutput();
        return true;
    }
}

void CgiHandler::closeInput()
{
    if (_stdinFd != -1)
    {
        close(_stdinFd);
        _stdinFd = -1;
    }
    _stdinOpen = false;
    std::string empty;
    _requestBody.swap(empty);
}

void CgiHandler::closeOutput()
{
    if (_stdoutFd != -1)
    {
        close(_stdoutFd);
        _stdoutFd = -1;
    }
    _stdoutOpen = false;
}

int CgiHandler::getClientFd() const
{
    return _clientFd;
}

int CgiHandler::getStdinFd() const
{
    return _stdinFd;
}
int CgiHandler::getStdoutFd() const
{
    return _stdoutFd;
}

pid_t CgiHandler::getPid() const
{
    return _pid;
}

bool CgiHandler::isStdinOpen() const
{
    return _stdinOpen;
}
bool CgiHandler::isStdoutOpen() const
{
    return _stdoutOpen;
}

const std::string &CgiHandler::getOutput() const
{
    return _output;
}

void CgiHandler::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        throw std::runtime_error("fcntl(F_GETFL) failed");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl(F_SETFL) failed");
}

void CgiHandler::buildEnvironment(const HttpRequest &request)
{
    _environment.clear();

    const std::string &path = request.getPath();
    const std::string &query = request.getQuery();

    std::string requestUri = path;
    if (!query.empty())
    {
        requestUri += "?";
        requestUri += query;
    }

    std::ostringstream contentLength;
    contentLength << request.getBody().size();

    std::ostringstream serverPort;
    serverPort << _serverPort;

    // CGI basic information
    _environment.push_back("GATEWAY_INTERFACE=CGI/1.1");
    _environment.push_back("REDIRECT_STATUS=200");

    // HTTP request information

    _environment.push_back("REQUEST_METHOD=" + request.getMethod());
    _environment.push_back("SERVER_PROTOCOL=" + request.getVersion());
    _environment.push_back("REQUEST_URI=" + requestUri);
    _environment.push_back("QUERY_STRING=" + request.getQuery());

    // CGI script information
    _environment.push_back("SCRIPT_NAME=" + path);
    _environment.push_back("SCRIPT_FILENAME=" + _scriptPath);
    _environment.push_back("PATH_INFO=" + path);

    // request body information
    _environment.push_back("CONTENT_TYPE=" + request.getHeader("content-type"));
    _environment.push_back("CONTENT_LENGTH=" + contentLength.str());

    // server and client information
    _environment.push_back("SERVER_NAME=" + _serverName);
    _environment.push_back("SERVER_PORT=" + serverPort.str());
    _environment.push_back("REMOTE_ADDR=" + _remoteAddr);

    const std::map<std::string, std::string> &headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
        it != headers.end();
        ++it)
    {
        if (it->first == "content-type" || it->first == "content-length")
            continue;
        std::string variableName = "HTTP_";
        for (std::size_t i = 0; i < it->first.size(); ++i)
        {
            char c = it->first[i];
            if (c == '-')
                variableName += '_';
            else if (c >= 'a' && c <= 'z')
                variableName += static_cast<char>(c - 'a' + 'A');
            else
                variableName += c;
        }
        _environment.push_back(variableName + "=" + it->second);
    }
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
    delete[] envp;
}

std::string CgiHandler::getDirectory(const std::string &path) const
{
    std::string::size_type pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return "."; // meancurrent directory.
    if (pos == 0)
        return "/"; // mean root directory;
    return path.substr(0, pos);
}

std::string CgiHandler::getFileName(const std::string &path) const
{
    std::string::size_type pos = path.find_last_of('/');

    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

bool CgiHandler::waitForChild()
{
    if (_pid <= 0)
        return true;

    int status;

    pid_t result = waitpid(_pid, &status, WNOHANG);

    if (result == 0)
        return false;

    if (result == _pid)
    {
        _pid = -1;
        _childFinished = true;
        _exitStatus = status;
        return true;
    }

    return false;
}

bool CgiHandler::hasTimedOut(int timeoutSeconds) const
{
    if (_pid <= 0)
        return false;
    if (_lastActivity == static_cast<std::time_t>(-1))
        return false;
    std::time_t now = std::time(NULL);
    if (now == static_cast<std::time_t>(-1))
        return false;
    return (now - _lastActivity) >= timeoutSeconds;
}

void CgiHandler::killChild()
{
    if (_pid <= 0)
        return;

    kill(_pid, SIGKILL);

    int status;

    pid_t result = waitpid(_pid, &status, 0);

    if (result == _pid)
    {
        _exitStatus = status;
        _childFinished = true;
        _pid = -1;
    }
}

bool CgiHandler::isChildSuccess() const
{
    if (!_childFinished)
        return false;
    return WIFEXITED(_exitStatus) && WEXITSTATUS(_exitStatus) == 0;
}