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

#include "ov5640.hpp"
#include "i2c.hpp"
#include <boost/json.hpp>

bool
ov5640::reset( i2c_linux::i2c& ) const
{
    // todo
    // gpio CAM_GPIO_0 clear and set
    return true;
}

std::optional< std::pair< uint8_t, uint8_t > >
ov5640::chipid( i2c_linux::i2c& i2c ) const
{
    if ( auto chip_id_high_byte = i2c.read_reg( 0x300a ) ) {
        if ( auto chip_id_low_byte = i2c.read_reg( 0x300b ) ) {
            return {{ *chip_id_high_byte, *chip_id_low_byte }};
        }
    }
    return {};
}

namespace {
    using namespace boost::json;
    //const std::vector< std::pair< const uint16_t, const char * > > __regs = {
    const std::vector< std::pair< const uint16_t, boost::json::object > > __regs = {
        { 0x3000, object( {{"name","SYSTEM RESET00"}   ,{"flds",array( {{7,"BIST"},{6,"MEM"},{5,"MCU"},{4,"OTP"},{3,"STB"},{2,"d5060"},{1,"TC"},{0,"AC"}} )} /**/}) }
        , { 0x3001, object( {{"name","SYSTEM RESET01"}   ,{"flds",array( {{7,"AWB"},{6,"AFC"},{5,"ISP"},{4,"FC"},{3,"S2P"},{2,"BLC"},{1,"AEC"},{0,"AEC"}} )} /**/}) }
        , { 0x3002, object( {{"name","SYSTEM RESET02"}   ,{"flds",array( {{7,"VFIFO"},{5,"FMT"},{4,"JFIFO"},{3,"SFIFO"},{2,"JPG"},{1,"MUX"},{0,"AVG"}} )} /**/}) }
        , { 0x3003, object( {{"name","SYSTEM RESET03"}   ,{"flds",array( {{5,"DGC"},{4,"SYNC FIFO"},{3,"PSRAM"},{2,"ISP FC"},{1,"MIPI"},{0,"DVP"}} )} /**/}) }
        , { 0x3004, object( {{"name","CLOCK ENABLE00"}   ,{"flds",array( {{7,"BIST"},{6,"MEM"},{5,"MCU"},{4,"OTP"},{3,"STB"},{2,"d5060"},{1,"TC"},{0,"AC"}} )} /**/}) }
        , { 0x3005, object( {{"name","CLOCK ENABLE01"}   ,{"flds",array( {{7,"AWB"},{6,"AFC"},{5,"ISP"},{4,"FC"},{3,"S2P"},{2,"BLC"},{1,"AEC"},{0,"AEC"}} )} /**/}) }
        , { 0x3006, object( {{"name","CLOCK ENABLE02"}   ,{"flds",array( {{7,"PSRAM"},{6,"FMT"},{5,"JPEG2x"},{3,"PEG"},{1,"MUX"},{0,"AVG"}} )} /**/}) }
        , { 0x3007, object( {{"name","CLOCK ENABLE03"}   ,{"flds",array( {{7,"DGC"},{6,"SYNC FIFO"},{5,"ISPFC"},{4,"MIPI pclk"},{3,"MIPI clk"},{2,"DVP pclk"},{1,"VFIFO pclk"},{0,"VFIFO sclk"}} )} /**/}) }
        , { 0x3008, object( {{"name","SYSTEM CTROL0"}    ,{"flds",array( {{7,"soft-reset"},{6,"PD"}} )}                  }) }
        , { 0x300a, object( {{"name", "CHIP ID HIGH BYTE" }          }) }
        , { 0x300b, object( {{"name", "CHIP ID LOW BYTE" }           }) }
        , { 0x300e, object( {{"name", "MIPI CONTROL 00"} ,{"flds",array( {{7,5,"l-mode"},{4,"TxPD"},{3,"RxPD"},{2,"EN"}}             )}/**/}) }
        , { 0x3016, object( {{"name", "PAD OUTPUT EN" }  ,{"flds",array( {{1,"strb-oe"} ,{0,"siod-oe"}}                        )}/**/}) }
        , { 0x3017, object( {{"name", "PAD OUTPUT EN" }  ,{"flds",array( {{7,"FREX"}    ,{6,"VSYNC"},{5,"HREF"},{4,"PCLK"},{3,0,"D[9:6]"} } )}/**/}) }
        , { 0x3018, object( {{"name", "PAD OUTPUT EN" }  ,{"flds",array( {{7,2,"D[5:0]"},{1,"GPIO1"},{0,"GPIO0"}             }          )}/**/}) }
        , { 0x3019, object( {{"name", "PAD OUTPUT VAL"}  ,{"flds",array( {{7,"MIPI"}    ,{6,"L2ST"},{5,"L1ST"},{4,"CLST"},{1,"STB"},{0,"SIOD"} } )}/**/}) }
        , { 0x301a, object( {{"name", "GPIO VALUE 01"}   ,{"flds",array( {{7,"FREX"}    ,{6,"VSYNC"},{5,"HREF"},{4,"PCLK"},{3,0,"D[9:6]"} } )}/**/}) }
        , { 0x301b, object( {{"name", "GPIO VALUE 02"}   ,{"flds",array( {{7,2,"D[5:0]"},{1,"GPIO1"},{0,"GPIO0"}             } )}/**/}) }
        , { 0x301c, object( {{"name", "OutSEL for GPIO"} ,{"flds",array( {{1,"IO_STRB"} ,{0,"IO_SIOD"}            } )}/**/}) }
        , { 0x301d, object( {{"name", "OutSEL for GPIO"} ,{"flds",array( {{7,"FREX"}    ,{6,"VSYNC"},{5,"HREF"},{4,"PCLK"},{3,0,"D[9:6]"} } )}/**/}) }
        , { 0x301e, object( {{"name", "OutSEL for GPIO"} ,{"flds",array( {{7,2,"D[5:0]sel(X)"},{1,"GPIO1"},{0,"GPIO0"}          } )}/**/}) }
        , { 0x302a, object( {{"name", "CHIP REVISIOHN" }               }) }
        , { 0x302c, object( {{"name", "PAD CONTROL 00" }               }) }

        , { 0x3031, object( {{"name", "SC PWC" }         ,{"flds",array( {{3,"Bypass regulator"}     } )}/**/}) }
        , { 0x3034, object( {{"name", "SC PLL CONTROL0" },{"flds",array( {{6,4,"PLL"},{3,0,"bit mode"}             } )}/**/}) }
        , { 0x3035, object( {{"name", "SC PLL CONTROL1" },{"flds",array( {{7,4,"clk div"},{3,0,"sc div"}                     } )}/**/}) }
        , { 0x3036, object( {{"name", "SC PLL CONTROL2" },{"flds",array( {{7,0,"pll mul"}                                    } )}/**/}) }
        , { 0x3037, object( {{"name", "SC PLL CONTROL3" },{"flds",array( {{4,"PLL root div"},{3,0,"PLL pre-div"} } )}/**/}) }
        , { 0x3039, object( {{"name", "SC PLL CONTROL5" },{"flds",array( {{7,"PLL bypass"}}                        )}/**/}) }
        , { 0x303a, object( {{"name", "SC PLLS CTRL0" }  ,{"flds",array( {{7,"PLLS bypass"}}                       )}/**/}) }
        , { 0x303b, object( {{"name", "SC PLLS CTRL1" }  ,{"flds",array( {{4,0,"PLLS mul"}}                        )}/**/}) }
        , { 0x303c, object( {{"name", "SC PLLS CTRL2" }  ,{"flds",array( {{6,4,"PLLS cpc"},{3,0,"PLLS div"}}         )}/**/}) }
        , { 0x303d, object( {{"name", "SC PLLS CTRL3" }  ,{"flds",array( {{5,4,"PLLS pre-div"},{1,0,"PLLS seld"}} )}/**/}) }
        , { 0x3050, object( {{"name", "IO PAD VALUE" }   ,{"flds",array( {{4,"FREX"},{3,"PWDN"},{1,"SIOC"}} )}/**/}) }
        , { 0x3051, object( {{"name", "IO PAD VALUE" }   ,{"flds",array( {{7,"OTPmo"},{6,"VSYNC"},{5,"HREF"},{4,"PCLK"},{3,0,"D[9:6]"}} )}/**/}) }
        , { 0x3051, object( {{"name", "Pad Input ST" }   ,{"flds",array( {{7,2,"D[5:0]"},{1,"GPIO1"},{0,"GPIO0"}}              )}/**/}) }
        , { 0x3100, object( {{"name", "SCCB_ID" }        ,{"flds",array( {{7,0,"SlaveID"}}              )}/**/}) }
        , { 0x3102, object( {{"name", "SCCB SYSTEM CTRL0"},{"flds",array( {{6,"MIPI SCres"},{5,"SRBres"},{4,"SCCB_rst"}
                                                                                               ,{3,"rst_pon_sccb"},{1,"MIPI-PHYrst"},{0,"PLLrst"}} )}/**/}) }
        , { 0x3103, object( {{"name", "PLL Clk SEL"}   ,{"flds",array( {{1,"sysclk"}}              )}/**/}) }
        , { 0x3108, object( {{"name", "PAD Clk div(SCCB)"},{"flds",array( {{5,4,"root div"},{3,2,"pclk2x div"},{1,0,"SCLK div"}}              )}/**/}) }
        , { 0x3200, object( {{"name", "GROUP ADDR0" }               }) }
        , { 0x3201, object( {{"name", "GROUP ADDR1" }               }) }
        , { 0x3202, object( {{"name", "GROUP ADDR2" }               }) }
        , { 0x3203, object( {{"name", "GROUP ADDR3" }               }) }
        , { 0x3212, object( {{"name", "SRM GROUP ACCESS" }               }) }
        , { 0x3213, object( {{"name", "SRM GROUP STATUS" }               }) }
        , { 0x3400, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3401, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3402, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3403, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3404, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3405, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3406, object( {{"name", "AWB R GAIN" }               }) }
        , { 0x3500, object( {{"name", "AEC PK EXPOSURE" }               }) }
        , { 0x3501, object( {{"name", "AEC PK EXPOSURE" }               }) }
        , { 0x3502, object( {{"name", "AEC PK EXPOSURE" }               }) }
        , { 0x3503, object( {{"name", "AEC PK MANUAL" }               }) }
        , { 0x350a, object( {{"name", "AEC PK REAL GAIN" }               }) }
        , { 0x350b, object( {{"name", "AEC PK REAL GAIN" }               }) }
        , { 0x350c, object( {{"name", "AEC PK VTS" }               }) }
        , { 0x350d, object( {{"name", "AEC PK VTS" }               }) }
        , { 0x3602, object( {{"name", "VCM CONTROL 0" }               }) }
        , { 0x3603, object( {{"name", "VCM CONTROL 1" }               }) }
        , { 0x3604, object( {{"name", "VCM CONTROL 2" }               }) }
        , { 0x3605, object( {{"name", "VCM CONTROL 3" }               }) }
        , { 0x3606, object( {{"name", "VCM CONTROL 4" }               }) }
        , { 0x3800, object( {{"name", "TIMING HS" }               }) }
        , { 0x3801, object( {{"name", "TIMING HS" }               }) }
        , { 0x3802, object( {{"name", "TIMING VS" }               }) }
        , { 0x3803, object( {{"name", "TIMING VS" }               }) }
        , { 0x3804, object( {{"name", "TIMING HW" }               }) }
        , { 0x3805, object( {{"name", "TIMING HW" }               }) }
        , { 0x3806, object( {{"name", "TIMING VH" }               }) }
        , { 0x3807, object( {{"name", "TIMING VH" }               }) }
        , { 0x3808, object( {{"name", "TIMING DVPHO" }               }) }
        , { 0x3809, object( {{"name", "TIMING DVPHO" }               }) }
        , { 0x380a, object( {{"name", "TIMING DVPHO" }               }) }
        , { 0x380b, object( {{"name", "TIMING DVPHO" }               }) }
        , { 0x380c, object( {{"name", "TIMING HTS" }               }) }
        , { 0x380d, object( {{"name", "TIMING HTS" }               }) }
        , { 0x380e, object( {{"name", "TIMING HTS" }               }) }
        , { 0x380f, object( {{"name", "TIMING HTS" }               }) }
        , { 0x3810, object( {{"name", "TIMING HOFFSET" }               }) }
        , { 0x3811, object( {{"name", "TIMING HOFFSET" }               }) }
        , { 0x3812, object( {{"name", "TIMING VOFFSET" }               }) }
        , { 0x3813, object( {{"name", "TIMING VOFFSET" }               }) }
        , { 0x3814, object( {{"name", "TIMING X INC" }               }) }
        , { 0x3815, object( {{"name", "TIMING Y INC" }               }) }
        , { 0x3816, object( {{"name", "HSYNC START" }               }) }
        , { 0x3817, object( {{"name", "HSYNC START" }               }) }
        , { 0x3818, object( {{"name", "HSYNC WIDTH" }               }) }
        , { 0x3819, object( {{"name", "HSYNC WIDTH" }               }) }
        , { 0x3820, object( {{"name", "TIMING TC REG20" }               }) }
        , { 0x3821, object( {{"name", "TIMING TC REG21" }               }) }
        , { 0x3a00, object( {{"name", "AEC CTRO00" }               }) }
        , { 0x3a01, object( {{"name", "AEC MIN EXPOSURE" }               }) }
        , { 0x3a02, object( {{"name", "AEC MAX EXPO (60HZ)" }               }) }
        , { 0x3a03, object( {{"name", "AEC MAX EXPO (60HZ)" }               }) }
        , { 0x3a06, object( {{"name", "AEC CTRO06" }               }) }
        , { 0x3a07, object( {{"name", "AEC CTRO07" }               }) }
        , { 0x3a08, object( {{"name", "AEC B50 STEP" }               }) }
        , { 0x3a09, object( {{"name", "AEC B50 STEP" }               }) }
        , { 0x3a0a, object( {{"name", "AEC B50 STEP" }               }) }
        , { 0x3a0b, object( {{"name", "AEC B50 STEP" }               }) }
        , { 0x3a0c, object( {{"name", "AEC CTRL0C" }               }) }
        , { 0x3a0d, object( {{"name", "AEC CTRL0D" }               }) }
        , { 0x3a0e, object( {{"name", "AEC CTRL0E" }               }) }
        , { 0x3a0f, object( {{"name", "AEC CTRL0F" }               }) }
        , { 0x3a10, object( {{"name", "AEC CTRL10" }               }) }
        , { 0x3a11, object( {{"name", "AEC CTRL11" }               }) }
        , { 0x3a13, object( {{"name", "AEC CTRL13" }               }) }
        , { 0x3a14, object( {{"name", "AEC MAX EXPO (50HZ)" }               }) }
        , { 0x3a15, object( {{"name", "AEC MAX EXPO (50HZ)" }               }) }
        , { 0x3a17, object( {{"name", "AEC CTRL17" }               }) }
        , { 0x3a18, object( {{"name", "AEC GAIN CEILING" }               }) }
        , { 0x3a19, object( {{"name", "AEC GAIN CEILING" }               }) }
        , { 0x3a1a, object( {{"name", "AEC DIFF MIN" }               }) }
        , { 0x3a1b, object( {{"name", "AEC CTRL1B" }               }) }
        , { 0x3a1c, object( {{"name", "LED ADD ROW" }               }) }
        , { 0x3a1d, object( {{"name", "LED ADD ROW" }               }) }
        , { 0x3a1e, object( {{"name", "LED CTRL1E" }               }) }
        , { 0x3a1f, object( {{"name", "LED CTRL1F" }               }) }
        , { 0x3a20, object( {{"name", "LED CTRL20" }               }) }
        , { 0x3a21, object( {{"name", "LED CTRL21" }               }) }
        , { 0x3a25, object( {{"name", "LED CTRL25" }               }) }
        , { 0x3b00, object( {{"name", "STROBE CTRL" }       ,{"flds",array( {{7,"STRBrq"},{6,"STBneg"},{3,2,"W-Xe"},{1,0,"STBmode"}}   )}/**/}) }
        , { 0x3b01, object( {{"name", "FREX EXPOSURE" }     ,{"flds",array( {{7,0,"EXP time[23:16]"}}                                  )}/**/}) }
        , { 0x3b02, object( {{"name", "FREX SHUTTER DELAY"} ,{"flds",array( {{5,0,"Shutter delay[12:8]"}}                              )}/**/}) }
        , { 0x3b03, object( {{"name", "FREX SHUTTER DELAY"} ,{"flds",array( {{5,0,"Shutter delay[7,0](64 x clk)"}}                     )}/**/}) }
        , { 0x3b04, object( {{"name", "FREX EXPOSURE" }     ,{"flds",array( {{7,0,"EXP time[15:8]"}}                                   )}/**/}) }
        , { 0x3b05, object( {{"name", "FREX EXPOSURE" }     ,{"flds",array( {{7,0,"EXP time[7:0](Tline)"}}                             )}/**/}) }
        , { 0x3b06, object( {{"name", "FREX CTRL 07" }      ,{"flds",array( {{7,4,"FREX frame delay"},{3,0,"STRB width"}}              )}/**/}) }
        , { 0x3b07, object( {{"name", "FREX MODE" }         ,{"flds",array( {{1,0,"FREX mode"}}                                        )}/**/}) }
        , { 0x3b08, object( {{"name", "FREX REQUEST" }                  }) }
        , { 0x3b09, object( {{"name", "FREX HREF DELAY" }               }) }
        , { 0x3b0a, object( {{"name", "FREX RST LENGTH" }   ,{"flds",array( {{2,0,"FREX precharge length"}}          )}/**/}) }
        , { 0x3b0b, object( {{"name", "STROBE WIDTH" }      ,{"flds",array( {{2,0,"STRB width[19:12]"}}              )}/**/}) }
        , { 0x3b0c, object( {{"name", "STROBE WIDTH" }      ,{"flds",array( {{7,0,"STRB width[11:4]"}}               )}/**/}) }

        , { 0x3c00, object( {{"name", "5060HZ CTRL00" }               }) }
        , { 0x3c01, object( {{"name", "5060HZ CTRL01" }               }) }
        , { 0x3c02, object( {{"name", "5060HZ CTRL02" }               }) }
        , { 0x3c03, object( {{"name", "5060HZ CTRL03" }               }) }
        , { 0x3c04, object( {{"name", "5060HZ CTRL04" }               }) }
        , { 0x3c05, object( {{"name",  "5060HZ CTRL05" }               }) }
        , { 0x3c06,	object( {{"name", "LIGHT METER1 THRESHOLD"  }               }) }
        , { 0x3c07, object( {{"name", "LIGHT METER1 THRESHOLD" }               }) }
        , { 0x3c08, object( {{"name", "LIGHT METER2 THRESHOLD" }               }) }
        , { 0x3c09, object( {{"name", "LIGHT METER2 THRESHOLD" }               }) }
        , { 0x3c0a, object( {{"name", "SAMPLE NUMBER" }               }) }
        , { 0x3c0b, object( {{"name", "SAMPLE NUMBER" }               }) }
        , { 0x3c0c, object( {{"name", "SIGMADELTA CTRL0C" }               }) }
        , { 0x3c0d, object( {{"name", "SUM 50" }               }) }
        , { 0x3c0e, object( {{"name", "SUM 50" }               }) }
        , { 0x3c0f, object( {{"name", "SUM 50" }               }) }
        , { 0x3c10, object( {{"name", "SUM 50" }               }) }
        , { 0x3c11, object( {{"name", "SUM 60" }               }) }
        , { 0x3c12, object( {{"name", "SUM 60" }               }) }
        , { 0x3c13, object( {{"name", "SUM 60" }               }) }
        , { 0x3c14, object( {{"name", "SUM 60" }               }) }
        , { 0x3c15, object( {{"name", "SUM 50 60" }               }) }
        , { 0x3c16, object( {{"name", "SUM 50 60" }               }) }
        , { 0x3c17, object( {{"name", "BLOCK COUNTER" }               }) }
        , { 0x3c18, object( {{"name", "BLOCK COUNTER" }               }) }
        , { 0x3c19, object( {{"name", "B6" }               }) }
        , { 0x3c1a, object( {{"name", "B6" }               }) }
        , { 0x3c1b, object( {{"name", "LIGHTMETER OUTPUT" }               }) }
        , { 0x3c1c, object( {{"name", "LIGHTMETER OUTPUT" }               }) }
        , { 0x3c1d, object( {{"name", "LIGHTMETER OUTPUT" }               }) }
        , { 0x3c1e, object( {{"name", "SUM THRESHOLD" }               }) }
        , { 0x3d00, object( {{"name", "OTP DATA00" }               }) }
        , { 0x3d01, object( {{"name", "OTP DATA01" }               }) }
        , { 0x3d02, object( {{"name", "OTP DATA02" }               }) }
        , { 0x3d03, object( {{"name", "OTP DATA03" }               }) }
        , { 0x3d04, object( {{"name", "OTP DATA04" }               }) }
        , { 0x3d05, object( {{"name", "OTP DATA05" }               }) }
        , { 0x3d06, object( {{"name", "OTP DATA06" }               }) }
        , { 0x3d07, object( {{"name", "OTP DATA07" }               }) }
        , { 0x3d08, object( {{"name", "OTP DATA08" }               }) }
        , { 0x3d09, object( {{"name", "OTP DATA09" }               }) }
        , { 0x3d0a, object( {{"name", "OTP DATA0A" }               }) }
        , { 0x3d0b, object( {{"name", "OTP DATA0B" }               }) }
        , { 0x3d0c, object( {{"name", "OTP DATA0C" }               }) }
        , { 0x3d0d, object( {{"name", "OTP DATA0D" }               }) }
        , { 0x3d0e, object( {{"name", "OTP DATA0E" }               }) }
        , { 0x3d0f, object( {{"name", "OTP DATA0F" }               }) }
        , { 0x3d10, object( {{"name", "OTP DATA10" }               }) }
        , { 0x3d11, object( {{"name", "OTP DATA11" }               }) }
        , { 0x3d12, object( {{"name", "OTP DATA12" }               }) }
        , { 0x3d13, object( {{"name", "OTP DATA13" }               }) }
        , { 0x3d14, object( {{"name", "OTP DATA14" }               }) }
        , { 0x3d15, object( {{"name", "OTP DATA15" }               }) }
        , { 0x3d16, object( {{"name", "OTP DATA16" }               }) }
        , { 0x3d17, object( {{"name", "OTP DATA17" }               }) }
        , { 0x3d18, object( {{"name", "OTP DATA18" }               }) }
        , { 0x3d19, object( {{"name", "OTP DATA19" }               }) }
        , { 0x3d1a, object( {{"name", "OTP DATA1A" }               }) }
        , { 0x3d1b, object( {{"name", "OTP DATA1B" }               }) }
        , { 0x3d1c, object( {{"name", "OTP DATA1C" }               }) }
        , { 0x3d1d, object( {{"name", "OTP DATA1D" }               }) }
        , { 0x3d1e, object( {{"name", "OTP DATA1E" }               }) }
        , { 0x3d1f, object( {{"name", "OTP DATA1F" }               }) }
        , { 0x3d20, object( {{"name", "OTP PROGRAM CTRL" }               }) }
        , { 0x3d21, object( {{"name", "OTP READ CTRL" }               }) }
        , { 0x3f00,	object( {{"name", "MC CTRL00" }               }) }
        , { 0x3f01,	object( {{"name", "MC INTERRUPT MASK0" }               }) }
        , { 0x3f02,	object( {{"name", "MC INTERRUPT MASK1" }               }) }
        , { 0x3f03,	object( {{"name", "MC READ INTERRUPT ADDRESS" }               }) }
        , { 0x3f04,	object( {{"name", "MC READ INTERRUPT ADDRESS" }               }) }
		, { 0x3f05,	object( {{"name", "MC WRITE INTERRUPT ADDRESS" }               }) }
		, { 0x3f06,	object( {{"name", "MC WRITE INTERRUPT ADDRESS" }               }) }
		, { 0x3f08,	object( {{"name", "MC INTERRUPT SOURCE SELECTION1" }               }) }

		, { 0x3f09,	object( {{"name", "MC INTERRUPT SOURCE SELECTION2" }               }) }
		, { 0x3f0a,	object( {{"name", "MC INTERRUPT SOURCE SELECTION3" }               }) }
		, { 0x3f0b,	object( {{"name", "MC INTERRUPT SOURCE SELECTION4" }               }) }
		, { 0x3f0c,	object( {{"name", "MC INTERRUPT0 STATUS" }               }) }
		, { 0x3f0d,	object( {{"name", "MC INTERRUPT1 STATUS" }               }) }

		, { 0x4000,	object( {{"name", "BLC CTRL00" }               }) }
		, { 0x4001,	object( {{"name", "BLC CTRL01" }               }) }
		, { 0x4002,	object( {{"name", "BLC CTRL02" }               }) }
		, { 0x4003,	object( {{"name", "BLC CTRL03" }               }) }
		, { 0x4004,	object( {{"name", "BLC CTRL04" }               }) }
		, { 0x4005,	object( {{"name", "BLC CTRL05" }               }) }
		, { 0x4006,	object( {{"name", "BLC CTRL06" }               }) }
		, { 0x4007,	object( {{"name", "BLC CTRL07" }               }) }
		, { 0x4009,	object( {{"name", "BLACK LEVEL" }               }) }
		, { 0x402c,	object( {{"name", "BLACK LEVEL00" }               }) }
		, { 0x402d,	object( {{"name", "BLACK LEVEL00" }               }) }
		, { 0x402e,	object( {{"name", "BLACK LEVEL01" }               }) }
		, { 0x402f,	object( {{"name", "BLACK LEVEL01" }               }) }
		, { 0x4030,	object( {{"name", "BLACK LEVEL10" }               }) }
		, { 0x4031,	object( {{"name", "BLACK LEVEL10" }               }) }
		, { 0x4032,	object( {{"name", "BLACK LEVEL11" }               }) }
		, { 0x4033,	object( {{"name", "BLACK LEVEL11" }               }) }
		, { 0x4201,	object( {{"name", "FRAME CTRL01" }               }) }
		, { 0x4202,	object( {{"name", "FRAME CTRL02" }               }) }
		, { 0x4300,	object( {{"name", "FORMAT CONTROL 00" }               }) }
		, { 0x4301,	object( {{"name", "FORMAT CONTROL 01" }               }) }
		, { 0x4302,	object( {{"name", "YMAX VALUE" }               }) }
		, { 0x4303,	object( {{"name", "YMAX VALUE" }               }) }
		, { 0x4304,	object( {{"name", "YMIN VALUE" }               }) }
		, { 0x4305,	object( {{"name", "YMIN VALUE" }               }) }
		, { 0x4306,	object( {{"name", "UMAX VALUE" }               }) }
		, { 0x4307,	object( {{"name", "UMAX VALUE" }               }) }
		, { 0x4308,	object( {{"name", "UMIN VALUE" }               }) }
		, { 0x4309,	object( {{"name", "UMIN VALUE" }               }) }
		, { 0x430a,	object( {{"name", "VMAX VALUE" }               }) }
		, { 0x430b,	object( {{"name", "VMAX VALUE" }               }) }
		, { 0x430c,	object( {{"name", "VMIN VALUE" }               }) }
		, { 0x430d,	object( {{"name", "VMIN VALUE" }               }) }
		// JPEG control               }) }
		, { 0x4400,	object( {{"name", "JPEG CTRL00" }               }) }
		, { 0x4401,	object( {{"name", "JPEG CTRL01" }               }) }
		, { 0x4402,	object( {{"name", "JPEG CTRL02" }               }) }
		, { 0x4403,	object( {{"name", "JPEG CTRL03" }               }) }
		, { 0x4404,	object( {{"name", "JPEG CTRL04" }               }) }
		, { 0x4405,	object( {{"name", "JPEG CTRL05" }               }) }
		, { 0x4406,	object( {{"name", "JPEG CTRL06" }               }) }
		, { 0x4407,	object( {{"name", "JPEG CTRL07" }               }) }
		, { 0x4408,	object( {{"name", "JPEG ISI CTRL" }               }) }
		, { 0x4409,	object( {{"name", "JPEG CTRL09" }               }) }
		, { 0x440a,	object( {{"name", "JPEG CTRL0A" }               }) }
		, { 0x440b,	object( {{"name", "JPEG CTRL0B" }               }) }
		, { 0x440c,	object( {{"name", "JPEG CTRL0C" }               }) }
		, { 0x4410,	object( {{"name", "JPEG QT DATA" }               }) }
		, { 0x4411,	object( {{"name", "JPEG QT ADDR" }               }) }
		, { 0x4412,	object( {{"name", "JPEG ISI ADDR" }               }) }
		, { 0x4413,	object( {{"name", "JPEG ISI CTRL" }               }) }
		, { 0x4414,	object( {{"name", "JPEG LENGTH" }               }) }
		, { 0x4415,	object( {{"name", "JPEG LENGTH" }               }) }
		, { 0x4416,	object( {{"name", "JPEG LENGTH" }               }) }
		, { 0x4417,	object( {{"name", "JFIFO OVERFLOW" }               }) }
		, { 0x4420,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4421,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4422,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4423,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4424,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4425,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4426,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4427,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4428,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4429,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x442a,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x442b,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x442c,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x442d,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x442e,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x442f,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4430,	object( {{"name", "JPEG COMMENT" }               }) }
		, { 0x4431,	object( {{"name", "JPEG COMMENT" }               }) }
		// VFIFO control               }) }
		, { 0x4600,	object( {{"name", "VFIFO CTRL00" }               }) }
		, { 0x4602,	object( {{"name", "VFIFO HSIZE" }               }) }
		, { 0x4603,	object( {{"name", "VFIFO HSIZE" }               }) }
		, { 0x4604,	object( {{"name", "VFIFO VSIZE" }               }) }
		, { 0x4605,	object( {{"name", "VFIFO VSIZE" }               }) }
		, { 0x460c,	object( {{"name", "VFIFO CTRL0C" }               }) }
		, { 0x460d,	object( {{"name", "VFIFO CTRL0D" }               }) }
		, { 0x4709,	object( {{"name", "DVP VYSNC WIDTH0" }               }) }
		, { 0x470a,	object( {{"name", "DVP VYSNC WIDTH1" }               }) }
		, { 0x470b,	object( {{"name", "DVP VYSNC WIDTH2" }               }) }
		, { 0x4711,	object( {{"name", "PAD LEFT CTRL" }               }) }
		, { 0x4712,	object( {{"name", "PAD LEFT CTRL" }               }) }
		, { 0x4713,	object( {{"name", "JPG MODE SELECT" }               }) }
		, { 0x4715,	object( {{"name", "656 DUMMY LINE" }               }) }
		, { 0x4719, object( {{"name", "CCIR656 CTRL" }               }) }
		, { 0x471b, object( {{"name", "SYNC CTRL00" }               }) }
		, { 0x471d, object( {{"name", "DVP VSYNC CTRL" }               }) }
		, { 0x471f, object( {{"name", "DVP HREF CTRL" }               }) }
		, { 0x4721, object( {{"name", "VERTICAL START OFFSET" }               }) }
		, { 0x4722, object( {{"name", "VERTICAL END OFFSET" }               }) }
		, { 0x4723, object( {{"name", "DVP CTRL23" }               }) }

		, { 0x4731, object( {{"name", "CCIR656 CTRL01" }               }) }
		, { 0x4732, object( {{"name", "CCIR656 FS" }               }) }
		, { 0x4733, object( {{"name", "CCIR656 FE" }               }) }
		, { 0x4734, object( {{"name", "CCIR656 LS" }               }) }
		, { 0x4735, object( {{"name", "CCIR656 LE" }               }) }
		, { 0x4736, object( {{"name", "CCIR656 CTRL6" }               }) }
		, { 0x4737, object( {{"name", "CCIR656 CTRL7" }               }) }
		, { 0x4738, object( {{"name", "CCIR656 CTRL8" }               }) }
		, { 0x4740, object( {{"name", "POLARITY CTRL00" }               }) }
		, { 0x4741, object( {{"name", "TEST PATTERN" }               }) }
		, { 0x4745, object( {{"name", "DATA ORDER" }               }) }
        // MIPI control               }) }
        , { 0x4800, object( {{"name", "MIPI CTRL 00" }               }) }
        , { 0x4801, object( {{"name", "MIPI CTRL 01" }               }) }
        , { 0x4805, object( {{"name", "MIPI CTRL 05" }               }) }
        , { 0x480a, object( {{"name", "MIPI DATA ORDER" }               }) }
        , { 0x4818, object( {{"name", "MIN HS ZERO H" }               }) }
        , { 0x4819, object( {{"name", "MIN HS ZERO L" }               }) }
        , { 0x481a, object( {{"name", "MIN MIPI HS TRAIL H" }               }) }
        , { 0x481b, object( {{"name", "MIN MIPI HS TRAIL L" }               }) }
        , { 0x481c, object( {{"name", "MIN MIPI CLK ZERO H" }               }) }
        , { 0x481d, object( {{"name", "MIN MIPI CLK ZERO L" }               }) }
        , { 0x481e, object( {{"name", "MIN MIPI CLK PREPARE H" }               }) }
        , { 0x481f, object( {{"name", "MIN MIPI CLK PREPARE L" }               }) }
        , { 0x4820, object( {{"name", "MIN CLK POST H" }               }) }
        , { 0x4821, object( {{"name", "MIN CLK POST L" }               }) }
        , { 0x4822, object( {{"name", "MIN CLK TRAIL H" }               }) }
        , { 0x4823, object( {{"name", "MIN CLK TRAIL L" }               }) }
        , { 0x4824, object( {{"name", "MIN LPX PCLK H" }               }) }
        , { 0x4825, object( {{"name", "MIN LPX PCLK L" }               }) }

        , { 0x4826, object( {{"name", "MIN HS PREPARE H"  }               }) }
        , { 0x4827, object( {{"name", "MIN HS PREPARE L" }               }) }
        , { 0x4828, object( {{"name", "MIN HS EXIT H"               }               }) }
        , { 0x4829, object( {{"name", "MIN HS EXIT L"               }               }) }
        , { 0x482A, object( {{"name", "MIN HS ZERO/UI"              }               }) }
        , { 0x482B, object( {{"name", "MIN HS TRAIL/UI"             }               }) }
        , { 0x482C, object( {{"name", "MIN CLK ZERO/UI"             }               }) }
        , { 0x482D, object( {{"name", "MIN CLK PREPARE/UI"          }               }) }
        , { 0x482E, object( {{"name", "MIN CLK POST/UI 0x34 RW"             }               }) }
        , { 0x482F, object( {{"name", "MIN CLK TRAIL/UI 0x00 RW"            }               }) }
        , { 0x4830, object( {{"name", "MIN LPX PCLK/UI 0x00 RW"             }               }) }
        , { 0x4831, object( {{"name", "MIN HS 0x04 RW PREPARE/UI"           }               }) }
        , { 0x4832, object( {{"name", "MIN HS EXIT/UI 0x00 RW"              }               }) }
        , { 0x4837, object( {{"name", "PCLK PERIOD"  }               }) }
        , { 0x4901, object( {{"name", "FRAME CTRL01"  }               }) }
        , { 0x4902, object( {{"name", "FRAME CTRL02"  }               }) }

        , { 0x5000, object( {{"name", "ISP CONTROL 00" }               }) }
        , { 0x5001, object( {{"name", "ISP CONTROL 01" }               }) }
        , { 0x5003, object( {{"name", "ISP CONTROL 03" }               }) }
        , { 0x5005, object( {{"name", "ISP CONTROL 05" }               }) }
        , { 0x501d, object( {{"name", "ISP MISC" }               }) }
        , { 0x501e, object( {{"name", "ISP MISC" }               }) }
        , { 0x501f, object( {{"name", "FORMAT MUX CONTROL" }               }) }
        , { 0x5020, object( {{"name", "DITHER CTRO 0" }               }) }
        , { 0x5027, object( {{"name", "DRAW WINDOW 0x02 RW CONTROL 00" }               }) }
        , { 0x5028, object( {{"name",   "DRAW WINDOW LEFT POSITION CONTROL"	}               }) }
        , { 0x5029, object( {{"name",   "DRAW WINDOW LEFT POSITION CONTROL"	}               }) }
        , { 0x502A, object( {{"name",   "DRAW WINDOW RIGHT POSITION CONTROL"	}               }) }
        , { 0x502B, object( {{"name",   "DRAW WINDOW RIGHT POSITION CONTROL"	}               }) }
        , { 0x502C, object( {{"name",   "DRAW WINDOW TOP POSITION CONTROL"	}               }) }
        , { 0x502D, object( {{"name",   "DRAW WINDOW TOP POSITION CONTROL"	}               }) }
        , { 0x502E, object( {{"name",   "DRAW WINDOW BOTTOM POSITION RW CONTROL"	}               }) }
        , { 0x502F, object( {{"name",   "DRAW WINDOW BOTTOM POSITION RW CONTROL"	}               }) }
        , { 0x5030, object( {{"name",   "DRAW WINDOW HORIZONTAL BOUNDARY WIDTH CONTROL"	}               }) }
        , { 0x5031, object( {{"name",   "DRAW WINDOW HORIZONTAL BOUNDARY WIDTH CONTROL"	}               }) }
        , { 0x5032, object( {{"name",   "DRAW WINDOW VERTICAL RW BOUNDARY WIDTH CONTROL"	}               }) }

        , { 0x5033, object( {{"name", "DRAW WINDOW VERTICAL BOUNDARY WIDTH CONTROL"	}               }) }
        , { 0x5034, object( {{"name", "DRAW WINDOW Y CONTROL"	}               }) }
        , { 0x5035, object( {{"name", "DRAW WINDOW U CONTROL"	}               }) }
        , { 0x5036, object( {{"name", "DRAW WINDOW V CONTROL"	}               }) }
        , { 0x503D, object( {{"name", "PRE ISP TEST SETTING 1"	}               }) }
        , { 0x5061, object( {{"name", "ISP SENSOR BIAS I"	}               }) }
        , { 0x5062, object( {{"name", "ISP SENSOR GAIN I"	}               }) }
        , { 0x5063, object( {{"name", "ISP SENSOR GAIN I"	}               }) }

        , { 0x5180, object( {{"name",  "AWB CONTROL 00"	}               }) }
        , { 0x5181, object( {{"name",  "AWB CONTROL 01"	}               }) }
        , { 0x5182, object( {{"name",  "AWB CONTROL 02"	}               }) }
        , { 0x5183, object( {{"name",  "AWB CONTROL 03"	}               }) }
        , { 0x5184, object( {{"name",  "AWB CONTROL 04"	}               }) }
        , { 0x5185, object( {{"name",  "AWB CONTROL 05"	}               }) }
        , { 0x5191, object( {{"name",  "AWB CONTROL 17"	}               }) }
        , { 0x5192, object( {{"name",  "AWB CONTROL 18"	}               }) }
        , { 0x5193, object( {{"name",  "AWB CONTROL 19"	}               }) }
        , { 0x5194, object( {{"name",  "AWB CONTROL 20"	}               }) }
        , { 0x5195, object( {{"name", "AWB CONTROL 20"	}               }) }

        , { 0x5196, object( {{"name", "AWB CONTROL 22"	}               }) }
        , { 0x5197, object( {{"name", "AWB CONTROL 23"	}               }) }
        , { 0x519E, object( {{"name", "AWB CONTROL 30"	}               }) }
        , { 0x519F, object( {{"name", "AWB CURRENT R GAIN"	}               }) }
        , { 0x51A0, object( {{"name", "AWB CURRENT R GAIN"	}               }) }
        , { 0x51A1, object( {{"name", "AWB CURRENT G GAIN"	}               }) }
        , { 0x51A2, object( {{"name", "AWB CURRENT G GAIN"	}               }) }
        , { 0x51A3, object( {{"name", "AWB CURRENT B GAIN"	}               }) }
        , { 0x51A4, object( {{"name", "AWB CURRENT B GAIN"	}               }) }
        , { 0x51A5, object( {{"name", "AWB AVERAGE B"	}               }) }
        , { 0x51A6, object( {{"name", "AWB AVERAGE B"	}               }) }
        , { 0x51A7, object( {{"name", "AWB AVERAGE B"	}               }) }
        , { 0x51D0, object( {{"name", "AWB CONTROL74"	}               }) }

        , { 0x5300, object( {{"name", "CIP SHARPENMT THRESHOLD 1"	}               }) }
        , { 0x5301, object( {{"name", "CIP SHARPENMT THRESHOLD 2"	}               }) }
        , { 0x5302, object( {{"name", "CIP SHARPENMT OFFSET1"	}               }) }
        , { 0x5303, object( {{"name", "CIP SHARPENMT OFFSET2"	}               }) }
        , { 0x5304, object( {{"name", "CIP DNS THRESHOLD 1"	}               }) }
        , { 0x5305, object( {{"name", "CIP DNS THRESHOLD 2"	}               }) }
        , { 0x5306, object( {{"name", "CIP DNS OFFSET1"	}               }) }
        , { 0x5307, object( {{"name", "CIP DNS OFFSET2"	}               }) }
        , { 0x5308, object( {{"name", "CIP CTRL"	}               }) }
        , { 0x5309, object( {{"name", "CIP SHARPENTH THRESHOLD 1"	}               }) }
        , { 0x530A, object( {{"name", "CIP SHARPENTH THRESHOLD 2"	}               }) }
        , { 0x530B, object( {{"name", "CIP SHARPENTH OFFSET1"	}               }) }
        , { 0x530C, object( {{"name", "CIP SHARPENTH OFFSET2"	}               }) }
        , { 0x530D, object( {{"name", "CIP EDGE MT AUTO"	}               }) }
        , { 0x530E, object( {{"name", "CIP DNS THRESHOLD AUTO"	}               }) }
        , { 0x530F, object( {{"name", "CIP SHARPEN THRESHOLD AUTO"	}               }) }

        , { 0x5380, object( {{"name", "CMX CTRL"	}               }) }
        , { 0x5381, object( {{"name", "CMX1"	}               }) }
        , { 0x5382, object( {{"name", "CMX2"	}               }) }
        , { 0x5383, object( {{"name", "CMX3"	}               }) }
        , { 0x5384, object( {{"name", "CMX4"	}               }) }
        , { 0x5385, object( {{"name", "CMX5"	}               }) }
        , { 0x5386, object( {{"name", "CMX6"	}               }) }
        , { 0x5387, object( {{"name", "CMX7"	}               }) }
        , { 0x5388, object( {{"name", "CMX8"	}               }) }
        , { 0x5389, object( {{"name", "CMX9"	}               }) }
        , { 0x538A, object( {{"name", "CMXSIGN"	}               }) }
        , { 0x538B, object( {{"name", "CMXSIGN"	}               }) }

        , { 0x5480, object( {{"name", "GAMMA CONTROL00"	}               }) }
        , { 0x5481, object( {{"name", "GAMMA YST00"	}               }) }
        , { 0x5482, object( {{"name", "GAMMA YST01"	}               }) }
        , { 0x5483, object( {{"name", "GAMMA YST02"	}               }) }
        , { 0x5484, object( {{"name", "GAMMA YST03"	}               }) }
        , { 0x5485, object( {{"name", "GAMMA YST04"	}               }) }
        , { 0x5486, object( {{"name", "GAMMA YST05"	}               }) }
        , { 0x5487, object( {{"name", "GAMMA YST06"	}               }) }
        , { 0x5488, object( {{"name", "GAMMA YST07"	}               }) }
        , { 0x5489, object( {{"name", "GAMMA YST08"	}          }) }
        , { 0x548A, object( {{"name", "GAMMA YST09"	}          }) }
        , { 0x548B, object( {{"name", "GAMMA YST0A"	}          }) }
        , { 0x548C, object( {{"name", "GAMMA YST0B"	}          }) }
        , { 0x548D, object( {{"name", "GAMMA YST0C"	}          }) }
        , { 0x548E, object( {{"name", "GAMMA YST0D"	}          }) }
        , { 0x548F, object( {{"name", "GAMMA YST0E"	}          }) }
        , { 0x5490, object( {{"name", "GAMMA YST0F"	}          }) }

        , { 0x5580, object( {{"name", "SDE CTRL0"	}          }) }
        , { 0x5581, object( {{"name", "SDE CTRL1"	}          }) }
        , { 0x5582, object( {{"name", "SDE CTRL2"	}          }) }
        , { 0x5583, object( {{"name", "SDE CTRL3"	}          }) }
        , { 0x5584, object( {{"name", "SDE CTRL4"	}          }) }
        , { 0x5585, object( {{"name", "SDE CTRL5"	}          }) }
        , { 0x5586, object( {{"name", "SDE CTRL6"	}          }) }
        , { 0x5587, object( {{"name", "SDE CTRL7"	}          }) }

        , { 0x5588, object( {{"name", "SDE CTRL8"	}                 }) }
        , { 0x5589, object( {{"name", "SDE CTRL9"	}                 }) }
        , { 0x558A, object( {{"name", "SDE CTRL10"	}             }) }
        , { 0x558B, object( {{"name", "SDE CTRL11"	}             }) }
        , { 0x558C, object( {{"name", "SDE CTRL12"	}             }) }

        , { 0x5600, object( {{"name", "SCALE CTRL 0"	}             }) }
        , { 0x5601, object( {{"name", "SCALE CTRL 1"	}             }) }
        , { 0x5602, object( {{"name", "SCALE CTRL 2"	}          }) }
        , { 0x5603, object( {{"name", "SCALE CTRL 3"	}          }) }
        , { 0x5604, object( {{"name", "SCALE CTRL 4"	}          }) }
        , { 0x5605, object( {{"name", "SCALE CTRL 5"	}          }) }
        , { 0x5606, object( {{"name", "SCALE CTRL 6"	}          }) }

        , { 0x5680, object( {{"name", "X START"	}          }) }
        , { 0x5681, object( {{"name", "X START"	}          }) }
        , { 0x5682, object( {{"name", "Y START"	}          }) }
        , { 0x5683, object( {{"name", "Y START"	}          }) }
        , { 0x5684, object( {{"name", "X WINDOW"	}          }) }
        , { 0x5685, object( {{"name", "X WINDOW"	}          }) }
        , { 0x5686, object( {{"name", "Y WINDOW"	}          }) }
        , { 0x5687, object( {{"name", "Y WINDOW"	}          }) }
        , { 0x5688, object( {{"name", "WEIGHT00"	}          }) }

        , { 0x5689, object( {{"name", "WEIGHT01"	}          }) }
        , { 0x568A, object( {{"name", "WEIGHT02"	}          }) }
        , { 0x568B, object( {{"name", "WEIGHT03"	}          }) }
        , { 0x568C, object( {{"name", "WEIGHT04"	}          }) }
        , { 0x568D, object( {{"name", "WEIGHT05"	}          }) }
        , { 0x568E, object( {{"name", "WEIGHT06"	}          }) }
        , { 0x568F, object( {{"name", "WEIGHT07"	}          }) }
        , { 0x5690, object( {{"name", "AVG CTRL10"	}          }) }
        , { 0x5691, object( {{"name",  "AVG WIN 00"	}          }) }
        , { 0x5692, object( {{"name",  "AVG WIN 01"	}          }) }
        , { 0x5693, object( {{"name", "AVG WIN 02"	}          }) }
        , { 0x5694, object( {{"name", "AVG WIN 03"	}          }) }
        , { 0x5695, object( {{"name", "AVG WIN 10"	}          }) }
        , { 0x5696, object( {{"name", "AVG WIN 11"	}          }) }
        , { 0x5697, object( {{"name", "AVG WIN 12"	}          }) }
        , { 0x5698, object( {{"name", "AVG WIN 13"	}          }) }
        , { 0x5699, object( {{"name", "AVG WIN 20"	}          }) }
        , { 0x569A, object( {{"name", "AVG WIN 21"	}          }) }
        , { 0x569B, object( {{"name", "AVG WIN 22"	}          }) }
        , { 0x569C, object( {{"name", "AVG WIN 23"	}          }) }
        , { 0x569D, object( {{"name", "AVG WIN 30"	}          }) }
        , { 0x569E, object( {{"name",  "AVG WIN 31"	}          }) }
        , { 0x569F, object( {{"name", "AVG WIN 32"	}          }) }
        , { 0x56A0, object( {{"name", "AVG WIN 33"	}          }) }
        , { 0x56a1, object( {{"name", "AVG READOUT"	}          }) }
        , { 0x56a2, object( {{"name", "AVG WEIGHT SUM"	}      }) }
    };

