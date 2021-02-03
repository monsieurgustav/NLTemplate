#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include "NLTemplate.h"


using namespace std;
using namespace NL::Template;
using namespace NL::Template::Private;



enum {
    TOKEN_END,
    TOKEN_TEXT,
    TOKEN_BLOCK,
    TOKEN_ENDBLOCK,
    TOKEN_INCLUDE,
    TOKEN_VAR
};


static inline bool alphanum( const char c ) {
    return
    ( c >= 'a' && c <= 'z' ) ||
    ( c >= 'A' && c <= 'Z' ) ||
    ( c >= '0' && c <= '9' ) ||
    ( c == '_' ) ||
    ( c == '/' ) ||
    ( c == '.' );
}


static inline long match_var( const char *text, string & result ) {
    if (text[ 0 ] != '{' ||
        text[ 1 ] != '{' ||
        text[ 2 ] != ' ' )
    {
        return -1;
    }
    
    const char *var = text + 3;
    const char *cursor = var;
    
    while ( *cursor ) {
        if (cursor[ 0 ] == ' ' &&
            cursor[ 1 ] == '}' &&
            cursor[ 2 ] == '}' )
        {
            result = string( var, cursor - var );
            return cursor + 3 - text;
        }
        
        if ( !alphanum( *cursor ) ) {
            return -1;
        }
        
        cursor++;
    }
    
    return -1;
}


static inline long match_tag_with_param( const char *tag, const char *text, string & result ) {
    if (text[ 0 ] != '{' ||
        text[ 1 ] != '%' ||
        text[ 2 ] != ' ')
    {
        return -1;
    }

    long taglen = strlen( tag );
    if ( strncmp( text + 3, tag, taglen ) ) {
        return -1;
    }

    const char *param = text + 3 + taglen;
    
    if ( *param != ' ' ) {
        return -1;
    }
    
    param++;

    const char *cursor = param;

    while ( *cursor ) {
        if (cursor[ 0 ] == ' ' &&
            cursor[ 1 ] == '%' &&
            cursor[ 2 ] == '}' )
        {
            result = string( param, cursor - param );
            return cursor + 3 - text;
        }

        if ( !alphanum( *cursor ) ) {
            return -1;
        }
        
        cursor++;
    }
    
    return -1;
}


Tokenizer::Tokenizer( const string & text ) :
text_ptr( text.c_str() ),
len( text.length() ),
pos( 0 ),
peeking( false )
{
}


Token Tokenizer::next() {
    static const char * s_endblock = "{% endblock %}";
    static const char * s_block = "block";
    static const char * s_include = "include";
    static const long s_endblock_len = strlen( s_endblock );
    
    if ( peeking ) {
        peeking = false;
        return peek;
    }
    
    Token token;
    token.value.clear();
    peek.value.clear();
    token.type = TOKEN_END;
    peek.type = TOKEN_END;
    
    long textpos = pos;
    long textlen = 0;
    
a:
    if ( pos < len ) {
        long m = match_tag_with_param( s_block, text_ptr + pos, peek.value );
        if ( m > 0 ) {
            peek.type = TOKEN_BLOCK;
            pos += m;
        } else if ( !strncmp( s_endblock, text_ptr + pos, s_endblock_len ) ) {
            peek.type = TOKEN_ENDBLOCK;
            pos += s_endblock_len;
        } else if ( ( m = match_tag_with_param( s_include, text_ptr + pos, peek.value ) ) > 0 ) {
            peek.type = TOKEN_INCLUDE;
            pos += m;
        } else if ( ( m = match_var( text_ptr + pos, peek.value ) ) > 0 ) {
            peek.type = TOKEN_VAR;
            pos += m;
        } else {
            textlen ++;
            pos ++;
            peeking = true;
            goto a;
        }
    }

    if ( peeking ) {
        token.type = TOKEN_TEXT;
        token.value = string( text_ptr + textpos, textlen );
        return token;
    }

    return peek;
}


const string Dictionary::find( const string & name ) const {
    for ( auto const & property : properties ) {
        if ( property.first == name ) {
            return property.second;
        }
    }
    return "";
}


void Dictionary::set( const string & name, const string & value ) {
    for ( auto & property : properties ) {
        if ( property.first == name ) {
            property.second = value;
            return;
        }
    }
    properties.push_back( pair<string, string>( name, value ) );
}


Fragment::~Fragment() {
}


bool Fragment::isBlockNamed( const string & ) const {
    return false;
}



Text::Text( const string & text ) : text( text ) {
}


void Text::render( ostream & output, const Dictionary & ) const {
    output << text;
}


