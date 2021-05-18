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

namespace {
    static const std::vector< std::pair< const uint16_t, const char * > > __regs = {
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
        , { 0x3500, "AEC PK EXPOSURE" }
        , { 0x3501, "AEC PK EXPOSURE" }
        , { 0x3502, "AEC PK EXPOSURE" }
        , { 0x3503, "AEC PK MANUAL" }
        , { 0x350a, "AEC PK REAL GAIN" }
        , { 0x350b, "AEC PK REAL GAIN" }
        , { 0x350c, "AEC PK VTS" }
        , { 0x350d, "AEC PK VTS" }
        , { 0x3602, "VCM CONTROL 0" }
        , { 0x3603, "VCM CONTROL 1" }
        , { 0x3604, "VCM CONTROL 2" }
        , { 0x3605, "VCM CONTROL 3" }
        , { 0x3606, "VCM CONTROL 4" }
        , { 0x3800, "TIMING HS" }
        , { 0x3801, "TIMING HS" }
        , { 0x3802, "TIMING VS" }
        , { 0x3803, "TIMING VS" }
        , { 0x3804, "TIMING HW" }
        , { 0x3805, "TIMING HW" }
        , { 0x3806, "TIMING VH" }
        , { 0x3807, "TIMING VH" }
        , { 0x3808, "TIMING DVPHO" }
        , { 0x3809, "TIMING DVPHO" }
        , { 0x380a, "TIMING DVPHO" }
        , { 0x380b, "TIMING DVPHO" }
        , { 0x380c, "TIMING HTS" }
        , { 0x380d, "TIMING HTS" }
        , { 0x380e, "TIMING HTS" }
        , { 0x380f, "TIMING HTS" }
        , { 0x3810, "TIMING HOFFSET" }
        , { 0x3811, "TIMING HOFFSET" }
        , { 0x3812, "TIMING VOFFSET" }
        , { 0x3813, "TIMING VOFFSET" }
        , { 0x3814, "TIMING X INC" }
        , { 0x3815, "TIMING Y INC" }
        , { 0x3816, "HSYNC START" }
        , { 0x3817, "HSYNC START" }
        , { 0x3818, "HSYNC WIDTH" }
        , { 0x3819, "HSYNC WIDTH" }
        , { 0x3820, "TIMING TC REG20" }
        , { 0x3821, "TIMING TC REG21" }
        , { 0x3a00, "AEC CTRO00" }
        , { 0x3a01, "AEC MIN EXPOSURE" }
        , { 0x3a02, "AEC MAX EXPO (60HZ)" }
        , { 0x3a03, "AEC MAX EXPO (60HZ)" }
        , { 0x3a06, "AEC CTRO06" }
        , { 0x3a07, "AEC CTRO07" }
        , { 0x3a08, "AEC B50 STEP" }
        , { 0x3a09, "AEC B50 STEP" }
        , { 0x3a0a, "AEC B50 STEP" }
        , { 0x3a0b, "AEC B50 STEP" }
        , { 0x3a0c, "AEC CTRL0C" }
        , { 0x3a0d, "AEC CTRL0D" }
        , { 0x3a0e, "AEC CTRL0E" }
        , { 0x3a0f, "AEC CTRL0F" }
        , { 0x3a10, "AEC CTRL10" }
        , { 0x3a11, "AEC CTRL11" }
        , { 0x3a13, "AEC CTRL13" }
        , { 0x3a14, "AEC MAX EXPO (50HZ)" }
        , { 0x3a15, "AEC MAX EXPO (50HZ)" }
        , { 0x3a17, "AEC CTRL17" }
        , { 0x3a18, "AEC GAIN CEILING" }
        , { 0x3a19, "AEC GAIN CEILING" }
        , { 0x3a1a, "AEC DIFF MIN" }
        , { 0x3a1b, "AEC CTRL1B" }
        , { 0x3a1c, "LED ADD ROW" }
        , { 0x3a1d, "LED ADD ROW" }
        , { 0x3a1e, "LED CTRL1E" }
        , { 0x3a1f, "LED CTRL1F" }
        , { 0x3a20, "LED CTRL20" }
        , { 0x3a21, "LED CTRL21" }
        , { 0x3a25, "LED CTRL25" }
        , { 0x3b00, "STROBE CTRL" }
        , { 0x3b01, "FREX EXPOSURE 02" }
        , { 0x3b02, "FREX SHUTTER DELAY 01" }
        , { 0x3b03, "FREX SHUTTER DELAY 00" }
        , { 0x3b04, "FREX EXPOSURE 01" }
        , { 0x3b05, "FREX EXPOSURE 00" }
        , { 0x3b06, "FREX CTRL 07" }
        , { 0x3b07, "FREX MODE" }
        , { 0x3b08, "FREX REQUEST" }
        , { 0x3b09, "FREX HREF DELAY" }
        , { 0x3b0a, "FREX RST LENGTH" }
        , { 0x3b0b, "STROBE WIDTH" }
        , { 0x3b0c, "STROBE WIDTH" }
        , { 0x3c00, "5060HZ CTRL00" }
    };

