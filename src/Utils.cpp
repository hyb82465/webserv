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
        if (result[i] >= 'A' && result[i] <= 'Z')
            result[i] = static_cast<char>(result[i] - 'A' + 'a');
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

    char c1 = path[i + 1];
    char c2 = path[i + 2];

    bool hex1 = (c1 >= '0' && c1 <= '9') ||
                (c1 >= 'a' && c1 <= 'f') ||
                (c1 >= 'A' && c1 <= 'F');

    bool hex2 = (c2 >= '0' && c2 <= '9') ||
                (c2 >= 'a' && c2 <= 'f') ||
                (c2 >= 'A' && c2 <= 'F');

    if (!hex1 || !hex2)
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