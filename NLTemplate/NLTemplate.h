#pragma once


#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include "NLRegex.h"




enum {
    TOKEN_END,
    TOKEN_TEXT,
    TOKEN_BLOCK,
    TOKEN_ENDBLOCK,
    TOKEN_INCLUDE,
    TOKEN_VAR,
};


struct NLToken {
    int type;
    std::string value;
};


class NLTemplateTokenizer {
protected:
    std::string text;
    NLRegex block;
    NLRegex endblock;
    NLRegex include;
    NLRegex var;

    int pos;
    NLToken peek;
    bool peeking;
public:
    NLTemplateTokenizer();
    void setText( const std::string & text );
    NLToken next();
};


class NLTemplateDictionary {
public:
    std::vector<std::pair<std::string, std::string> > properties;
    
public:
    const std::string find( const std::string & name ) const;
    void set( const std::string & name, const std::string & value );
};


class NLTemplateOutput {
public:
    virtual void print( const std::string & text ) = 0;
};


class NLTemplateFragment {
public:
    virtual void render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const = 0;
    virtual ~NLTemplateFragment();
    virtual NLTemplateFragment *copy() const = 0;
    virtual bool isBlockNamed( const std::string & name ) const;
};


class NLTemplateText : public NLTemplateFragment {
protected:
    const std::string text;
    
public:
    NLTemplateText( const std::string & text );
    void render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const;
    NLTemplateFragment *copy() const;
};


class NLTemplateProperty : public NLTemplateFragment {
protected:
    const std::string name;
    
public:
    NLTemplateProperty( const std::string & name );
    void render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const;
    NLTemplateFragment *copy() const;
};


class NLTemplateBlock;


class NLTemplateNode : public NLTemplateFragment, public NLTemplateDictionary {
public:
    std::vector<NLTemplateFragment*> fragments;
    
public:
    ~NLTemplateNode();
    NLTemplateFragment *copy() const;
    void render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const;
    NLTemplateBlock & block( const std::string & name ) const;
};


class NLTemplateBlock : public NLTemplateNode {
protected:
    const std::string name;
    bool enabled;
    bool resized;
    std::vector<NLTemplateNode*> nodes;
    
public:
    NLTemplateBlock( const std::string & name );
    NLTemplateFragment *copy() const;
    ~NLTemplateBlock();
    bool isBlockNamed( const std::string & name ) const;
    void enable();
    void disable();
    void repeat( size_t n );
    NLTemplateNode & operator[]( size_t index );
    void render( NLTemplateOutput & output, const NLTemplateDictionary & dictionary ) const;
};



class NLTemplateOutputString : public NLTemplateOutput {
public:
    std::stringstream buf;
    
public:
    void print( const std::string & text );
};


class NLTemplateLoader {
public:
    virtual std::string load( const char *name ) = 0;
};


class NLTemplateLoaderFile : public NLTemplateLoader {
public:
    std::string load( const char *name );
};



class NLTemplate : public NLTemplateBlock {
protected:
    NLTemplateLoader & loader;
    
public:
    NLTemplate( NLTemplateLoader & loader );
    void clear();
    void load( const char *name );
    std::string render() const;
    
protected:
    void load_recursive( const char *name, std::vector<NLTemplateTokenizer*> & files, std::vector<NLTemplateNode*> & nodes );
};
