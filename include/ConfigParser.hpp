#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

# include <string>
# include <vector>

class TokenStream;
class ServerConfig;

class ConfigParser
{
private:
    ConfigParser(const ConfigParser &other);
    ConfigParser &operator=(const ConfigParser &other);

    std::size_t parseBodySizeValue(TokenStream &tokens);

    ServerConfig parseServer(TokenStream &tokens);
    void parseListen(TokenStream &tokens, ServerConfig &config);
    ListenConfig parseListenValue(const std::string &value);
    void parseRoot(TokenStream &tokens, ServerConfig &config);
    void parseIndex(TokenStream &tokens, ServerConfig &config);
    void parseErrorPage(TokenStream &tokens, ServerConfig &config);
     void parseServerClientMaxBodySize(TokenStream &tokens, ServerConfig &config);

    LocationConfig parseLocation(TokenStream &tokens);
        void parseLocationRoot(TokenStream &tokens, LocationConfig &location);
    void parseMethods(TokenStream &tokens, LocationConfig &location);
    void parseAutoindex(TokenStream &tokens, LocationConfig &location);
    void parseUploadStore(TokenStream &tokens, LocationConfig &location);
    void parseRedirect(TokenStream &tokens, LocationConfig &location);
    void parseLocationIndex(TokenStream &tokens, LocationConfig &location);
    void parseCgi(TokenStream &tokens, LocationConfig &location);
    void parseLocationClientMaxBodySize(TokenStream &tokens, LocationConfig &location);
public:
    ConfigParser();
    ~ConfigParser();

    std::vector<ServerConfig> parse(const std::string &filename);
};

#endif