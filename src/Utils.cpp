#include "Utils.hpp"

std::string Utils::trim(const std::string &line)
{
    std::string trimmedLine = line;
    size_t start = trimmedLine.find_first_not_of(" \t\n\r");
    size_t end = trimmedLine.find_last_not_of(" \t\n\r");

    if (start == std::string::npos || end == std::string::npos)
        return "";

    return trimmedLine.substr(start, end - start + 1);
}
