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
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <iomanip>
#include <iostream>

extern bool __verbose;

bool
pcam5c::read_all( i2c_linux::i2c& i2c )
{
    uint8_t value = 0;
    for ( const auto& reg: ov5640::regs() ) {
        if ( auto value = i2c.read_reg( reg.first ) ) {
            std::cout << reg.first << "\t=\t"
                      << std::hex << std::setw(2) << unsigned(*value)
                      << "\t" << reg.second << std::endl;
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
            const char * label = "";
            auto it = std::lower_bound(ov5640::regs().begin(), ov5640::regs().end(), reg
                                       , []( const auto& a, const auto& b ){ return a.first < b; } );
            if ( it != ov5640::regs().end() )
                label = it->second;
            if ( auto value = iic.read_reg( reg ) ) {
                std::cout << std::hex << reg << "\t=\t"
                          << std::setw(2) << unsigned(*value)
                          << "\t" << label << std::endl;
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
        const char * label = "";
        auto it = std::lower_bound( ov5640::regs().begin(), ov5640::regs().end(), r.first
                                    , []( const auto& a, const auto& b ){ return a.first < b; } );
        if ( it != ov5640::regs().end() )
            label = it->second;
        std::cout << "write reg: " << std::hex << r.first << "\t<--\t" << unsigned( r.second )
                  << "\t" << std::boolalpha << result  << "\t" << label << std::endl;
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
    write_reg( iic, {0x3103, 0x11}, __verbose );
    write_reg( iic, {0x3008, 0x82}, __verbose );
    std::this_thread::sleep_for(1.0us);

    for ( const auto& r: ov5640::cfg_init() )
        write_reg( iic, r, __verbose );

    return true;
}
