***NLTemplate - simple HTML template system for C++***

**Features**

- Variable replacement
- Repeatable or optional blocks
- File includes
- No external dependencies

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
        cout << t.render() << endl;
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

