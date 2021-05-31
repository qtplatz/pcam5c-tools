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
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/system/system_error.hpp>
#include <filesystem>
#include <iostream>
#include <optional>

gpio::~gpio()
{
    if ( auto_unexport_ )
        unexport();
}

gpio::gpio( uint32_t num, bool unexport ) : num_( num )
                                          , auto_unexport_( unexport )
{
    __export( num );
}

bool
gpio::__export( uint32_t num )
{
    boost::filesystem::path dir = std::string( "/sys/class/gpio/gpio" ) + std::to_string( num );
    boost::system::error_code ec;
    if ( !boost::filesystem::exists( dir, ec ) ) {
        std::ofstream of ( "/sys/class/gpio/export" );
        of << num_;
    }
    if ( boost::filesystem::exists( dir, ec ) ) {
        auto direction = dir / "direction";
        if ( boost::filesystem::exists( direction, ec ) ) {
            std::ofstream of ( direction );
            of << "out";
        }

        auto path = dir / "value";
        if ( boost::filesystem::exists( path, ec ) ) {
            outf_.open( path.string(), std::ios::out | std::ios::binary );
            inf_.open( path.string(), std::ios::in );
        }
    }
    return true;
}

bool
gpio::unexport()
{
    boost::system::error_code ec;
    boost::filesystem::path dir = std::string( "/sys/class/gpio/gpio" ) + std::to_string( num_ );
    if ( boost::filesystem::exists( dir, ec ) ) {
        std::ofstream of ( "/sys/class/gpio/unexport" );
        of << num_;
    }
    return true;
}

void
gpio::operator << ( bool flag )
{
    outf_ << ( flag ? "1" : "0" ) << std::endl;
    outf_.flush();
}

int
gpio::read()
{
    char value;
    if ( inf_.read( &value, 1 ) ) {
        if ( value == '0' || value == '1' )
            return value - '0';
    }
    return -1;
}

#if 0
int
main( int argc, char **argv )
{
    namespace po = boost::program_options;
    po::variables_map vm;
    po::options_description description( argv[ 0 ] );
    {
        description.add_options()
            ( "help,h",        "Display this help message" )
            ( "gpio,d",        po::value< uint32_t >()->default_value(960), "gpio number" )
            ( "replicates,r",  po::value< uint32_t >(), "toggle replicates" )
            ( "set",           po::value< bool >(), "gpio set to value[0|1]" )
            ( "read",          "read value" )
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
    uint32_t num = vm[ "gpio" ].as< uint32_t >();
    boost::filesystem::path dir = std::string( "/sys/class/gpio/gpio" ) + std::to_string( num );
    boost::system::error_code ec;
    if ( !boost::filesystem::exists( dir, ec ) ) {
        std::ofstream of ( "/sys/class/gpio/export" );
        of << num;
    }

    boost::filesystem::path direction = dir / "direction";
    if ( boost::filesystem::exists( direction, ec ) ) {
        std::ofstream of ( direction );
        of << "out";
    }

    boost::filesystem::path value = dir / "value";
    gpio_pin gpio( num, value.string().c_str() );
    bool readback = vm.count( "read" );

    bool flag( false );
    if ( vm.count( "set" ) ) {
        flag = vm[ "set" ].as< bool >();
        gpio << flag;
    }

    if ( readback ) {
        int f = gpio.read();
        if ( f >= '0' && f < '1' )
            std::cout << static_cast< char >(f) << std::endl;
        else
            std::cout << value << " = " << f << std::endl;
    }

    if ( vm.count( "replicates" ) ) {
        auto replicates = vm[ "replicates" ].as< uint32_t >();
        while ( replicates-- ) {
            gpio << flag;
            flag = !flag;
        }
    }
    return 0;
}
#endif
