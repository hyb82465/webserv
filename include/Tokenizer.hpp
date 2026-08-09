#ifndef Tokenizer_hpp
#define Tokenizer_hpp
#include <string>
#include <vector>

class Tokenizer
{
    public:
        std::vector<std::string> getTokens(const std::string &fileContent);
};

#endif