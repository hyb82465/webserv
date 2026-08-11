#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>

class Utils
{
public:
    static std::string trim(const std::string &str);
    static std::string toLower(const std::string &str);
};

#endif