#include "NLTemplate.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


using namespace std;


static inline bool alphanum( const char c ) {
    return
    ( c >= 'a' && c <= 'z' ) ||
    ( c >= 'A' && c <= 'Z' ) ||
    ( c >= '0' && c <= '9' ) ||
    ( c == '_' ) ||
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
    
    long taglen = strlen( tag );
    
    if (text[ 0 ] != '{' ||
        text[ 1 ] != '%' ||
        text[ 2 ] != ' ' ||
        strncmp( text + 3, tag, taglen ) )
    {
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


NLTemplateTokenizer::NLTemplateTokenizer( const char *text ) :
text( text ),
len( strlen( text ) ),
pos( 0 ),
peeking( false )
{
}


NLTemplateTokenizer::~NLTemplateTokenizer() {
    free( (void*) text );
}


NLToken NLTemplateTokenizer::next() {
    static const char * s_endblock = "{% endblock %}";
    static const char * s_block = "block";
    static const char * s_include = "include";
    static const long s_endblock_len = strlen( s_endblock );
    
    if ( peeking ) {
        peeking = false;
        return peek;
    }
    
    NLToken token;
    token.value.clear();
    peek.value.clear();
    token.type = TOKEN_END;
    peek.type = TOKEN_END;
    
    long textpos = pos;
    long textlen = 0;
    
a:
    if ( pos < len ) {
        long m = match_tag_with_param( s_block, text + pos, peek.value );
        if ( m > 0 ) {
            peek.type = TOKEN_BLOCK;
            pos += m;
        } else if ( !strncmp( s_endblock, text + pos, s_endblock_len ) ) {
            peek.type = TOKEN_ENDBLOCK;
            pos += s_endblock_len;
        } else if ( ( m = match_tag_with_param( s_include, text + pos, peek.value ) ) > 0 ) {
            peek.type = TOKEN_INCLUDE;
            pos += m;
        } else if ( ( m = match_var( text + pos, peek.value ) ) > 0 ) {
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
        token.value = string( text + textpos, textlen );
        return token;
    }

    return peek;
}


const string NLTemplateDictionary::find( const string & name ) const {
    for ( size_t i=0; i < properties.size(); i++ ) {
        if ( properties[ i ].first == name ) {
            return properties[ i ].second;
        }
    }
    return "";
}


void NLTemplateDictionary::set( const string & name, const string & value ) {
    for ( size_t i=0; i < properties.size(); i++ ) {
        if ( properties[ i ].first == name ) {
            properties[ i ].second = value;
            return;
        }
    }
    properties.push_back( pair<string, string>( name, value ) );
}


NLTemplateFragment::~NLTemplateFragment() {
}


bool NLTemplateFragment::isBlockNamed( const string & name ) const {
    return false;
}



NLTemplateText::NLTemplateText( const string & text ) : text( text ) {
}


void NLTemplateText::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    output.print( text );
}


NLTemplateFragment *NLTemplateText::copy() const {
    return new NLTemplateText( text );
}


NLTemplateProperty::NLTemplateProperty( const string & name ) : name( name ) {
}


void NLTemplateProperty::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    output.print( dictionary.find( name ) );
}


NLTemplateFragment *NLTemplateProperty::copy() const {
    return new NLTemplateProperty( name );
}


NLTemplateNode::~NLTemplateNode() {
    for ( size_t i=0; i < fragments.size(); i++ ) {
        delete fragments[ i ];
    }
}


NLTemplateFragment *NLTemplateNode::copy() const {
    NLTemplateNode *node = new NLTemplateNode();
    node->properties = properties;
    for ( size_t i=0; i < fragments.size(); i++ ) {
        node->fragments.push_back( fragments[ i ]->copy() );
    }
    return node;
}


void NLTemplateNode::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    for ( size_t i=0; i < fragments.size(); i++ ) {
        fragments[ i ]->render( output, *this );
    }
}



NLTemplateBlock & NLTemplateNode::block( const string & name ) const {
    for ( size_t i=0; i < fragments.size(); i++ ) {
        if ( fragments[ i ]->isBlockNamed( name ) ) {
            return *dynamic_cast<NLTemplateBlock*>( fragments[ i ] );
        }
    }
    throw 0;
}