    const std::vector< std::pair< const uint16_t, const uint8_t > > __cfg_init = {
		{0x3008, 0x42}      //[7]=0 Software reset; [6]=1 Software power down; Default=0x02
		, {0x3103, 0x03}    //[1]=1 System input clock from PLL; Default read = 0x11
		, {0x3017, 0x00}    //[3:0]=0000 MD2P,MD2N,MCP,MCN input; Default=0x00
		, {0x3018, 0x00}    //[7:2]=000000 MD1P,MD1N, D3:0 input; Default=0x00
		, {0x3034, 0x18}    //[6:4]=001 PLL charge pump, [3:0]=1000 MIPI 8-bit mode
		//PLL1 configuration
		, {0x3035, 0x11}    //[7:4]=0001 System clock divider /1, [3:0]=0001 Scale divider for MIPI /1
		, {0x3036, 0x38}    //[7:0]=56 PLL multiplier
		, {0x3037, 0x11}    //[4]=1 PLL root divider /2, [3:0]=1 PLL pre-divider /1
		, {0x3108, 0x01}    //[5:4]=00 PCLK root divider /1, [3:2]=00 SCLK2x root divider /1, [1:0]=01 SCLK root divider /2
		//PLL2 configuration
		, {0x303D, 0x10}	//[5:4]=01 PRE_DIV_SP /1.5, [2]=1 R_DIV_SP /1, [1:0]=00 DIV12_SP /1
		, {0x303B, 0x19}    //[4:0]=11001 PLL2 multiplier DIV_CNT5B = 25
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
		, {0x460c, 0x20}    //[1]=0 DVP PCLK divider manual control by 0x3824[4:0]
        , {0x3824, 0x01}    //[4:0]=1 SCALE_DIV=INT(3824[4:0]/2)
		, {0x5000, 0x07}
		, {0x5001, 0x03}
	};

