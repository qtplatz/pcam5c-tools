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
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>
#include <boost/endian.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>

const static char * i2cdev = "/dev/i2c-0";
bool __verbose = true;

class i2c0 {
    std::unique_ptr< i2c_linux::i2c > i2c_;
    i2c0() : i2c_( std::make_unique< i2c_linux::i2c >() ) {
        if ( ! i2c_->open( i2cdev, 0x3c ) ) {
            std::cerr << "I2C device: " << i2cdev << " could not be opened." << std::endl;
        }
    }
public:
    static i2c0 * instance() {
        static i2c0 __instance;
        return &__instance;
    }
    operator i2c_linux::i2c& () {
        return *i2c_;
    }
};

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
            ( "wreg,w",        po::value<std::vector<std::string> >()->multitoken(), "write reg <addr, value>" )
            ( "all,a",         "read all registers" )
            ( "startup",       "initialize pcam-5c" )
            ( "gpio-number,n", po::value< uint32_t >()->default_value( 960 ), "cam_gpio number" ) // 906+54
            ( "gpio",          po::value< std::string >()->default_value("")->implicit_value("read")
              , "gpio set value [0|1]" )
            ( "off",           "Halt Pcam 5c (gpio down)" )
            ( "on",            "PCam 5c power on" )
            ( "reset",         "PCam 5c power cycle" )
            ( "status",        "PCam 5c power state" )
            ( "iostat",        "PCam 5c IO status" )
            ( "frex",          "PCam 5c FREX status" )
            ( "sstat",         "PCam 5c system status" )
            ( "pad",           "PCam 5c pad output status" )
            ( "sccb",          "PCam 5c SCCB status" )
            ( "sysclk",        "Compute sysclk" )
            ( "light_freq",    "Get light frequency" )
            ( "csi2rx",        "CSI2 RX register" )
            ( "d_phyrx",       "MIPI D-PHY RX register" )
            ;
        po::positional_options_description p;
        p.add( "args",  -1 );
        po::store( po::command_line_parser( argc, argv ).options( description ).positional(p).run(), vm );
        po::notify(vm);
    }
    if ( vm.count( "help" ) || argc < 2 ) {
        std::cerr << description;
        return 0;
    }
    i2cdev = vm[ "device" ].as< std::string >().c_str();

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
        } else if ( arg == "unexport" ) {
            io.unexport();
        }
        return 0;
    }

    if ( vm.count( "status" ) ) {
        std::cout << "gpio_state: " << std::boolalpha << pcam5c().gpio_state() << std::endl;

        std::vector< std::string > regs = { "0x301d", "0x301a", "0x3051, 0x503d" };
        pcam5c().read_regs( *i2c0::instance(), regs );
    }

    if ( vm.count( "reset" ) ) {
        auto res = pcam5c().gpio_reset( 5ms );
        std::cout << "gpio_reset: " << std::boolalpha << res << "\tgpio_state: " << pcam5c().gpio_state() << std::endl;
    }
    if ( vm.count( "off" ) ) {
        if ( pcam5c().gpio_value( false ) && (pcam5c().gpio_state() == false) ) {
            std::cout << "gpio_value set to false with success" << std::endl;
        } else {
            std::cout << "gpio_value set to false but it was failed" << std::endl;
        }
    }
    if ( vm.count( "on" ) ) {
        if ( pcam5c().gpio_value( true ) && (pcam5c().gpio_state() == true) ) {
            std::cout << "gpio_value set to true with success" << std::endl;
        } else {
            std::cout << "gpio_value set to true but it was failed" << std::endl;
        }
    }

    if ( vm.count( "rreg" ) ) {
        pcam5c().read_regs( *i2c0::instance(), vm[ "rreg" ].as< std::vector< std::string > >() );
    }

    if ( vm.count( "wreg" ) ) {
        auto values = vm[ "wreg" ].as< std::vector< std::string > >();
        if ( values.size() == 2 ) {
            char * p_end;
            auto reg = std::strtol( values[0].c_str(), &p_end, 0 );
            auto val = std::strtol( values[1].c_str(), &p_end, 0 );
            pcam5c().write_reg( *i2c0::instance(), {uint16_t(reg), uint8_t(val)}, true );
        } else {
            std::cerr << "invalid number of arguments" << std::endl;
        }
    }
    if ( vm.count( "iostat" ) ) {
        std::vector< std::string > regs = { "0x301d", "0x301a", "0x3050", "0x3051, 0x503d" };
        pcam5c().read_regs( *i2c0::instance(), regs );
    }
    if ( vm.count( "frex" ) ) {
        std::vector< std::string > regs = {
            "0x3b00", "0x3b01", "0x3b02", "0x3b03", "0x3b04", "0x3b05", "0x3b06"
            , "0x3b07", "0x3b08", "0x3b09", "0x3b0a", "0x3b0b", "0x3b0c" };
        pcam5c().read_regs( *i2c0::instance(), regs );
    }
    if ( vm.count( "sstat" ) ) {
        std::vector< std::string > regs = {
            "0x3000", "0x3001", "0x3002", "0x3003", "0x3004"
            , "0x3005", "0x3006", "0x3007", "0x3008", "0x300e", "0x302a" };
        pcam5c().read_regs( *i2c0::instance(), regs );
    }
    if ( vm.count( "pad" ) ) {
        std::vector< std::string > regs = {
            "0x3016", "0x3017", "0x3018", "0x3019", "0x301a"
            , "0x301b", "0x301c", "0x301d", "0x301e", "0x301f", "0x300e", "0x3016", "0x302c" };
        pcam5c().read_regs( *i2c0::instance(), regs );
    }
    if ( vm.count( "sccb" ) ) {
        std::vector< std::string > regs = { "0x3100", "0x3101", "0x3102", "0x3103", "0x3108" };
        pcam5c().read_regs( *i2c0::instance(), regs );
    }
    if ( vm.count( "all" ) ) {
        pcam5c().read_all( *i2c0::instance() );
        return 0;
    }
    if ( vm.count( "startup" ) ) {
        pcam5c().startup( *i2c0::instance() );
    }
    if ( vm.count( "sysclk" ) ) {
        if ( auto sclk = pcam5c().get_sysclk( *i2c0::instance() ) ) {
            std::cout << "sysclk: " << *sclk << std::endl;
        } else {
            std::cout << "sysclk: get failed" << std::endl;
        }
    }
    if ( vm.count( "light_freq" ) ) {
        if ( auto freq = pcam5c().get_light_freq( *i2c0::instance() ) )
            std::cout << "light frequency: " << *freq << "Hz" << std::endl;
        else
            std::cout << "light frequency get failed\n";
    }

    return 0;
}
