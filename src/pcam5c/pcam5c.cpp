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
#include "pcam5c.hpp"
#include "ov5640.hpp"
#include <boost/format.hpp>
#include <algorithm>
#include <array>
#include <bitset>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <thread>
#include <iomanip>
#include <iostream>

extern bool __verbose;

void
pcam5c::pprint( std::ostream& o,  const std::pair< const uint16_t, boost::json::object >& reg, uint8_t value ) const
{
    const char * label = "";
    if ( !reg.second.empty() ) {
        const auto& obj = reg.second;
        auto vit = std::find_if( obj.begin(), obj.end(), [](const auto& a ){ return a.key() == "name"; });
        if ( vit != obj.end() )
            label = vit->value().as_string().c_str();
        o << boost::format( "[%04x] =\t0x%02x\t" ) % reg.first % unsigned( value ) << label;

        auto fit = std::find_if( obj.begin(), obj.end(), [](const auto& a ){ return a.key() == "flds"; });
        if ( fit != obj.end() ) {
            // o << "\t" << fit->value();
            if ( fit->value().is_array() ) {
                auto a = fit->value().as_array();
                for ( const auto& i: a ) {
                    if ( i.is_array() ) {
                        auto ba = i.as_array();
                        if ( ba.size() == 3 ) {
                            uint32_t msb = ba[0].as_int64();
                            uint32_t lsb = ba[1].as_int64();
                            std::string bin;
                            switch( msb - lsb + 1 ) {
                            case 1: bin = std::bitset< 1 >( value >> lsb ).to_string(); break;
                            case 2: bin = std::bitset< 2 >( value >> lsb ).to_string(); break;
                            case 3: bin = std::bitset< 3 >( value >> lsb ).to_string(); break;
                            case 4: bin = std::bitset< 4 >( value >> lsb ).to_string(); break;
                            case 5: bin = std::bitset< 5 >( value >> lsb ).to_string(); break;
                            case 6: bin = std::bitset< 6 >( value >> lsb ).to_string(); break;
                            case 7: bin = std::bitset< 7 >( value >> lsb ).to_string(); break;
                            case 8: bin = std::bitset< 8 >( value >> lsb ).to_string(); break;
                            }
                            o << "\t" << ba[2].as_string() << "[" << msb << ":" << lsb << "]=" << bin;
                        } else if ( ba.size() == 2 ) {
                            uint32_t lsb = ba[0].as_int64();
                            o << "\t" << ba[1].as_string() << "[" << lsb << "]=" << std::bitset< 1 >( value >> lsb ).to_string();
                        }
                    }
                }
            }
        }
        o << std::endl;
    } else {
        o << boost::format( "[%04x] =\t0x%02x\t" ) % reg.first % unsigned( value ) << "--" << std::endl;
    }
}

bool
pcam5c::read_all( i2c_linux::i2c& i2c )
{
    uint8_t value = 0;
    for ( const auto& reg: ov5640::regs() ) {
        if ( auto value = i2c.read_reg( reg.first ) ) {
            pprint( std::cout, reg, *value );
        } else {
            std::cout << "read error" << std::endl;
        }
    }
    return true;
}

void
pcam5c::read_regs( i2c_linux::i2c& iic, const std::vector< std::string >& regs )
{
    for ( const auto& sreg: regs ) {
        char * p_end;
        auto reg = std::strtol( sreg.c_str(), &p_end, 0 );
        if ( reg >= 0x3000 && reg < 0x6040 ) {
            boost::json::object obj;
            const char * label = "";
            auto it = std::lower_bound(ov5640::regs().begin(), ov5640::regs().end(), reg
                                       , []( const auto& a, const auto& b ){ return a.first < b; } );
            if ( auto value = iic.read_reg( reg ) ) {
                pprint( std::cout, *it, *value );
            }
        } else {
            std::cerr << "specified register : " << sreg << ", (" << std::hex << reg << ") out of range\n";
        }
    }
}

bool
pcam5c::write_reg( i2c_linux::i2c& iic, const std::pair<uint16_t, uint8_t>& r, bool verbose ) const
{
    bool result = iic.write_reg( r.first, r.second );

    if ( verbose ) {
        // boost::json::object obj;
        auto it = std::lower_bound( ov5640::regs().begin(), ov5640::regs().end(), r.first
                                    , []( const auto& a, const auto& b ){ return a.first < b; } );
        if ( it != ov5640::regs().end() ) {
            // obj = it->second;
            std::cout << "write reg: ";
            pprint( std::cout, *it, r.second );
        }
    }
    return result;
}

bool
pcam5c::startup( i2c_linux::i2c& iic )
{
    using namespace std::chrono_literals;

    if ( auto chipid = ov5640().chipid( iic ) ) {
        std::cout << "chipid: "
                  << std::hex << unsigned(chipid->first)
                  << ", " << unsigned(chipid->second)
                  << "\t" << std::boolalpha << bool( *chipid == ov5640_chipid_t ) << std::endl;
        if ( *chipid != ov5640_chipid_t ) {
            std::cerr << "chipid does not match." << std::endl;
            return false;
        }
    }
    if ( gpio_reset() ) {
        write_reg( iic, {0x3103, 0x11}, __verbose ); // ??? something wrong
        write_reg( iic, {0x3008, 0x82}, __verbose ); // software reset (b7 = 1)

        std::this_thread::sleep_for(5ms);

        for ( const auto& r: ov5640::cfg_init() )
            write_reg( iic, r, __verbose );
        for ( const auto& r: ov5640::cfg_1080p_30fps() )
            write_reg( iic, r, __verbose );

    } else {
        std::cerr << "gpio reset failed" << std::endl;
    }

    return true;
}

bool
pcam5c::gpio_state() const
{
    auto inf = std::ifstream( "/dev/ov5640-gpio0", std::ios::binary );
    uint8_t value;
    inf >> value;
    return value;
}

bool
pcam5c::gpio_value( bool value ) const
{
    auto of = std::ofstream( "/dev/ov5640-gpio0", std::ios::binary );
    if ( value )
        of.write( "\1", 1 );
    else
        of.write( "\0", 1 );
    return true;
}

bool
pcam5c::gpio_reset( std::chrono::milliseconds twait ) const
{
    if ( gpio_value( false ) ) {
        std::this_thread::sleep_for( twait );

        gpio_value( true );
        if ( gpio_state() )
            std::this_thread::sleep_for( twait );

        return true;
    }
    return false;
}