    // config_word_t const cfg_1080p_30fps_[] =
    const std::vector< std::pair< const uint16_t, const uint8_t > > __cfg_1080p_30fps = {
        //1920 x 1080 @ 30fps, RAW10, MIPISCLK=420, SCLK=84MHz, PCLK=84M
		//PLL1 configuration
		//[7:4]=0010 System clock divider /2, [3:0]=0001 Scale divider for MIPI /1
		{0x3035, 0x21} // 30fps setting
		//[7:0]=105 PLL multiplier
		, {0x3036, 0x69}
		//[4]=0 PLL root divider /1, [3:0]=5 PLL pre-divider /1.5
		, {0x3037, 0x05}
		//[5:4]=01 PCLK root divider /2, [3:2]=00 SCLK2x root divider /1, [1:0]=01 SCLK root divider /2
		, {0x3108, 0x11}

		//[6:4]=001 PLL charge pump, [3:0]=1010 MIPI 10-bit mode
		, {0x3034, 0x1A}

		//[3:0]=0 X address start high byte
		, {0x3800, (336 >> 8) & 0x0F}
		//[7:0]=0 X address start low byte
		, {0x3801, 336 & 0xFF}
		//[2:0]=0 Y address start high byte
		, {0x3802, (426 >> 8) & 0x07}
		//[7:0]=0 Y address start low byte
		, {0x3803, 426 & 0xFF}

		//[3:0] X address end high byte
		, {0x3804, (2287 >> 8) & 0x0F}
		//[7:0] X address end low byte
		, {0x3805, 2287 & 0xFF}
		//[2:0] Y address end high byte
		, {0x3806, (1529 >> 8) & 0x07}
		//[7:0] Y address end low byte
		, {0x3807, 1529 & 0xFF}

		//[3:0]=0 timing hoffset high byte
		, {0x3810, (16 >> 8) & 0x0F}
		//[7:0]=0 timing hoffset low byte
		, {0x3811, 16 & 0xFF}
		//[2:0]=0 timing voffset high byte
		, {0x3812, (12 >> 8) & 0x07}
		//[7:0]=0 timing voffset low byte
		, {0x3813, 12 & 0xFF}

		//[3:0] Output horizontal width high byte
		, {0x3808, (1920 >> 8) & 0x0F}
		//[7:0] Output horizontal width low byte
		, {0x3809, 1920 & 0xFF}
		//[2:0] Output vertical height high byte
		, {0x380a, (1080 >> 8) & 0x7F}
		//[7:0] Output vertical height low byte
		, {0x380b, 1080 & 0xFF}

		//HTS line exposure time in # of pixels Tline=HTS/sclk
		, {0x380c, (2500 >> 8) & 0x1F}
		, {0x380d, 2500 & 0xFF}
		//VTS frame exposure time in # lines
		, {0x380e, (1120 >> 8) & 0xFF}
		, {0x380f, 1120 & 0xFF}

		//[7:4]=0x1 horizontal odd subsample increment, [3:0]=0x1 horizontal even subsample increment
		, {0x3814, 0x11}
		//[7:4]=0x1 vertical odd subsample increment, [3:0]=0x1 vertical even subsample increment
		, {0x3815, 0x11}

		//[2]=0 ISP mirror, [1]=0 sensor mirror, [0]=0 no horizontal binning
		, {0x3821, 0x00}

		//little MIPI shit: global timing unit, period of PCLK in ns * 2(depends on # of lanes)
		, {0x4837, 24} // 1/84M*2

		//Undocumented anti-green settings
		, {0x3618, 0x00} // Removes vertical lines appearing under bright light
		, {0x3612, 0x59}
		, {0x3708, 0x64}
		, {0x3709, 0x52}
		, {0x370c, 0x03}

		//[7:4]=0x0 Formatter RAW, [3:0]=0x0 BGBG/GRGR
		, {0x4300, 0x00}
		//[2:0]=0x3 Format select ISP RAW (DPC)
		, {0x501f, 0x03}
	};

