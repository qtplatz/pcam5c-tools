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

namespace {
    enum OV5640_REG {
        OV5640_REG_SYS_RESET02          =	0x3002
        , OV5640_REG_SYS_CLOCK_ENABLE02	=	0x3006
        , OV5640_REG_SYS_CTRL0          =	0x3008
        , OV5640_REG_SYS_CTRL0_SW_PWDN	=	0x42
        , OV5640_REG_SYS_CTRL0_SW_PWUP	=	0x02
        , OV5640_REG_CHIP_ID            =	0x300a
        , OV5640_REG_IO_MIPI_CTRL00     =	0x300e
        , OV5640_REG_PAD_OUTPUT_ENABLE01=	0x3017
        , OV5640_REG_PAD_OUTPUT_ENABLE02=	0x3018
        , OV5640_REG_PAD_OUTPUT00		=	0x3019
        , OV5640_REG_SYSTEM_CONTROL1	=	0x302e
        , OV5640_REG_SC_PLL_CTRL0		=	0x3034
        , OV5640_REG_SC_PLL_CTRL1		=	0x3035
        , OV5640_REG_SC_PLL_CTRL2		=	0x3036
        , OV5640_REG_SC_PLL_CTRL3		=	0x3037
        , OV5640_REG_SLAVE_ID           =	0x3100
        , OV5640_REG_SCCB_SYS_CTRL1     =	0x3103
        , OV5640_REG_SYS_ROOT_DIVIDER	=	0x3108
        , OV5640_REG_AWB_R_GAIN         =	0x3400
        , OV5640_REG_AWB_G_GAIN         =	0x3402
        , OV5640_REG_AWB_B_GAIN         =	0x3404
        , OV5640_REG_AWB_MANUAL_CTRL	=	0x3406
        , OV5640_REG_AEC_PK_EXPOSURE_HI	=	0x3500
        , OV5640_REG_AEC_PK_EXPOSURE_MED=	0x3501
        , OV5640_REG_AEC_PK_EXPOSURE_LO	=	0x3502
        , OV5640_REG_AEC_PK_MANUAL      =	0x3503
        , OV5640_REG_AEC_PK_REAL_GAIN	=	0x350a
        , OV5640_REG_AEC_PK_VTS         =	0x350c
        , OV5640_REG_TIMING_DVPHO		=	0x3808
        , OV5640_REG_TIMING_DVPVO		=	0x380a
        , OV5640_REG_TIMING_HTS         =	0x380c
        , OV5640_REG_TIMING_VTS         =	0x380e
        , OV5640_REG_TIMING_TC_REG20	=	0x3820
        , OV5640_REG_TIMING_TC_REG21	=	0x3821
        , OV5640_REG_AEC_CTRL00         =	0x3a00
        , OV5640_REG_AEC_B50_STEP		=	0x3a08
        , OV5640_REG_AEC_B60_STEP		=	0x3a0a
        , OV5640_REG_AEC_CTRL0D         =	0x3a0d
        , OV5640_REG_AEC_CTRL0E         =	0x3a0e
        , OV5640_REG_AEC_CTRL0F         =	0x3a0f
        , OV5640_REG_AEC_CTRL10         =	0x3a10
        , OV5640_REG_AEC_CTRL11         =	0x3a11
        , OV5640_REG_AEC_CTRL1B         =	0x3a1b
        , OV5640_REG_AEC_CTRL1E         =	0x3a1e
        , OV5640_REG_AEC_CTRL1F         =	0x3a1f
        , OV5640_REG_HZ5060_CTRL00      =	0x3c00
        , OV5640_REG_HZ5060_CTRL01      =	0x3c01
        , OV5640_REG_SIGMADELTA_CTRL0C	=	0x3c0c
        , OV5640_REG_FRAME_CTRL01		=	0x4202
        , OV5640_REG_FORMAT_CONTROL00	=	0x4300
        , OV5640_REG_VFIFO_HSIZE		=	0x4602
        , OV5640_REG_VFIFO_VSIZE		=	0x4604
        , OV5640_REG_JPG_MODE_SELECT	=	0x4713
        , OV5640_REG_CCIR656_CTRL00     =	0x4730
        , OV5640_REG_POLARITY_CTRL00	=	0x4740
        , OV5640_REG_MIPI_CTRL00		=	0x4800
        , OV5640_REG_DEBUG_MODE         =	0x4814
        , OV5640_REG_ISP_FORMAT_MUX_CTRL=	0x501f
        , OV5640_REG_PRE_ISP_TEST_SET1	=	0x503d
        , OV5640_REG_SDE_CTRL0          =	0x5580
        , OV5640_REG_SDE_CTRL1          =	0x5581
        , OV5640_REG_SDE_CTRL3          =	0x5583
        , OV5640_REG_SDE_CTRL4          =	0x5584
        , OV5640_REG_SDE_CTRL5          =	0x5585
        , OV5640_REG_AVG_READOUT		=	0x56a1
    };
}

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
                            o << "\t[" << msb << ":" << lsb << "]" << ba[2].as_string() << "= " << bin;
                        } else if ( ba.size() == 2 ) {
                            uint32_t lsb = ba[0].as_int64();
                            o << "\t[" << lsb << "]" << ba[1].as_string() << "= " << std::bitset< 1 >( value >> lsb ).to_string();
                        }
                    }
                }
            }
        }
        o << std::endl;
    } else {
        o << boost::format( "[%04x] =\t0x%02x\t" ) % reg.first % unsigned( value ) << "\t--" << std::endl;
    }
}

