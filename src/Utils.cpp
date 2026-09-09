#include "Utils.hpp"
#include <sstream>
#include <iomanip>
#include <cctype>

std::string Utils::trim(const std::string &line)
{
    size_t start = line.find_first_not_of(" \t\n\r");
    size_t end = line.find_last_not_of(" \t\n\r");

    if (start == std::string::npos || end == std::string::npos)
        return "";

    return line.substr(start, end - start + 1);
}

std::string Utils::toLower(const std::string &str)
{
    std::string result = str;
    for (std::size_t i = 0; i < result.size(); ++i)
    {
        result[i] = static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(result[i])));
    }
    return result;
}

std::string Utils::sizetToString(std::size_t n)
{
    std::stringstream ss;
    ss << n;
    return ss.str();
}

bool Utils::decodeUri(std::string &path)
{
    std::string decoded;
    for (std::size_t i = 0; i < path.size(); ++i)
    {
        if (path[i] != '%')
        {
            decoded += path[i];
            continue;
        }
        if (i + 2 >= path.size())
            return false;
        if (!std::isxdigit(static_cast<unsigned char>(path[i + 1])) || !std::isxdigit(static_cast<unsigned char>(path[i + 2])))
            return false;
        std::string hex = path.substr(i + 1, 2);
        unsigned int value;
        std::stringstream ss(hex);
        if (!(ss >> std::hex >> value))
            return false;
        if (value == 0)
            return false;
        decoded += static_cast<char>(value);
        i += 2;
    }
    path = decoded;
    return true;
}