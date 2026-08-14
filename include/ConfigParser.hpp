#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <string>
#include <fstream>
#include <vector>
#include "ServerConfig.hpp"
#include "TokenStream.hpp"

class ConfigParser
{
    public:
        ConfigParser();
        ~ConfigParser();
        //ConfigParser(const ConfigParser &other);
        //ConfigParser &operator=(const ConfigParser &other);

        std::vector<ServerConfig> parse(const std::string &filename);
    
    private:    
        ServerConfig parseServer(TokenStream &tokens);
        
        void parseListen(TokenStream &tokens, ServerConfig &config);
        ListenConfig parseListenValue(const std::string &value);
        void parseRoot(TokenStream &tokens, ServerConfig &config);
        void parseIndex(TokenStream &tokens, ServerConfig &config);
        void parseClientMaxBodySize(TokenStream &tokens, ServerConfig &config);
        void parseErrorPage(TokenStream &tokens, ServerConfig &config);

        LocationConfig parseLocation(TokenStream &tokens);
        void parseMethods(TokenStream &tokens, LocationConfig &location);
        void parseLocationRoot(TokenStream &tokens, LocationConfig &location);
        void parseAutoindex(TokenStream &tokens, LocationConfig &location);
        void parseUploadStore(TokenStream &tokens, LocationConfig &location);
        void parseRedirect(TokenStream &tokens, LocationConfig &location);
    };

#endif