    static const std::pair< uint16_t, uint8_t > __cfg_init [] =
	{
		{0x3008, 0x42} //[7]=0 Software reset; [6]=1 Software power down; Default=0x02
		, {0x3103, 0x03} //[1]=1 System input clock from PLL; Default read = 0x11
		, {0x3017, 0x00}//[3:0]=0000 MD2P,MD2N,MCP,MCN input; Default=0x00
		, {0x3018, 0x00}//[7:2]=000000 MD1P,MD1N, D3:0 input; Default=0x00
		, {0x3034, 0x18}//[6:4]=001 PLL charge pump, [3:0]=1000 MIPI 8-bit mode

		//PLL1 configuration
		, {0x3035, 0x11}//[7:4]=0001 System clock divider /1, [3:0]=0001 Scale divider for MIPI /1
		, {0x3036, 0x38}//[7:0]=56 PLL multiplier
		, {0x3037, 0x11}//[4]=1 PLL root divider /2, [3:0]=1 PLL pre-divider /1
		, {0x3108, 0x01}//[5:4]=00 PCLK root divider /1, [3:2]=00 SCLK2x root divider /1, [1:0]=01 SCLK root divider /2
		//PLL2 configuration
		, {0x303D, 0x10}		//[5:4]=01 PRE_DIV_SP /1.5, [2]=1 R_DIV_SP /1, [1:0]=00 DIV12_SP /1
		, {0x303B, 0x19}//[4:0]=11001 PLL2 multiplier DIV_CNT5B = 25
		, {0x3630, 0x2e}
		, {0x3631, 0x0e}
		, {0x3632, 0xe2}
		, {0x3633, 0x23}
		, {0x3621, 0xe0}
		, {0x3704, 0xa0}
		, {0x3703, 0x5a}
		, {0x3715, 0x78}
		, {0x3717, 0x01}
		, {0x370b, 0x60}
		, {0x3705, 0x1a}
		, {0x3905, 0x02}
		, {0x3906, 0x10}
        , {0x3901, 0x0a}
		, {0x3731, 0x02}
		//VCM debug mode
		, {0x3600, 0x37}
		, {0x3601, 0x33}
		//System control register changing not recommended
		, {0x302d, 0x60}
		//??
		, {0x3620, 0x52}
		, {0x371b, 0x20}
		//?? DVP
		, {0x471c, 0x50}

		, {0x3a13, 0x43}
		, {0x3a18, 0x00}
		, {0x3a19, 0xf8}
		, {0x3635, 0x13}
		, {0x3636, 0x06}
		, {0x3634, 0x44}
		, {0x3622, 0x01}
		, {0x3c01, 0x34}
		, {0x3c04, 0x28}
		, {0x3c05, 0x98}
		, {0x3c06, 0x00}
		, {0x3c07, 0x08}
		, {0x3c08, 0x00}
		, {0x3c09, 0x1c}
		, {0x3c0a, 0x9c}
		, {0x3c0b, 0x40}
		, {0x503d, 0x00} //[7]=1 color bar enable, [3:2]=00 eight color bar
		, {0x3820, 0x46} //[2]=1 ISP vflip, [1]=1 sensor vflip
		, {0x300e, 0x45}
		, {0x4800, 0x14}
		, {0x302e, 0x08}
		, {0x4300, 0x6f}
		, {0x501f, 0x01}
		, {0x4713, 0x03}
		, {0x4407, 0x04}
		, {0x440e, 0x00}
		, {0x460b, 0x35}
		, {0x460c, 0x20}//[1]=0 DVP PCLK divider manual control by 0x3824[4:0]
        , {0x3824, 0x01}//[4:0]=1 SCALE_DIV=INT(3824[4:0]/2)
		, {0x5000, 0x07}
		, {0x5001, 0x03}
	};

}

bool
pcam5c::read_all( i2c_linux::i2c& i2c )
{
    uint8_t value = 0;
    for ( const auto& reg: __regs ) {
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
            auto it = std::lower_bound(__regs.begin(), __regs.end(), reg
                                       , []( const auto& a, const auto& b ){ return a.first < b; } );
            if ( it != __regs.end() )
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
        auto it = std::lower_bound(__regs.begin(), __regs.end(), r.first
                                   , []( const auto& a, const auto& b ){ return a.first < b; } );
        if ( it != __regs.end() )
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

    for ( const auto& r: __cfg_init )
        write_reg( iic, r, __verbose );

    return true;
}
