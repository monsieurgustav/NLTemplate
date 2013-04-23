#include "NLTemplate.h"


using namespace std;



NLTemplateTokenizer::NLTemplateTokenizer() :
peeking( false ),
pos( 0 ),
block( "^(\\{\\% block ([a-zA-Z0-9_]*) \\%\\}).*" ),
endblock( "^(\\{\\% endblock \\%\\}).*" ),
include( "^(\\{\\% include ([a-zA-Z0-9_\\.]*) \\%\\}).*" ),
var( "^(\\{\\{ ([a-zA-Z0-9_]*) \\}\\}).*" )
{
}


void NLTemplateTokenizer::setText( const string &text ) {
    this->text = text;
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
    

    const char *src = this->text.c_str();
    
a:
    if ( pos < text.length() ) {
        if ( block.match( src + pos ) ) {
            peek.type = TOKEN_BLOCK;
            peek.value = block[ 2 ];
            pos += block.matchlen( 1 );
        } else if ( endblock.match( src + pos ) ) {
            peek.type = TOKEN_ENDBLOCK;
            pos += endblock.matchlen( 1 );
        } else if ( include.match( src + pos ) ) {
            peek.type = TOKEN_INCLUDE;
            peek.value = include[ 2 ];
            pos += include.matchlen( 1 );
        } else if ( var.match( src + pos ) ) {
            peek.type = TOKEN_VAR;
            peek.value = var[ 2 ];
            pos += var.matchlen( 1 );
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
    for ( int i=0; i < properties.size(); i++ ) {
        if ( properties[ i ].first == name ) {
            return properties[ i ].second;
        }
    }
    return "";
}


void NLTemplateDictionary::set( const string & name, const string & value ) {
    for ( int i=0; i < properties.size(); i++ ) {
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
    for ( int i=0; i < fragments.size(); i++ ) {
        delete fragments[ i ];
    }
}


NLTemplateFragment *NLTemplateNode::copy() const {
    NLTemplateNode *node = new NLTemplateNode();
    node->properties = properties;
    for ( int i=0; i < fragments.size(); i++ ) {
        node->fragments.push_back( fragments[i]->copy() );
    }
    return node;
}


void NLTemplateNode::render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const {
    for ( int i=0; i < fragments.size(); i++ ) {
        fragments[i]->render( output, *this );
    }
}



NLTemplateBlock & NLTemplateNode::block( const string & name ) const {
    for ( int i=0; i < fragments.size(); i++ ) {
        if ( fragments[i]->isBlockNamed( name ) ) {
            return *dynamic_cast<NLTemplateBlock*>( fragments[i] );
        }
    }
    throw 0;
}


NLTemplateBlock::NLTemplateBlock( const string & name ) : name( name ), enabled( true ), resized( false ) {
}


NLTemplateFragment *NLTemplateBlock::copy() const {
    NLTemplateBlock *block = new NLTemplateBlock( name );
    block->properties = properties;
    for ( int i=0; i < fragments.size(); i++ ) {
        block->fragments.push_back( fragments[i]->copy() );
    }
    return block;
}


NLTemplateBlock::~NLTemplateBlock() {
    for ( int i=0; i < nodes.size(); i++ ) {
        delete nodes[i];
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
        delete nodes[i];
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
            for ( int i=0; i < nodes.size(); i++ ) {
                nodes[i]->render( output, *nodes[i] );
            }
        } else {
            NLTemplateNode::render( output, *this );
        }
    }
}


void NLTemplateOutputString::print( const string & text ) {
    buf << text;
}


string NLTemplateLoaderFile::load( const char *name ) {
    stringstream source;
    source << ifstream( name ).rdbuf();
    return source.str();
}


NLTemplate::NLTemplate( NLTemplateLoader & loader ) : NLTemplateBlock( "main" ), loader( loader ) {
}


void NLTemplate::load_recursive( const char *name, vector<NLTemplateTokenizer*> & files, vector<NLTemplateNode*> & nodes ) {
    NLTemplateTokenizer *tokenizer = new NLTemplateTokenizer();
    tokenizer->setText( loader.load( name ) );
    files.push_back( tokenizer );
    
    bool done = false;
    while( !done ) {
        NLToken token = files.back()->next();
	printf( "token type %d value %s\n", token.type, token.value.c_str() );
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
    for ( int i=0; i < fragments.size(); i++ ) {
        delete fragments[i];
    }
    for ( int i=0; i < nodes.size(); i++ ) {
        delete nodes[i];
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


string NLTemplate::render() const {
    //return source.str();
    NLTemplateOutputString output;
    NLTemplateNode::render( output, *this );
    return output.buf.str();
}
