#include "Tokenizer.hpp"
std::vector<std::string> Tokenizer::getTokens(const std::string &fileContent)
{
    std::vector<std::string> tokens;
    std::string token;

    bool inComment = false;
    for (size_t i = 0; i < fileContent.size(); ++i)
    {
        char c = fileContent[i];

        if (inComment)
        {
            if (c == '\n')
            {
                inComment = false;
            }
            continue;
        }

        if (c == '#')
        {
            inComment = true;
            continue;
        }

        if (isspace(c))
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            continue;
        }
        if (c == ';' || c == '{' || c == '}')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, c));

            continue;
        }
        else
        {
            token += c;
        }
    }
    if (!token.empty())
    {
        tokens.push_back(token);
    }
    return tokens;
}   