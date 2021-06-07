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

#include "gpio.hpp"
#include "i2c.hpp"
#include "pcam5c.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/endian.hpp>

const static char * i2cdev = "/dev/i2c-0";
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
            ( "device,d",      po::value< std::string >()->default_value("/dev/i2c-0"), "i2c device" )
            ( "rreg,r",        po::value<std::vector<std::string> >()->multitoken(), "read regs" )
            ( "all,a",         "read all registers" )
            ( "startup",       "initialize pcam-5c" )
            ( "gpio-number,n", po::value< uint32_t >()->default_value( 960 ), "cam_gpio number" ) // 906+54
            ( "gpio",          po::value< std::string >()->default_value("")->implicit_value("read")
              , "gpio set value [0|1]" )
            ( "dg",            "list delay pulse data" )
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

    if ( ! vm[ "gpio" ].as< std::string >().empty() ) {
        auto num = vm[ "gpio-number" ].as< uint32_t >();
        gpio io( num );
        auto arg = vm[ "gpio" ].as< std::string >();
        if ( arg == "read" ) {
            auto value = io.read();
            std::cout << value << std::endl;
        } else if ( arg == "1" || arg == "true" ) {
            if ( io << 1 )
                std::cout << "gpio" << num << "=" << 1 << std::endl;
        } else if ( arg == "0" || arg == "false" ) {
            if ( io << 0 )
                std::cout << "gpio" << num << "=" << 0 << std::endl;
        }
        return 0;
    }

    if ( vm.count( "rreg" ) || vm.count( "all" ) || vm.count( "startup" ) ) {

        if ( auto i2c = std::make_unique< i2c_linux::i2c >() ) {
            if ( vm.count( "device" ) ) {
                i2cdev = vm[ "device" ].as< std::string >().c_str();
            }
            if ( i2c->open( i2cdev, 0x3c ) ) {
                if ( vm.count( "rreg" ) ) {
                    pcam5c().read_regs( *i2c, vm[ "rreg" ].as< std::vector< std::string > >() );
                    return 0;
                }
                if ( vm.count( "all" ) ) {
                    pcam5c().read_all( *i2c );
                    return 0;
                }
                if ( vm.count( "startup" ) ) {
                    pcam5c().startup( * i2c );
                }
            } else {
                std::cerr << "Failed to acquire bus access and/or talk to slave." << std::endl;
                exit(1);
            }
        }
    }

    return 0;
}
