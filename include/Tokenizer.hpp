#ifndef Tokenizer_hpp
#define Tokenizer_hpp
#include <string>
#include <vector>

class Tokenizer
{
    public:
        std::vector<std::string> tokenize(const std::string &fileContent);
};

#endif