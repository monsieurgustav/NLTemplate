***NLTemplate - simple HTML template system for C++***

**Features**

- Variable replacement
- Repeatable or optional blocks
- File includes
- No external dependencies

**Installation**

To start using NLTemplate, add NLTemplate.cpp to your project and make sure NLTemplate.h is in your header search path.

**Demo**

If you use Xcode, just open and run the demo project. On the command line, you could run the demo like this:

    git clone git@github.com:catnapgames/NLTemplate.git
    cd NLTemplate/NLTemplate
    gcc -Wall -pedantic -o demo -lstdc++ *.cpp
    ./demo

**Example - code**

    #include <iostream>
    #include "NLTemplate.h"

    using namespace std;


    int main(int argc, char *argv[] ) {
        const char *titles[3] = { "Chico", "Harpo", "Groucho" };
        const char *details[3] = { "Red", "Green", "Blue" };

        NLTemplateLoaderFile loader;
        NLTemplate t( loader );
        t.load( "test.txt" );
        t.set( "text", "Hello, world" );
        t.block( "items" ).repeat( 3 );
        for ( int i=0; i < 3; i++ ) {
            t.block( "items" )[ i ].set( "title", titles[ i ] );
            t.block( "items" )[ i ].set( "text", "Lorem Ipsum" );
            NLTemplateBlock & detail = t.block( "items" )[ i ].block( "detail" );
            detail.set( "detail", details[ i ] );
            if ( i==0 ) {
                detail.disable();
            }
        }
        
        NLTemplateOutputString output;
        
        t.render( output );
        
        cout << output.buf.str() << endl;
    }

**Example - test.txt**

    {% include header.txt %}
    
    <p>Items:</p>
    {% block items %}<p>
      Title: {{ title }}<br/>
      Text: {{ text }}<br/>
      {% block details %}Detail: {{ detail }}{% endblock %}
    </p>{% endblock %}

**Example - header.txt**

    <html><body>
    <h1>{{ text }}</h1>

