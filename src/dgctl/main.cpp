/*
 * MIT License
 *
 * Copyright (c) 2021 Toshinobu Hondo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pretty_print.hpp"
#include <boost/program_options.hpp>
#include <boost/json.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>

bool __verbose = true;

int
main( int argc, char **argv )
{
    namespace po = boost::program_options;
    po::variables_map vm;
    po::options_description description( argv[ 0 ] );
    {
        description.add_options()
            ( "help,h",        "Display this help message" )
            ( "device,d",      po::value< std::string >()->default_value("/dev/dgmod0"), "dgmod device" )
            ( "list,l",        "list register" )
            ( "commit,c",      "commit" )
            ;
        po::positional_options_description p;
        p.add( "args",  -1 );
        po::store( po::command_line_parser( argc, argv ).options( description ).positional(p).run(), vm );
        po::notify(vm);
    }
    if ( vm.count( "help" ) ) {
        std::cerr << description;
        return 0;
    }

    if ( vm.count( "list" ) ) {
        std::ifstream in( vm[ "device" ].as< std::string >(), std::ios::binary | std::ios::in );
        if ( in ) {
            std::array< uint32_t, 32 > data[ 2 ];
            in.read( reinterpret_cast< char * >( data[ 0 ].data() ), data[0].size() * sizeof( uint32_t ) );
            in.read( reinterpret_cast< char * >( data[ 1 ].data() ), data[1].size() * sizeof( uint32_t ) );

            boost::json::array ja;
            for ( size_t i = 0; i < data[ 0 ].size(); i += 2 ) {
                boost::json::array a;
                a.push_back( data[ 0 ].at( i ) );
                a.push_back( data[ 0 ].at( i + 1 ) );
                a.push_back( data[ 1 ].at( i ) );
                a.push_back( data[ 1 ].at( i + 1 ) );
                ja.push_back( a );
            }
            boost::json::object jobj;
            jobj[ "dg" ] = ja;
            pretty_print( std::cout, jobj );
        }
    }

    return 0;
}