NLTemplateBlock::NLTemplateBlock( const string & name ) : name( name ), enabled( true ), resized( false ) {
}


NLTemplateFragment *NLTemplateBlock::copy() const {
    NLTemplateBlock *block = new NLTemplateBlock( name );
    block->properties = properties;
    for ( size_t i=0; i < fragments.size(); i++ ) {
        block->fragments.push_back( fragments[ i ]->copy() );
    }
    return block;
}


NLTemplateBlock::~NLTemplateBlock() {
    for ( size_t i=0; i < nodes.size(); i++ ) {
        delete nodes[ i ];
    }
}


bool NLTemplateBlock::isBlockNamed( const string & name ) const {
    return this->name == name;
}


void NLTemplateBlock::enable() {
    enabled = true;
}


void NLTemplateBlock::disable() {
    enabled = false;
}

void NLTemplateBlock::repeat( size_t n ) {
    resized = true;
    for ( size_t i=0; i < nodes.size(); i++ ) {
        delete nodes[ i ];
    }
    nodes.clear();
    for ( size_t i=0; i < n; i++ ) {
        nodes.push_back( static_cast<NLTemplateNode*>( copy() ) );
    }
}


NLTemplateNode & NLTemplateBlock::operator[]( size_t index ) {
    return *nodes.at( index );
}


void NLTemplateBlock::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    if ( enabled ) {
        if ( resized ) {
            for ( size_t i=0; i < nodes.size(); i++ ) {
                nodes[ i ]->render( output, *nodes[ i ] );
            }
        } else {
            NLTemplateNode::render( output, *this );
        }
    }
}


NLTemplateOutput::~NLTemplateOutput() {
}


void NLTemplateOutputString::print( const string & text ) {
    buf << text;
}


void NLTemplateOutputStdout::print( const std::string &text ) {
    cout << text;
}


const char * NLTemplateLoaderFile::load( const char *name ) {
    FILE *f = fopen( name, "rb" );
    fseek( f, 0, SEEK_END );
    long len = ftell( f );
    fseek( f, 0, SEEK_SET );
    char *buffer = (char*) malloc( len + 1 );
    fread( (void*) buffer, len, 1, f );
    fclose( f );
    buffer[ len ] = 0;
    return buffer;
}


NLTemplate::NLTemplate( NLTemplateLoader & loader ) : NLTemplateBlock( "main" ), loader( loader ) {
}


void NLTemplate::load_recursive( const char *name, vector<NLTemplateTokenizer*> & files, vector<NLTemplateNode*> & nodes ) {
    NLTemplateTokenizer *tokenizer = new NLTemplateTokenizer( loader.load( name ) );
    files.push_back( tokenizer );
    
    bool done = false;
    while( !done ) {
        NLToken token = files.back()->next();
        switch ( token.type ) {
            case TOKEN_END:
                done = true;
                break;
            case TOKEN_BLOCK: {
                NLTemplateBlock *block = new NLTemplateBlock( token.value );
                nodes.back()->fragments.push_back( block );
                nodes.push_back( block );
            }
                break;
            case TOKEN_ENDBLOCK:
                nodes.pop_back();
                break;
            case TOKEN_VAR:
                nodes.back()->fragments.push_back( new NLTemplateProperty( token.value ) );
                break;
            case TOKEN_TEXT:
                nodes.back()->fragments.push_back( new NLTemplateText( token.value ) );
                break;
            case TOKEN_INCLUDE:
                load_recursive( token.value.c_str(), files, nodes );
                break;
        }
    }
    
    delete files.back();
    files.pop_back();
}


void NLTemplate::clear() {
    for ( size_t i=0; i < fragments.size(); i++ ) {
        delete fragments[ i ];
    }
    for ( size_t i=0; i < nodes.size(); i++ ) {
        delete nodes[ i ];
    }
    nodes.clear();
    fragments.clear();
    properties.clear();
}


void NLTemplate::load( const char *name ) {
    clear();
    
    vector<NLTemplateNode*> stack;
    stack.push_back( this );
    
    vector<NLTemplateTokenizer*> file_stack;
    
    load_recursive( name, file_stack, stack );
}


void NLTemplate::render( NLTemplateOutput & output ) const {
    NLTemplateNode::render( output, *this );
}
