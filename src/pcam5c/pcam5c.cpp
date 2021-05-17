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
#include <array>
#include <iostream>
#include <iomanip>

// bool
// pcam5c::reg_read( i2c_linux::i2c& i2c, const uint16_t& reg, uint8_t * data, size_t size ) const
// {
//     return i2c.read_reg16( reg, data, size );
// }

bool
pcam5c::read_all( i2c_linux::i2c& i2c )
{
    std::pair< const uint16_t, const char * > regs [] = {
        { 0x3007,   "CLOCK ENABLE03" }
        , { 0x3008, "SYSTEM CTROL0" }
        , { 0x300a, "CHIP ID HIGH BYTE" }
        , { 0x300b, "CHIP ID LOW BYTE" }
        , { 0x300e, "MIPI CONTROL 00" }
        , { 0x3016, "PAD OUTPUT ENABLE 00" }
        , { 0x3017, "PAD OUTPUT ENABLE 01" }
        , { 0x3018, "PAD OUTPUT ENABLE 02" }
        , { 0x3019, "PAD OUTPUT VALUE 00" }
        , { 0x301a, "PAD OUTPUT VALUE 01" }
        , { 0x301b, "PAD OUTPUT VALUE 02" }
        , { 0x301c, "PAD SELECT 00" }
        , { 0x301d, "PAD SELECT 01" }
        , { 0x301e, "PAD SELECT 02" }
        , { 0x302a, "CHIP REVISIOHN" }
        , { 0x302c, "PAD CONTROL 00" }
        , { 0x302d, "SYSTEM CONTROL (DON'T CHANGE)" }
        , { 0x302e, "SYSTEM CONTROL (DON'T CHANGE)" }
        , { 0x302f, "SYSTEM CONTROL (DON'T CHANGE)" }
        , { 0x3030, "SYSTEM CONTROL (DON'T CHANGE)" }
        , { 0x3031, "SC PWC" }
        , { 0x3034, "SC PLL CONTROL0" }
        , { 0x3035, "SC PLL CONTROL1" }
        , { 0x3036, "SC PLL CONTROL2" }
        , { 0x3037, "SC PLL CONTROL3" }
        , { 0x3039, "SC PLL CONTROL5" }
        , { 0x303a, "SC PLLS CTRL0" }
        , { 0x303b, "SC PLLS CTRL1" }
        , { 0x303c, "SC PLLS CTRL2" }
        , { 0x303d, "SC PLLS CTRL3" }
        , { 0x3050, "IO PAD VALUE" }
        , { 0x3051, "IO PAD VALUE" }
        , { 0x3051, "IO PAD VALUE" }
        , { 0x3100, "SCCB_ID" }
        , { 0x3102, "SCCB SYSTEM CTRL0" }
        , { 0x3103, "SCCB SYSTEM CTRL1" }
        , { 0x3108, "SYSTEM ROOT DIVIDER" }
        , { 0x3200, "GROUP ADDR0" }
        , { 0x3201, "GROUP ADDR1" }
        , { 0x3202, "GROUP ADDR2" }
        , { 0x3203, "GROUP ADDR3" }
        , { 0x3212, "SRM GROUP ACCESS" }
        , { 0x3213, "SRM GROUP STATUS" }
        , { 0x3400, "AWB R GAIN" }
        , { 0x3401, "AWB R GAIN" }
        , { 0x3402, "AWB R GAIN" }
        , { 0x3403, "AWB R GAIN" }
        , { 0x3404, "AWB R GAIN" }
        , { 0x3405, "AWB R GAIN" }
        , { 0x3406, "AWB R GAIN" }
    };

    uint8_t value = 0;
    for ( const auto& reg: regs ) {
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

bool
pcam5c::startup( i2c_linux::i2c& i2c )
{
    if ( auto chipid = ov5640().chipid( i2c ) ) {
        std::cout << "chipid: "
                  << std::hex << unsigned(chipid->first)
                  << ", " << unsigned(chipid->second)
                  << "\t" << std::boolalpha << bool( *chipid == ov5640_chipid_t ) << std::endl;
        if ( *chipid != ov5640_chipid_t ) {
            std::cerr << "chipid does not match." << std::endl;
            return false;
        }
    }
    read_all( i2c );
    return true;
}
