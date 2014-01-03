#include <iostream>
#include "NLTemplate.h"


using namespace std;
using namespace NL::Template;



int main(int, char *[] ) {
    const char *titles[ 3 ] = { "Chico", "Harpo", "Groucho" };
    const char *details[ 3 ] = { "Red", "Green", "Blue" };

    // Let's use the default loader that loads files from disk.
    LoaderFile loader;
    
    // Initialize and load the main template. This will parse it and load any files that are included by it using the {% include ... %} tags.
    Template t( loader );
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
        Block & block = t.block( "items" )[ i ].block( "detailblock" );
        block.set( "detail", details[ i ] );
        
        // Disable this block for the first item in the list. Can be useful for opening/closing HTML tables etc.
        if ( i==0 ) {
            block.disable();
        }
    }
    
    // Render the template with the variables we've set above, print to screen
    t.render( cout );

    
    cout << "--------------------------------------------------------------------------------" << endl;
    
    // Another output method can place the output into a string
    stringstream buffer;
    t.render( buffer );
    cout << buffer.str() << endl;
    
    return 0;
}
