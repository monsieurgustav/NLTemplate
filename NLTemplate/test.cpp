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
        auto & detail = t.block( "items" )[ i ].block( "detail" );
        detail.set( "detail", details[ i ] );
        if ( i==0 ) {
            detail.disable();
        }
    }
    cout << t.render() << endl;
}
