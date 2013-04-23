#pragma once


#include <string>
#include <regex.h>



class NLRegex {
private:
    regex_t r;
    regmatch_t *groups;
    std::string text;
    
public:
    
    NLRegex( const char *pattern );
    ~NLRegex();
    bool match( const char *src );
    size_t ngroups() const;
    bool matched( unsigned int i ) const;
    size_t matchlen( unsigned int i ) const;
    std::string operator[]( unsigned int i ) const;
};