Fragment *Text::copy() const {
    return new Text( text );
}


Property::Property( const string & name ) : name( name ) {
}


void Property::render( ostream & output, const Dictionary & dictionary ) const {
    output << dictionary.find( name );
}


Fragment *Property::copy() const {
    return new Property( name );
}


Node::~Node() {
    for ( auto fragment : fragments ) {
        delete fragment;
    }
}


Fragment *Node::copy() const {
    Node *node = new Node();
    node->properties = properties;
    for ( auto const & fragment : fragments ) {
        node->fragments.push_back( fragment->copy() );
    }
    return node;
}


void Node::render( ostream & output, const Dictionary & ) const {
    for ( auto const & fragment : fragments ) {
        fragment->render( output, *this );
    }
}



Block & Node::block( const string & name ) const {
    for ( auto & fragment : fragments ) {
        if ( fragment->isBlockNamed( name ) ) {
            return *dynamic_cast<Block*>( fragment );
        }
    }
    throw "Block not found";
}


Block::Block( const string & name ) : name( name ), enabled( true ), resized( false ) {
}


Fragment *Block::copy() const {
    Block *block = new Block( name );
    block->properties = properties;
    for ( auto const & fragment : fragments ) {
        block->fragments.push_back( fragment->copy() );
    }
    return block;
}


Block::~Block() {
    for ( auto node : nodes ) {
        delete node;
    }
}


bool Block::isBlockNamed( const string & name ) const {
    return this->name == name;
}


void Block::enable() {
    enabled = true;
}


void Block::disable() {
    enabled = false;
}

void Block::repeat( size_t n ) {
    resized = true;
    for ( auto node : nodes ) {
        delete node;
    }
    nodes.clear();
    for ( size_t i=0; i < n; i++ ) {
        nodes.push_back( static_cast<Node*>( copy() ) );
    }
}


Node & Block::operator[]( size_t index ) {
    return *nodes.at( index );
}


void Block::render( ostream & output, const Dictionary & ) const {
    if ( enabled ) {
        if ( resized ) {
            for ( auto node : nodes ) {
                node->render( output, *node );
            }
        } else {
            Node::render( output, *this );
        }
    }
}


Loader::~Loader() {
}


Loader::Result LoaderFile::load( const string & name ) {
    ifstream input;
    input.open( name );
    
    if ( ! input.is_open() ) {
        return { false, nullptr, "Could not open file " + name };
    }
    
    std::string content( (std::istreambuf_iterator<char>( input ) ),
                         (std::istreambuf_iterator<char>() ) );

    return { true, content, "" };
}


void LoaderMemory::add( const std::string & name, const std::string & body ) {
    files.push_back( { name, body } );
}


Loader::Result LoaderMemory::load( const std::string & name ) {
    for ( auto & pair : files ) {
        if ( pair.first == name ) {
            return { true, pair.second, "" };
        }
    }

    return { false, "", name + " not found." };
}


Template::Template( Loader & loader ) : Block( "main" ), loader( loader ) {
}


void Template::load_recursive( const string & name, vector<Tokenizer> & files, vector<Node*> & nodes ) {
    auto loaded = loader.load( name );
    if ( !loaded.valid ) {
        // TODO pass loaded.error somewhere..
        return;
    }
    files.emplace_back( loaded.data );
    
    bool done = false;
    while( !done ) {
        Token token = files.back().next();
        switch ( token.type ) {
            case TOKEN_END:
                done = true;
                break;
            case TOKEN_BLOCK: {
                Block *block = new Block( token.value );
                nodes.back()->fragments.push_back( block );
                nodes.push_back( block );
            }
                break;
            case TOKEN_ENDBLOCK:
                nodes.pop_back();
                break;
            case TOKEN_VAR:
                nodes.back()->fragments.push_back( new Property( token.value ) );
                break;
            case TOKEN_TEXT:
                nodes.back()->fragments.push_back( new Text( token.value ) );
                break;
            case TOKEN_INCLUDE:
                load_recursive( token.value.c_str(), files, nodes );
                break;
        }
    }
    
    files.pop_back();
}


void Template::clear() {
    for ( auto fragment : fragments ) {
        delete fragment;
    }
    for ( auto node : nodes ) {
        delete node;
    }
    nodes.clear();
    fragments.clear();
    properties.clear();
}


void Template::load( const string & name ) {
    clear();
    
    vector<Node*> stack;
    stack.push_back( this );
    
    vector<Tokenizer> file_stack;
    
    load_recursive( name, file_stack, stack );
}


void Template::render( ostream & output ) const {
    Node::render( output, *this );
}

