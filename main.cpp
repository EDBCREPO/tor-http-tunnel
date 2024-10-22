#include <nodepp/nodepp.h>
#include <nodepp/http.h>

using namespace nodepp;

void resolve_socket_1( http_t& cli ) {
    http_t dpx; auto uri = url::parse( cli.path );
    dpx.set_timeout(0); cli.set_timeout(0);
    dpx.socket( uri.hostname, uri.port ); 

    if( dpx.connect() < 0 ){
        cli.write_header( 404, header_t({  }) );
        cli.write("404 | Error while connecting the server");
        return; 
    }

    dpx.write_header( cli.method, uri.path, cli.get_version(), cli.headers );
    stream::duplex( dpx, cli );
}

void resolve_onion_1( http_t& cli ) {
    http_t dpx; auto uri = url::parse( cli.path );
    dpx.set_timeout(0); cli.set_timeout(0);
    dpx.socket( "127.0.0.1", 9050 ); 

    if( dpx.connect() < 0 ){
        cli.write_header( 404, header_t({  }) );
        cli.write("404 | Error while connecting the server");
        return; 
    }

    dpx.write( ptr_t<char>({ 0x05, 0x01, 0x00, 0x00 }) );
    if( dpx.read(2)!=ptr_t<char>({ 0x05, 0x00, 0x00 }) ){ 
        cli.write_header( 404, header_t({  }) );
        cli.write("404 | Error while Handshaking Sock5");
        return; 
    }

    auto dir = uri.path;
    auto dip = uri.hostname; 
    int  prt = (int) uri.port; 
    int  len = (int) dip.size(); 

    dpx.write( ptr_t<char>({ 0x05, 0x01, 0x00, 0x03, len, 0x00 }) );
    dpx.write( dip ); dpx.write( ptr_t<char>({ 0x00, prt, 0x00 }) );
    dpx.read();

    dpx.write_header( cli.method, uri.path, cli.get_version(), cli.headers );
    stream::duplex( dpx, cli );
}

void resolve_socket_2( array_t<string_t>& data, http_t& cli ) {

    data[0] = dns::lookup( data[0] ); socket_t dpx;
    dpx.socket( data[0], string::to_ulong( data[1] ) ); 
    dpx.set_timeout(0); cli.set_timeout(0);

    if( dpx.connect() < 0 ){
        cli.write_header( 404, header_t({  }) );
        cli.write("404 | Error while connecting the server");
        return; 
    }

    cli.write_header( 200, header_t({  }) );
    stream::duplex( cli, dpx );
    
}

void resolve_onion_2( array_t<string_t>& data, http_t& cli ) {

    socket_t dpx; dpx.socket( "127.0.0.1", 9050 ); 
    dpx.set_timeout(0); cli.set_timeout(0);

    if( dpx.connect() < 0 ){
        cli.write_header( 404, header_t({  }) );
        cli.write("404 | Error while connecting the server");
        return; 
    }

    dpx.write( ptr_t<char>({ 0x05, 0x01, 0x00, 0x00 }) );
    if( dpx.read(2)!=ptr_t<char>({ 0x05, 0x00, 0x00 }) ){ 
        cli.write_header( 404, header_t({  }) );
        cli.write("404 | Error while Handshaking Sock5");
        return; 
    }

    int  prt = 80;
    auto dip = data[0]; 
    int  len = (int) dip.size(); 

    dpx.write( ptr_t<char>({ 0x05, 0x01, 0x00, 0x03, len, 0x00 }) );
    dpx.write( dip ); dpx.write( ptr_t<char>({ 0x00, prt, 0x00 }) );
    dpx.read();

    cli.write_header( 200, header_t({  }) );
    stream::duplex( cli, dpx );
    
}

void onMain() {

    auto server = http::server([]( http_t cli ){

        if( cli.method != "CONNECT" ){
            auto uri = url::parse( cli.path );

            if( regex::test( cli.path, "\\.onion" ) ){
                resolve_onion_1( cli );
            } else {
                resolve_socket_1( cli );
            }   return;

        }
        
        auto data = regex::split( cli.path, ":" );

        if( regex::test( data[0], "\\.onion" ) ){
            resolve_onion_2( data, cli );
        } else {
            resolve_socket_2( data, cli );
        }

    });

    server.listen( "0.0.0.0", 5090 ,[]( ... ){
        console::log( "Listenning http://localhost:5090" );
    });

}