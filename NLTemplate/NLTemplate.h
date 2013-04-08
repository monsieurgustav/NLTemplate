#pragma once


#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <regex>


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
    const std::string text;
    const std::regex block;
    const std::regex endblock;
    const std::regex include;
    const std::regex var;
    int pos;
    NLToken peek;
    bool peeking;
public:
    NLTemplateTokenizer( const std::string & text );
    NLToken next();
};


class NLTemplateDictionary {
public:
    std::unordered_map<std::string, std::string> properties;
    
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
    void repeat( int n );
    NLTemplateNode & operator[]( int index );
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
    virtual std::string load( const std::string & name ) = 0;
};


class NLTemplateLoaderFile : public NLTemplateLoader {
public:
    std::string load( const std::string & name );
};



class NLTemplate : public NLTemplateBlock {
protected:
    NLTemplateLoader & loader;
    
public:
    NLTemplate( NLTemplateLoader & loader );
    void clear();
    void load( const std::string & name );
    std::string render() const;
    
protected:
    void load_recursive( const std::string & name, std::vector<NLTemplateTokenizer> & files, std::vector<NLTemplateNode*> & nodes );
};
