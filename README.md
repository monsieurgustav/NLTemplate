##NLTemplate - simple HTML template library for C++##

Use tags like **{{ variable }}** or **{% include header.html %}** or
**{% block items %}{{ item }}{% endblock %}** in your template files.
Load the templates with NLTemplate, fill in the variables and setup the
repeating blocks with C++ code and render the result to stdout or
into a string.

###Features###

- Variable replacement
- Repeatable or optional blocks
- File includes
- No external dependencies

###Requirements###

- C++11 (please use the C++98 branch if you need support for legacy compilers)

###Installation###

To start using NLTemplate, add NLTemplate.cpp to your project and make sure NLTemplate.h is in your header search path.

###Demo###

If you use Xcode, just open and run the demo project. On the command line, you could run the demo like this:

``` bash
git clone git@github.com:catnapgames/NLTemplate.git
cd NLTemplate/NLTemplate
gcc -Wall -pedantic -o demo -lstdc++ *.cpp
./demo
```

###Example###

``` c++
#include <iostream>
#include "NLTemplate.h"

using namespace std;


int main(int, char *[] ) {
    const char *titles[ 3 ] = { "Chico", "Harpo", "Groucho" };
    const char *details[ 3 ] = { "Red", "Green", "Blue" };

    // Let's use the default loader that loads files from disk.
    NLTemplateLoaderFile loader;
    
    // Initialize and load the main template. This will parse it and load any files that are included by it using the {% include ... %} tags.
    NLTemplate t( loader );
    t.load( "test.txt" );
    
    // Set a top-level variable
    t.set( "text", "Hello, world" );
    
    // We need to know in advance that the "items" block will repeat 3 times.
    t.block( "items" ).repeat( 3 );
    
    // Let's fill in the data for the repeated block.
    for ( int i=0; i < 3; i++ ) {
        // Set title and text by accessing the variable directly
        t.block( "items" )[ i ].set( "title", titles[ i ] );
        t.block( "items" )[ i ].set( "text", "Lorem Ipsum" );
        
        // We can get a shortcut reference to a nested block
        NLTemplateBlock & block = t.block( "items" )[ i ].block( "detailblock" );
        block.set( "detail", details[ i ] );
        
        // Disable this block for the first item in the list. Can be useful for opening/closing HTML tables etc.
        if ( i==0 ) {
            block.disable();
        }
    }
    
    // Render the template with the variables we've set above
    t.render( cout );
}
```

###Example - test.txt###

``` html
{% include header.txt %}
    
<p>Items:</p>
{% block items %}<p>
  Title: {{ title }}<br/>
  Text: {{ text }}<br/>
  {% block details %}Detail: {{ detail }}{% endblock %}
</p>{% endblock %}
```

###Example - header.txt###

``` html
<html><body>
<h1>{{ text }}</h1>
```

###Note about HTML###

Despite the headline, there is nothing HTML-specific in NLTemplate.
You can use XML, JSON or plain text as well for your templates.


###Personal note###

If you use NLTemplate in a project, I'd love to hear about it. Please do let me know at tom@catnapgames.com. Thanks!

### See also ###

Goes well with [NLDatabase](https://github.com/catnapgames/NLDatabase) - a lightweight SQLite wrapper for C++.
