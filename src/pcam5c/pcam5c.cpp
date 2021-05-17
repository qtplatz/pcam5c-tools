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

#include "i2c.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <memory>

#if HAVE_BOOST
# include <boost/program_options.hpp>
# include <boost/endian.hpp>
#endif

const static char * i2cdev = "/dev/i2c-0";

class pcam5c {
public:
    bool reg_read( i2c_linux::i2c& i2c, const uint16_t& reg, uint8_t * data, size_t size ) const {
        std::array< uint8_t, sizeof(reg) > ereg = { uint8_t(reg >> 8u), uint8_t(reg & 0xff) };
        return i2c.write( ereg.data(), ereg.size() ) && i2c.read( data, size );
    }
};

int
main( int argc, char **argv )
{
#if HAVE_BOOST
    namespace po = boost::program_options;
    po::variables_map vm;
    po::options_description description( argv[ 0 ] );
    {
        description.add_options()
            ( "help,h",        "Display this help message" )
            ( "device,d",      po::value< std::string >()->default_value("/dev/i2c-0"), "i2c device" )
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

    if ( vm.count( "device" ) ) {
        i2cdev = vm[ "device" ].as< std::string >().c_str();
    }
    std::cerr << "i2c: " << i2cdev << std::endl;
#endif

    if ( auto i2c = std::make_unique< i2c_linux::i2c >() ) {
        if ( i2c->open( i2cdev, 0x3c ) ) {
            uint16_t regs [] = { 0x302c
                                 , 0x301d, 0x301a, 0x3051, 0x3017, 0x301d, 0x301a, 0x3051 // Table 2-2
                                 , 0x3200, 0x3201, 0x3202, 0x3203 // Table 2-3
                                 , 0x3213 // Table 2-4
            };

            uint8_t value = 0;
            for ( const auto& reg: regs ) {
                if ( pcam5c().reg_read( *i2c, reg, &value, 1 ) )
                    std::cout << "pcam5c reg: " << std::hex << reg
                              << "\tvalue: " << std::setw(2) << unsigned(value) << std::endl;
                else
                    std::cout << "read error" << std::endl;
            }

        } else {
            std::cerr << "Failed to acquire bus access and/or talk to slave." << std::endl;
            exit(1);
        }
    }

    return 0;
}
