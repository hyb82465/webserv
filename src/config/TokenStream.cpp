#include "TokenStream.hpp"

#include <stdexcept>

TokenStream::TokenStream(const std::vector<std::string> &tokens):_tokens(tokens), _currentIndex(0) {}

bool TokenStream::hasNext() const
{
    return _currentIndex < _tokens.size();
}

std::string TokenStream::peek() const
{
    if (!hasNext())
        throw std::runtime_error("No more tokens to peek");
    return _tokens[_currentIndex];
}   

std::string TokenStream::consume()
{
    if (!hasNext())
        throw std::runtime_error("No more tokens to consume");
    return _tokens[_currentIndex++];
}

bool TokenStream::match(const std::string &expected)
{
    if (hasNext() && peek() == expected)
    {
        consume();
        return true;
    }
    return false;
}

void TokenStream::expect(const std::string &expected)
{
    if (!match(expected))
        throw std::runtime_error("Expected token: " + expected + ", but got: " + (hasNext() ? peek() : "end of stream"));
}