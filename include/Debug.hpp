#ifndef DEBUG_HPP
# define DEBUG_HPP

# include <iostream>

# ifndef WEBSERV_DEBUG
#  define WEBSERV_DEBUG 0
# endif

# define DEBUG_LOG(message)                     \
    do                                          \
    {                                           \
        if (WEBSERV_DEBUG)                      \
            std::cout << message << std::endl;  \
    } while (0)

#endif