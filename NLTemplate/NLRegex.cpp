#include "NLRegex.h"



NLRegex::NLRegex( const char *pattern ) {
    int result = regcomp( &r, pattern, REG_EXTENDED );
    
    if ( result ) {
        char err[200];
        regerror( result, &r, err, 200 );
        printf( "could not compile regex: %s\n", err );
    }
    
    groups = new regmatch_t[ r.re_nsub + 1 ];
}


NLRegex::~NLRegex() {
    delete[] groups;
    regfree( &r );
}


bool NLRegex::match( const char *src ) {
    text.assign( src );
    return !regexec(&r, text.c_str(), r.re_nsub + 1, groups, 0);
}


size_t NLRegex::ngroups() const {
    return r.re_nsub + 1;
}


bool NLRegex::matched( unsigned int i ) const {
    return groups[ i ].rm_so != (size_t) - 1;
}


size_t NLRegex::matchlen( unsigned int i ) const {
    if ( groups[ i ].rm_so != (size_t) - 1 ) {
        return groups[ i ].rm_eo - groups[ i ].rm_so;
    }
    return 0;
}


std::string NLRegex::operator[]( unsigned int i ) const {
    return text.substr( groups[ i ].rm_so, groups[ i ].rm_eo - groups[ i ].rm_so );
}
