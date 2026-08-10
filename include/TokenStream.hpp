#ifndef TOKENSTREAM_HPP
# define TOKENSTREAM_HPP    

#include <string>
#include <vector>

class TokenStream
{
    public:
        TokenStream(const std::vector<std::string> &tokens); 
        
        bool hasNext() const;
        std::string peek() const;
        std::string consume();
        bool match(const std::string &expected);
        void expect(const std::string &expected);

    private:
        std::vector<std::string> _tokens;
        size_t _currentIndex;
};

#endif