void
pcam5c::pprint( std::ostream& o,  uint16_t reg, uint8_t value ) const
{
    auto it = std::lower_bound(ov5640::regs().begin(), ov5640::regs().end(), reg, []( const auto& a, const auto& b ){ return a.first < b; } );
    if ( it != ov5640::regs().end() && it->first == reg ) {
        pprint( o, *it, value );
    } else {
        o << boost::format( "[%04x] =\t0x%02x\t" ) % reg % unsigned( value ) << "\t--" << std::endl;
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
            if ( auto value = iic.read_reg( reg ) ) {
                pprint( std::cout, reg, *value );
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
        std::cout << "write reg: ";
        pprint( std::cout, r.first, r.second );
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

        // for ( const auto& r: ov5640::cfg_init() )
        //     write_reg( iic, r, __verbose );
        for ( const auto& reg: ov5640::init_setting_30fps_VGA() ) {
            write_reg( iic, { reg.reg_addr, reg.val }, __verbose );
        }

        // for ( const auto& r: ov5640::cfg_1080p_30fps() )
        for ( const auto& reg: ov5640::setting_1080P_1920_1080() ) {
            write_reg( iic, { reg.reg_addr, reg.val }, __verbose );
        }

        iic.write_reg( OV5640_REG_IO_MIPI_CTRL00, 0x45 ); // on (0x40 for off)
        iic.write_reg( OV5640_REG_FRAME_CTRL01,   0x00 ); // on (0x0f for off)
    } else {
        std::cerr << "gpio reset failed" << std::endl;
        return false;
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

std::optional< uint32_t >
pcam5c::get_sysclk( i2c_linux::i2c& iic )
{
	 /* calculate sysclk */
	uint32_t xvclk = 24000000 / 10000;
	uint32_t multiplier(0), prediv(0), VCO(0), sysdiv(0), pll_rdiv(0);
	uint32_t sclk_rdiv_map[] = {1, 2, 4, 8};
	uint32_t bit_div2x(1), sclk_rdiv(0), sysclk(0);

    if ( auto ret = iic.read_reg( OV5640_REG_SC_PLL_CTRL0 ) ) {
        uint8_t temp2 = *ret & 0x0f;
        if (temp2 == 8 || temp2 == 10)
            bit_div2x = temp2 / 2;
    } else {
        return {};
    }
    if ( auto ret = iic.read_reg( OV5640_REG_SC_PLL_CTRL1 ) ) {
        sysdiv = *ret >> 4;
        if (sysdiv == 0)
            sysdiv = 16;
    } else {
        return {};
    }

    if ( auto ret = iic.read_reg(OV5640_REG_SC_PLL_CTRL2 ) ) {
        multiplier = *ret;
    } else {
        return {};
    }

    if ( auto ret = iic.read_reg( OV5640_REG_SC_PLL_CTRL3 ) ) {
        prediv = *ret & 0x0f;
        pll_rdiv = ((*ret >> 4) & 0x01) + 1;
    } else {
        return {};
    }

    if ( auto ret = iic.read_reg( OV5640_REG_SYS_ROOT_DIVIDER ) ) {
        uint8_t temp2 = *ret & 0x03;
        sclk_rdiv = sclk_rdiv_map[temp2];
    } else {
        return {};
    }

	if (!prediv || !sysdiv || !pll_rdiv || !bit_div2x)
		return {};

	VCO = xvclk * multiplier / prediv;

	sysclk = VCO / sysdiv / pll_rdiv * 2 / bit_div2x / sclk_rdiv;

	return sysclk;
}

std::optional< uint32_t >
pcam5c::get_light_freq(  i2c_linux::i2c& iic )
{
	/* get banding filter value */
	if ( auto temp = iic.read_reg( OV5640_REG_HZ5060_CTRL01) ) {
        if ( *temp & 0x80 ) { /* manual */
            if ( auto temp1 = iic.read_reg( OV5640_REG_HZ5060_CTRL00 ) ) {
                if (*temp1 & 0x04) {
                    return 50; // light_freq = 50; /* 50Hz */
                } else {
                    return 60; // light_freq = 60; /* 60Hz */
                }
            }
        } else { /* auto */
            if ( auto temp1 = iic.read_reg( OV5640_REG_SIGMADELTA_CTRL0C ) ) {
                if (*temp1 & 0x01) {
                    return 50; // light_freq = 50; /* 50Hz */
                } else {
                    return 60; // light_freq = 60; /* 60Hz */
                }
            }
        }
    }
    return {};
}
