#include "Utils.hpp"

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
                static_cast<unsigned char>(result[i]))
        );
    }
    return result;
}