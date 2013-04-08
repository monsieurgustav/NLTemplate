#include "NLTemplate.h"

using namespace std;


NLTemplateTokenizer::NLTemplateTokenizer( const string & text ) :
peeking( false ),
pos( 0 ),
text( text ),
block( "^(\\{\\% block ([a-zA-Z0-9_]*) \\%\\})" ),
endblock( "^(\\{\\% endblock \\%\\})" ),
include( "^(\\{\\% include ([a-zA-Z0-9_\\.]*) \\%\\})" ),
var( "^(\\{\\{ ([a-zA-Z0-9_]*) \\}\\})" )
{
}


NLToken NLTemplateTokenizer::next() {
    if ( peeking ) {
        peeking = false;
        return peek;
    }
    
    NLToken token;
    token.value.clear();
    peek.value.clear();
    token.type = TOKEN_END;
    peek.type = TOKEN_END;
    
    int textpos = pos;
    int textlen = 0;
    smatch match;
    
a:
    if ( pos < text.length() ) {
        if ( regex_search( text.begin() + pos, text.end(), match, block ) ) {
            peek.type = TOKEN_BLOCK;
            peek.value = match[2];
            pos += match[1].length();
        } else if ( regex_search( text.begin() + pos, text.end(), match, endblock ) ) {
            peek.type = TOKEN_ENDBLOCK;
            pos += match[1].length();
        } else if ( regex_search( text.begin() + pos, text.end(), match, include ) ) {
            peek.type = TOKEN_INCLUDE;
            peek.value = match[2];
            pos += match[1].length();
        } else if ( regex_search( text.begin() + pos, text.end(), match, var ) ) {
            peek.type = TOKEN_VAR;
            peek.value = match[2];
            pos += match[1].length();
        } else {
            textlen ++;
            pos ++;
            peeking = true;
            goto a;
        }
    }
    
    if ( peeking ) {
        token.type = TOKEN_TEXT;
        token.value = text.substr( textpos, textlen );
        return token;
    }
    
    return peek;
}


const string NLTemplateDictionary::find( const string & name ) const {
    auto found = properties.find( name );
    if ( found != properties.end() ) {
        return found->second;
    }
    return "";
}


void NLTemplateDictionary::set( const string & name, const string & value ) {
    properties[ name ] = value;
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
    for ( auto fragment : fragments ) {
        delete fragment;
    }
}


NLTemplateFragment *NLTemplateNode::copy() const {
    NLTemplateNode *node = new NLTemplateNode();
    node->properties = properties;
    for ( auto fragment : fragments ) {
        node->fragments.push_back( fragment->copy() );
    }
    return node;
}


void NLTemplateNode::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    for ( auto fragment : fragments ) {
        fragment->render( output, *this );
    }
}



NLTemplateBlock & NLTemplateNode::block( const string & name ) const {
    for ( auto fragment : fragments ) {
        if ( fragment->isBlockNamed( name ) ) {
            return *dynamic_cast<NLTemplateBlock*>( fragment );
        }
    }
    throw 0;
}


NLTemplateBlock::NLTemplateBlock( const string & name ) : name( name ), enabled( true ), resized( false ) {
}


NLTemplateFragment *NLTemplateBlock::copy() const {
    NLTemplateBlock *block = new NLTemplateBlock( name );
    block->properties = properties;
    for ( auto fragment : fragments ) {
        block->fragments.push_back( fragment->copy() );
    }
    return block;
}


NLTemplateBlock::~NLTemplateBlock() {
    for ( auto node : nodes ) {
        delete node;
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

void NLTemplateBlock::repeat( int n ) {
    resized = true;
    for ( auto node : nodes ) {
        delete node;
    }
    nodes.clear();
    for ( int i=0; i < n; i++ ) {
        nodes.push_back( static_cast<NLTemplateNode*>( copy() ) );
    }
}


NLTemplateNode & NLTemplateBlock::operator[]( int index ) {
    return *nodes.at( index );
}


void NLTemplateBlock::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    if ( enabled ) {
        if ( resized ) {
            for ( auto node : nodes ) {
                node->render( output, *node );
            }
        } else {
            NLTemplateNode::render( output, *this );
        }
    }
}


void NLTemplateOutputString::print( const string & text ) {
    buf << text;
}


string NLTemplateLoaderFile::load( const string & name ) {
    stringstream source;
    source << ifstream( name ).rdbuf();
    return source.str();
}


NLTemplate::NLTemplate( NLTemplateLoader & loader ) : NLTemplateBlock( "main" ), loader( loader ) {
}


void NLTemplate::load_recursive( const string & name, vector<NLTemplateTokenizer> & files, vector<NLTemplateNode*> & nodes ) {
    files.emplace_back( loader.load( name ) );
    
    bool done = false;
    while( !done ) {
        NLToken token = files.back().next();
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
                load_recursive( token.value, files, nodes );
                break;
        }
    }
    
    files.pop_back();
}


void NLTemplate::clear() {
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


void NLTemplate::load( const string & name ) {
    clear();
    
    vector<NLTemplateNode*> stack;
    stack.push_back( this );
    
    vector<NLTemplateTokenizer> file_stack;
    
    load_recursive( name, file_stack, stack );
}


string NLTemplate::render() const {
    //return source.str();
    NLTemplateOutputString output;
    NLTemplateNode::render( output, *this );
    return output.buf.str();
}