    const std::vector< reg_value > __ov5640_init_setting_30fps_VGA = {
        {0x3103, 0x11, 0, 0}, {0x3008, 0x82, 0, 5}, {0x3008, 0x42, 0, 0},
        {0x3103, 0x03, 0, 0}, {0x3630, 0x36, 0, 0},
        {0x3631, 0x0e, 0, 0}, {0x3632, 0xe2, 0, 0}, {0x3633, 0x12, 0, 0},
        {0x3621, 0xe0, 0, 0}, {0x3704, 0xa0, 0, 0}, {0x3703, 0x5a, 0, 0},
        {0x3715, 0x78, 0, 0}, {0x3717, 0x01, 0, 0}, {0x370b, 0x60, 0, 0},
        {0x3705, 0x1a, 0, 0}, {0x3905, 0x02, 0, 0}, {0x3906, 0x10, 0, 0},
        {0x3901, 0x0a, 0, 0}, {0x3731, 0x12, 0, 0}, {0x3600, 0x08, 0, 0},
        {0x3601, 0x33, 0, 0}, {0x302d, 0x60, 0, 0}, {0x3620, 0x52, 0, 0},
        {0x371b, 0x20, 0, 0}, {0x471c, 0x50, 0, 0}, {0x3a13, 0x43, 0, 0},
        {0x3a18, 0x00, 0, 0}, {0x3a19, 0xf8, 0, 0}, {0x3635, 0x13, 0, 0},
        {0x3636, 0x03, 0, 0}, {0x3634, 0x40, 0, 0}, {0x3622, 0x01, 0, 0},
        {0x3c01, 0xa4, 0, 0}, {0x3c04, 0x28, 0, 0}, {0x3c05, 0x98, 0, 0},
        {0x3c06, 0x00, 0, 0}, {0x3c07, 0x08, 0, 0}, {0x3c08, 0x00, 0, 0},
        {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
        {0x3820, 0x41, 0, 0}, {0x3821, 0x07, 0, 0}, {0x3814, 0x31, 0, 0},
        {0x3815, 0x31, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
        {0x3802, 0x00, 0, 0}, {0x3803, 0x04, 0, 0}, {0x3804, 0x0a, 0, 0},
        {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9b, 0, 0},
        {0x3810, 0x00, 0, 0},
        {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x06, 0, 0},
        {0x3618, 0x00, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x64, 0, 0},
        {0x3709, 0x52, 0, 0}, {0x370c, 0x03, 0, 0}, {0x3a02, 0x03, 0, 0},
        {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
        {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
        {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
        {0x4001, 0x02, 0, 0}, {0x4004, 0x02, 0, 0}, {0x3000, 0x00, 0, 0},
        {0x3002, 0x1c, 0, 0}, {0x3004, 0xff, 0, 0}, {0x3006, 0xc3, 0, 0},
        {0x302e, 0x08, 0, 0}, {0x4300, 0x3f, 0, 0},
        {0x501f, 0x00, 0, 0}, {0x4407, 0x04, 0, 0},
        {0x440e, 0x00, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
        {0x4837, 0x0a, 0, 0}, {0x3824, 0x02, 0, 0},
        {0x5000, 0xa7, 0, 0}, {0x5001, 0xa3, 0, 0}, {0x5180, 0xff, 0, 0},
        {0x5181, 0xf2, 0, 0}, {0x5182, 0x00, 0, 0}, {0x5183, 0x14, 0, 0},
        {0x5184, 0x25, 0, 0}, {0x5185, 0x24, 0, 0}, {0x5186, 0x09, 0, 0},
        {0x5187, 0x09, 0, 0}, {0x5188, 0x09, 0, 0}, {0x5189, 0x88, 0, 0},
        {0x518a, 0x54, 0, 0}, {0x518b, 0xee, 0, 0}, {0x518c, 0xb2, 0, 0},
        {0x518d, 0x50, 0, 0}, {0x518e, 0x34, 0, 0}, {0x518f, 0x6b, 0, 0},
        {0x5190, 0x46, 0, 0}, {0x5191, 0xf8, 0, 0}, {0x5192, 0x04, 0, 0},
        {0x5193, 0x70, 0, 0}, {0x5194, 0xf0, 0, 0}, {0x5195, 0xf0, 0, 0},
        {0x5196, 0x03, 0, 0}, {0x5197, 0x01, 0, 0}, {0x5198, 0x04, 0, 0},
        {0x5199, 0x6c, 0, 0}, {0x519a, 0x04, 0, 0}, {0x519b, 0x00, 0, 0},
        {0x519c, 0x09, 0, 0}, {0x519d, 0x2b, 0, 0}, {0x519e, 0x38, 0, 0},
        {0x5381, 0x1e, 0, 0}, {0x5382, 0x5b, 0, 0}, {0x5383, 0x08, 0, 0},
        {0x5384, 0x0a, 0, 0}, {0x5385, 0x7e, 0, 0}, {0x5386, 0x88, 0, 0},
        {0x5387, 0x7c, 0, 0}, {0x5388, 0x6c, 0, 0}, {0x5389, 0x10, 0, 0},
        {0x538a, 0x01, 0, 0}, {0x538b, 0x98, 0, 0}, {0x5300, 0x08, 0, 0},
        {0x5301, 0x30, 0, 0}, {0x5302, 0x10, 0, 0}, {0x5303, 0x00, 0, 0},
        {0x5304, 0x08, 0, 0}, {0x5305, 0x30, 0, 0}, {0x5306, 0x08, 0, 0},
        {0x5307, 0x16, 0, 0}, {0x5309, 0x08, 0, 0}, {0x530a, 0x30, 0, 0},
        {0x530b, 0x04, 0, 0}, {0x530c, 0x06, 0, 0}, {0x5480, 0x01, 0, 0},
        {0x5481, 0x08, 0, 0}, {0x5482, 0x14, 0, 0}, {0x5483, 0x28, 0, 0},
        {0x5484, 0x51, 0, 0}, {0x5485, 0x65, 0, 0}, {0x5486, 0x71, 0, 0},
        {0x5487, 0x7d, 0, 0}, {0x5488, 0x87, 0, 0}, {0x5489, 0x91, 0, 0},
        {0x548a, 0x9a, 0, 0}, {0x548b, 0xaa, 0, 0}, {0x548c, 0xb8, 0, 0},
        {0x548d, 0xcd, 0, 0}, {0x548e, 0xdd, 0, 0}, {0x548f, 0xea, 0, 0},
        {0x5490, 0x1d, 0, 0}, {0x5580, 0x02, 0, 0}, {0x5583, 0x40, 0, 0},
        {0x5584, 0x10, 0, 0}, {0x5589, 0x10, 0, 0}, {0x558a, 0x00, 0, 0},
        {0x558b, 0xf8, 0, 0}, {0x5800, 0x23, 0, 0}, {0x5801, 0x14, 0, 0},
        {0x5802, 0x0f, 0, 0}, {0x5803, 0x0f, 0, 0}, {0x5804, 0x12, 0, 0},
        {0x5805, 0x26, 0, 0}, {0x5806, 0x0c, 0, 0}, {0x5807, 0x08, 0, 0},
        {0x5808, 0x05, 0, 0}, {0x5809, 0x05, 0, 0}, {0x580a, 0x08, 0, 0},
        {0x580b, 0x0d, 0, 0}, {0x580c, 0x08, 0, 0}, {0x580d, 0x03, 0, 0},
        {0x580e, 0x00, 0, 0}, {0x580f, 0x00, 0, 0}, {0x5810, 0x03, 0, 0},
        {0x5811, 0x09, 0, 0}, {0x5812, 0x07, 0, 0}, {0x5813, 0x03, 0, 0},
        {0x5814, 0x00, 0, 0}, {0x5815, 0x01, 0, 0}, {0x5816, 0x03, 0, 0},
        {0x5817, 0x08, 0, 0}, {0x5818, 0x0d, 0, 0}, {0x5819, 0x08, 0, 0},
        {0x581a, 0x05, 0, 0}, {0x581b, 0x06, 0, 0}, {0x581c, 0x08, 0, 0},
        {0x581d, 0x0e, 0, 0}, {0x581e, 0x29, 0, 0}, {0x581f, 0x17, 0, 0},
        {0x5820, 0x11, 0, 0}, {0x5821, 0x11, 0, 0}, {0x5822, 0x15, 0, 0},
        {0x5823, 0x28, 0, 0}, {0x5824, 0x46, 0, 0}, {0x5825, 0x26, 0, 0},
        {0x5826, 0x08, 0, 0}, {0x5827, 0x26, 0, 0}, {0x5828, 0x64, 0, 0},
        {0x5829, 0x26, 0, 0}, {0x582a, 0x24, 0, 0}, {0x582b, 0x22, 0, 0},
        {0x582c, 0x24, 0, 0}, {0x582d, 0x24, 0, 0}, {0x582e, 0x06, 0, 0},
        {0x582f, 0x22, 0, 0}, {0x5830, 0x40, 0, 0}, {0x5831, 0x42, 0, 0},
        {0x5832, 0x24, 0, 0}, {0x5833, 0x26, 0, 0}, {0x5834, 0x24, 0, 0},
        {0x5835, 0x22, 0, 0}, {0x5836, 0x22, 0, 0}, {0x5837, 0x26, 0, 0},
        {0x5838, 0x44, 0, 0}, {0x5839, 0x24, 0, 0}, {0x583a, 0x26, 0, 0},
        {0x583b, 0x28, 0, 0}, {0x583c, 0x42, 0, 0}, {0x583d, 0xce, 0, 0},
        {0x5025, 0x00, 0, 0}, {0x3a0f, 0x30, 0, 0}, {0x3a10, 0x28, 0, 0},
        {0x3a1b, 0x30, 0, 0}, {0x3a1e, 0x26, 0, 0}, {0x3a11, 0x60, 0, 0},
        {0x3a1f, 0x14, 0, 0}, {0x3008, 0x02, 0, 0}, {0x3c00, 0x04, 0, 300},
    };

    const std::vector< reg_value > __ov5640_setting_1080P_1920_1080 = {
        {0x3c07, 0x08, 0, 0},
        {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
        {0x3814, 0x11, 0, 0},
        {0x3815, 0x11, 0, 0}, {0x3800, 0x00, 0, 0}, {0x3801, 0x00, 0, 0},
        {0x3802, 0x00, 0, 0}, {0x3803, 0x00, 0, 0}, {0x3804, 0x0a, 0, 0},
        {0x3805, 0x3f, 0, 0}, {0x3806, 0x07, 0, 0}, {0x3807, 0x9f, 0, 0},
        {0x3810, 0x00, 0, 0},
        {0x3811, 0x10, 0, 0}, {0x3812, 0x00, 0, 0}, {0x3813, 0x04, 0, 0},
        {0x3618, 0x04, 0, 0}, {0x3612, 0x29, 0, 0}, {0x3708, 0x21, 0, 0},
        {0x3709, 0x12, 0, 0}, {0x370c, 0x00, 0, 0}, {0x3a02, 0x03, 0, 0},
        {0x3a03, 0xd8, 0, 0}, {0x3a08, 0x01, 0, 0}, {0x3a09, 0x27, 0, 0},
        {0x3a0a, 0x00, 0, 0}, {0x3a0b, 0xf6, 0, 0}, {0x3a0e, 0x03, 0, 0},
        {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x03, 0, 0}, {0x3a15, 0xd8, 0, 0},
        {0x4001, 0x02, 0, 0}, {0x4004, 0x06, 0, 0},
        {0x4407, 0x04, 0, 0}, {0x460b, 0x35, 0, 0}, {0x460c, 0x22, 0, 0},
        {0x3824, 0x02, 0, 0}, {0x5001, 0x83, 0, 0},
        {0x3c07, 0x07, 0, 0}, {0x3c08, 0x00, 0, 0},
        {0x3c09, 0x1c, 0, 0}, {0x3c0a, 0x9c, 0, 0}, {0x3c0b, 0x40, 0, 0},
        {0x3800, 0x01, 0, 0}, {0x3801, 0x50, 0, 0}, {0x3802, 0x01, 0, 0},
        {0x3803, 0xb2, 0, 0}, {0x3804, 0x08, 0, 0}, {0x3805, 0xef, 0, 0},
        {0x3806, 0x05, 0, 0}, {0x3807, 0xf1, 0, 0},
        {0x3612, 0x2b, 0, 0}, {0x3708, 0x64, 0, 0},
        {0x3a02, 0x04, 0, 0}, {0x3a03, 0x60, 0, 0}, {0x3a08, 0x01, 0, 0},
        {0x3a09, 0x50, 0, 0}, {0x3a0a, 0x01, 0, 0}, {0x3a0b, 0x18, 0, 0},
        {0x3a0e, 0x03, 0, 0}, {0x3a0d, 0x04, 0, 0}, {0x3a14, 0x04, 0, 0},
        {0x3a15, 0x60, 0, 0}, {0x4407, 0x04, 0, 0},
        {0x460b, 0x37, 0, 0}, {0x460c, 0x20, 0, 0}, {0x3824, 0x04, 0, 0},
        {0x4005, 0x1a, 0, 0},
    };

}

const std::vector< std::pair< const uint16_t, object > >&
ov5640::regs()
{
    return __regs;
}

const std::vector< std::pair< const uint16_t, const uint8_t > >&
ov5640::cfg_init()
{
    return __cfg_init;
}

const std::vector< std::pair< const uint16_t, const uint8_t > >&
ov5640::cfg_1080p_30fps()
{
    return __cfg_1080p_30fps;
}

const std::vector< reg_value >&
ov5640::setting_1080P_1920_1080()
{
    return __ov5640_setting_1080P_1920_1080;
}

const std::vector< reg_value >&
ov5640::init_setting_30fps_VGA()
{
    return __ov5640_init_setting_30fps_VGA;
}
