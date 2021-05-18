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

bool
ov5640::init( i2c_linux::i2c& ) const
{
#if 0
    uint8_t id_h, id_l;
    readReg(reg_ID_h, id_h);
    readReg(reg_ID_l, id_l);
    if (id_h != dev_ID_h_ || id_l != dev_ID_l_)
		{
			/* Does not work. https://www.xilinx.com/support/answers/64193.html
	      std::stringstream ss;
	      ss << "Got " << std::hex << id_h << id_l << ". Expected " << dev_ID_h_ << dev_ID_l_;
			 */
			char msg[100];
			snprintf(msg, sizeof(msg), "Got %02x %02x. Expected %02x %02x\r\n", id_h, id_l, dev_ID_h_, dev_ID_l_);
			throw HardwareError(HardwareError::WRONG_ID, msg);
		}
		//[1]=0 System input clock from pad; Default read = 0x11
		writeReg(0x3103, 0x11);
		//[7]=1 Software reset; [6]=0 Software power down; Default=0x02
		writeReg(0x3008, 0x82);

		usleep(1000000);

		size_t i;
		for (i=0;i<sizeof(OV5640_cfg::cfg_init_)/sizeof(OV5640_cfg::cfg_init_[0]); ++i)
		{
			writeReg(OV5640_cfg::cfg_init_[i].addr, OV5640_cfg::cfg_init_[i].data);
		}
#endif
        return false;
}

namespace {

    const std::vector< std::pair< const uint16_t, const char * > > __regs = {
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

        , { 0x3c01, "5060HZ CTRL01" }
        , { 0x3c02, "5060HZ CTRL02" }
        , { 0x3c03, "5060HZ CTRL03" }
        , { 0x3c04, "5060HZ CTRL04" }
        , { 0x3c05,  "5060HZ CTRL05" }
        , { 0x3c06,	"LIGHT METER1 THRESHOLD"  }
        , { 0x3c07, "LIGHT METER1 THRESHOLD" }
        , { 0x3c08, "LIGHT METER2 THRESHOLD" }
        , { 0x3c09, "LIGHT METER2 THRESHOLD" }
        , { 0x3c0a, "SAMPLE NUMBER" }
        , { 0x3c0b, "SAMPLE NUMBER" }
        , { 0x3c0c, "SIGMADELTA CTRL0C" }
        , { 0x3c0d, "SUM 50" }
        , { 0x3c0e, "SUM 50" }
        , { 0x3c0f, "SUM 50" }
        , { 0x3c10, "SUM 50" }
        , { 0x3c11, "SUM 60" }
        , { 0x3c12, "SUM 60" }
        , { 0x3c13, "SUM 60" }
        , { 0x3c14, "SUM 60" }
        , { 0x3c15, "SUM 50 60" }
        , { 0x3c16, "SUM 50 60" }
        , { 0x3c17, "BLOCK COUNTER" }
        , { 0x3c18, "BLOCK COUNTER" }
        , { 0x3c19, "B6" }
        , { 0x3c1a, "B6" }
        , { 0x3c1b, "LIGHTMETER OUTPUT" }
        , { 0x3c1c, "LIGHTMETER OUTPUT" }
        , { 0x3c1d, "LIGHTMETER OUTPUT" }
        , { 0x3c1e, "SUM THRESHOLD" }
        , { 0x3d00, "OTP DATA00" }
        , { 0x3d01, "OTP DATA01" }
        , { 0x3d02, "OTP DATA02" }
        , { 0x3d03, "OTP DATA03" }
        , { 0x3d04, "OTP DATA04" }
        , { 0x3d05, "OTP DATA05" }
        , { 0x3d06, "OTP DATA06" }
        , { 0x3d07, "OTP DATA07" }
        , { 0x3d08, "OTP DATA08" }
        , { 0x3d09, "OTP DATA09" }
        , { 0x3d0a, "OTP DATA0A" }
        , { 0x3d0b, "OTP DATA0B" }
        , { 0x3d0c, "OTP DATA0C" }
        , { 0x3d0d, "OTP DATA0D" }
        , { 0x3d0e, "OTP DATA0E" }
        , { 0x3d0f, "OTP DATA0F" }
        , { 0x3d10, "OTP DATA10" }
        , { 0x3d11, "OTP DATA11" }
        , { 0x3d12, "OTP DATA12" }
        , { 0x3d13, "OTP DATA13" }
        , { 0x3d14, "OTP DATA14" }
        , { 0x3d15, "OTP DATA15" }
        , { 0x3d16, "OTP DATA16" }
        , { 0x3d17, "OTP DATA17" }
        , { 0x3d18, "OTP DATA18" }
        , { 0x3d19, "OTP DATA19" }
        , { 0x3d1a, "OTP DATA1A" }
        , { 0x3d1b, "OTP DATA1B" }
        , { 0x3d1c, "OTP DATA1C" }
        , { 0x3d1d, "OTP DATA1D" }
        , { 0x3d1e, "OTP DATA1E" }
        , { 0x3d1f, "OTP DATA1F" }
        , { 0x3d20, "OTP PROGRAM CTRL" }
        , { 0x3d21, "OTP READ CTRL" }
        , { 0x3f00,	"MC CTRL00" }
        , { 0x3f01,	"MC INTERRUPT MASK0" }
        , { 0x3f02,	"MC INTERRUPT MASK1" }
        , { 0x3f03,	"MC READ INTERRUPT ADDRESS" }
        , { 0x3f04,	"MC READ INTERRUPT ADDRESS" }
		, { 0x3f05,	"MC WRITE INTERRUPT ADDRESS" }
		, { 0x3f06,	"MC WRITE INTERRUPT ADDRESS" }
		, { 0x3f08,	"MC INTERRUPT SOURCE SELECTION1" }

		, { 0x3f09,	"MC INTERRUPT SOURCE SELECTION2" }
		, { 0x3f0a,	"MC INTERRUPT SOURCE SELECTION3" }
		, { 0x3f0b,	"MC INTERRUPT SOURCE SELECTION4" }
		, { 0x3f0c,	"MC INTERRUPT0 STATUS" }
		, { 0x3f0d,	"MC INTERRUPT1 STATUS" }

		, { 0x4000,	"BLC CTRL00" }
		, { 0x4001,	"BLC CTRL01" }
		, { 0x4002,	"BLC CTRL02" }
		, { 0x4003,	"BLC CTRL03" }
		, { 0x4004,	"BLC CTRL04" }
		, { 0x4005,	"BLC CTRL05" }
		, { 0x4006,	"BLC CTRL06" }
		, { 0x4007,	"BLC CTRL07" }
		, { 0x4009,	"BLACK LEVEL" }
		, { 0x402c,	"BLACK LEVEL00" }
		, { 0x402d,	"BLACK LEVEL00" }
		, { 0x402e,	"BLACK LEVEL01" }
		, { 0x402f,	"BLACK LEVEL01" }
		, { 0x4030,	"BLACK LEVEL10" }
		, { 0x4031,	"BLACK LEVEL10" }
		, { 0x4032,	"BLACK LEVEL11" }
		, { 0x4033,	"BLACK LEVEL11" }
		, { 0x4201,	"FRAME CTRL01" }
		, { 0x4202,	"FRAME CTRL02" }
		, { 0x4300,	"FORMAT CONTROL 00" }
		, { 0x4301,	"FORMAT CONTROL 01" }
		, { 0x4302,	"YMAX VALUE" }
		, { 0x4303,	"YMAX VALUE" }
		, { 0x4304,	"YMIN VALUE" }
		, { 0x4305,	"YMIN VALUE" }
		, { 0x4306,	"UMAX VALUE" }
		, { 0x4307,	"UMAX VALUE" }
		, { 0x4308,	"UMIN VALUE" }
		, { 0x4309,	"UMIN VALUE" }
		, { 0x430a,	"VMAX VALUE" }
		, { 0x430b,	"VMAX VALUE" }
		, { 0x430c,	"VMIN VALUE" }
		, { 0x430d,	"VMIN VALUE" }
		// JPEG control
		, { 0x4400,	"JPEG CTRL00" }
		, { 0x4401,	"JPEG CTRL01" }
		, { 0x4402,	"JPEG CTRL02" }
		, { 0x4403,	"JPEG CTRL03" }
		, { 0x4404,	"JPEG CTRL04" }
		, { 0x4405,	"JPEG CTRL05" }
		, { 0x4406,	"JPEG CTRL06" }
		, { 0x4407,	"JPEG CTRL07" }
		, { 0x4408,	"JPEG ISI CTRL" }
		, { 0x4409,	"JPEG CTRL09" }
		, { 0x440a,	"JPEG CTRL0A" }
		, { 0x440b,	"JPEG CTRL0B" }
		, { 0x440c,	"JPEG CTRL0C" }
		, { 0x4410,	"JPEG QT DATA" }
		, { 0x4411,	"JPEG QT ADDR" }
		, { 0x4412,	"JPEG ISI ADDR" }
		, { 0x4413,	"JPEG ISI CTRL" }
		, { 0x4414,	"JPEG LENGTH" }
		, { 0x4415,	"JPEG LENGTH" }
		, { 0x4416,	"JPEG LENGTH" }
		, { 0x4417,	"JFIFO OVERFLOW" }
		, { 0x4420,	"JPEG COMMENT" }
		, { 0x4421,	"JPEG COMMENT" }
		, { 0x4422,	"JPEG COMMENT" }
		, { 0x4423,	"JPEG COMMENT" }
		, { 0x4424,	"JPEG COMMENT" }
		, { 0x4425,	"JPEG COMMENT" }
		, { 0x4426,	"JPEG COMMENT" }
		, { 0x4427,	"JPEG COMMENT" }
		, { 0x4428,	"JPEG COMMENT" }
		, { 0x4429,	"JPEG COMMENT" }
		, { 0x442a,	"JPEG COMMENT" }
		, { 0x442b,	"JPEG COMMENT" }
		, { 0x442c,	"JPEG COMMENT" }
		, { 0x442d,	"JPEG COMMENT" }
		, { 0x442e,	"JPEG COMMENT" }
		, { 0x442f,	"JPEG COMMENT" }
		, { 0x4430,	"JPEG COMMENT" }
		, { 0x4431,	"JPEG COMMENT" }
		// VFIFO control
		, { 0x4600,	"VFIFO CTRL00" }
		, { 0x4602,	"VFIFO HSIZE" }
		, { 0x4603,	"VFIFO HSIZE" }
		, { 0x4604,	"VFIFO VSIZE" }
		, { 0x4605,	"VFIFO VSIZE" }
		, { 0x460c,	"VFIFO CTRL0C" }
		, { 0x460d,	"VFIFO CTRL0D" }
		, { 0x4709,	"DVP VYSNC WIDTH0" }
		, { 0x470a,	"DVP VYSNC WIDTH1" }
		, { 0x470b,	"DVP VYSNC WIDTH2" }
		, { 0x4711,	"PAD LEFT CTRL" }
		, { 0x4712,	"PAD LEFT CTRL" }
		, { 0x4713,	"JPG MODE SELECT" }
		, { 0x4715,	"656 DUMMY LINE" }
		, { 0x4719, "CCIR656 CTRL" }
		, { 0x471b, "SYNC CTRL00" }
		, { 0x471d, "DVP VSYNC CTRL" }
		, { 0x471f, "DVP HREF CTRL" }
		, { 0x4721, "VERTICAL START OFFSET" }
		, { 0x4722, "VERTICAL END OFFSET" }
		, { 0x4723, "DVP CTRL23" }

		, { 0x4731, "CCIR656 CTRL01" }
		, { 0x4732, "CCIR656 FS" }
		, { 0x4733, "CCIR656 FE" }
		, { 0x4734, "CCIR656 LS" }
		, { 0x4735, "CCIR656 LE" }
		, { 0x4736, "CCIR656 CTRL6" }
		, { 0x4737, "CCIR656 CTRL7" }
		, { 0x4738, "CCIR656 CTRL8" }
		, { 0x4740, "POLARITY CTRL00" }
		, { 0x4741, "TEST PATTERN" }
		, { 0x4745, "DATA ORDER" }
        // MIPI control
        , { 0x4800, "MIPI CTRL 00" }
        , { 0x4801, "MIPI CTRL 01" }
        , { 0x4805, "MIPI CTRL 05" }
        , { 0x480a, "MIPI DATA ORDER" }
        , { 0x4818, "MIN HS ZERO H" }
        , { 0x4819, "MIN HS ZERO L" }
        , { 0x481a, "MIN MIPI HS TRAIL H" }
        , { 0x481b, "MIN MIPI HS TRAIL L" }
        , { 0x481c, "MIN MIPI CLK ZERO H" }
        , { 0x481d, "MIN MIPI CLK ZERO L" }
        , { 0x481e, "MIN MIPI CLK PREPARE H" }
        , { 0x481f, "MIN MIPI CLK PREPARE L" }
        , { 0x4820, "MIN CLK POST H" }
        , { 0x4821, "MIN CLK POST L" }
        , { 0x4822, "MIN CLK TRAIL H" }
        , { 0x4823, "MIN CLK TRAIL L" }
        , { 0x4824, "MIN LPX PCLK H" }
        , { 0x4825, "MIN LPX PCLK L" }

        , { 0x4826, "MIN HS PREPARE H"  }
        , { 0x4827, "MIN HS PREPARE L" }
        , { 0x4828, "MIN HS EXIT H"               }
        , { 0x4829, "MIN HS EXIT L"               }
        , { 0x482A, "MIN HS ZERO/UI"              }
        , { 0x482B, "MIN HS TRAIL/UI"             }
        , { 0x482C, "MIN CLK ZERO/UI"             }
        , { 0x482D, "MIN CLK PREPARE/UI"          }
        , { 0x482E, "MIN CLK POST/UI 0x34 RW"             }
        , { 0x482F, "MIN CLK TRAIL/UI 0x00 RW"            }
        , { 0x4830, "MIN LPX PCLK/UI 0x00 RW"             }
        , { 0x4831, "MIN HS 0x04 RW PREPARE/UI"           }
        , { 0x4832, "MIN HS EXIT/UI 0x00 RW"              }
        , { 0x4837, "PCLK PERIOD"  }
        , { 0x4901, "FRAME CTRL01"  }
        , { 0x4902, "FRAME CTRL02"  }
        //
        , { 0x5000, "ISP CONTROL 00" }
        , { 0x5001, "ISP CONTROL 01" }
        , { 0x5003, "ISP CONTROL 03" }
        , { 0x5005, "ISP CONTROL 05" }
        , { 0x501d, "ISP MISC" }
        , { 0x501e, "ISP MISC" }
        , { 0x501f, "FORMAT MUX CONTROL" }
        , { 0x5020, "DITHER CTRO 0" }
        , { 0x5027, "DRAW WINDOW 0x02 RW CONTROL 00" }
        , { 0x5028,   "DRAW WINDOW LEFT POSITION CONTROL"	}
        , { 0x5029,   "DRAW WINDOW LEFT POSITION CONTROL"	}
        , { 0x502A,   "DRAW WINDOW RIGHT POSITION CONTROL"	}
        , { 0x502B,   "DRAW WINDOW RIGHT POSITION CONTROL"	}
        , { 0x502C,   "DRAW WINDOW TOP POSITION CONTROL"	}
        , { 0x502D,   "DRAW WINDOW TOP POSITION CONTROL"	}
        , { 0x502E,   "DRAW WINDOW BOTTOM POSITION RW CONTROL"	}
        , { 0x502F,   "DRAW WINDOW BOTTOM POSITION RW CONTROL"	}
        , { 0x5030,   "DRAW WINDOW HORIZONTAL BOUNDARY WIDTH CONTROL"	}
        , { 0x5031,   "DRAW WINDOW HORIZONTAL BOUNDARY WIDTH CONTROL"	}
        , { 0x5032,   "DRAW WINDOW VERTICAL RW BOUNDARY WIDTH CONTROL"	}
        , { 	}
        , { 0x5033, "DRAW WINDOW VERTICAL BOUNDARY WIDTH CONTROL"	}
        , { 0x5034, "DRAW WINDOW Y CONTROL"	}
        , { 0x5035, "DRAW WINDOW U CONTROL"	}
        , { 0x5036, "DRAW WINDOW V CONTROL"	}
        , { 0x503D, "PRE ISP TEST SETTING 1"	}
        , { 0x5061, "ISP SENSOR BIAS I"	}
        , { 0x5062, "ISP SENSOR GAIN I"	}
        , { 0x5063, "ISP SENSOR GAIN I"	}
        , { 	}
        , { 0x5180,  "AWB CONTROL 00"	}
        , { 0x5181,  "AWB CONTROL 01"	}
        , { 0x5182,  "AWB CONTROL 02"	}
        , { 0x5183,  "AWB CONTROL 03"	}
        , { 0x5184,  "AWB CONTROL 04"	}
        , { 0x5185,  "AWB CONTROL 05"	}
        , { 0x5191,  "AWB CONTROL 17"	}
        , { 0x5192,  "AWB CONTROL 18"	}
        , { 0x5193,  "AWB CONTROL 19"	}
        , { 0x5194,  "AWB CONTROL 20"	}
        , { 0x5195,  "AWB CONTROL 20"	}

        , { 0x5196,  "AWB CONTROL 22"	}
        , { 0x5197,  "AWB CONTROL 23"	}
        , { 0x519E,  "AWB CONTROL 30"	}
        , { 0x519F,  "AWB CURRENT R GAIN"	}
        , { 0x51A0,  "AWB CURRENT R GAIN"	}
        , { 0x51A1,  "AWB CURRENT G GAIN"	}
        , { 0x51A2,  "AWB CURRENT G GAIN"	}
        , { 0x51A3,  "AWB CURRENT B GAIN"	}
        , { 0x51A4,  "AWB CURRENT B GAIN"	}
        , { 0x51A5,  "AWB AVERAGE B"	}
        , { 0x51A6,  "AWB AVERAGE B"	}
        , { 0x51A7,  "AWB AVERAGE B"	}
        , { 0x51D0,  "AWB CONTROL74"	}

        , { 0x5300,  "CIP SHARPENMT THRESHOLD 1"	}
        , { 0x5301,  "CIP SHARPENMT THRESHOLD 2"	}
        , { 0x5302,  "CIP SHARPENMT OFFSET1"	}
        , { 0x5303,  "CIP SHARPENMT OFFSET2"	}
        , { 0x5304,  "CIP DNS THRESHOLD 1"	}
        , { 0x5305,  "CIP DNS THRESHOLD 2"	}
        , { 0x5306,  "CIP DNS OFFSET1"	}
        , { 0x5307,  "CIP DNS OFFSET2"	}
        , { 0x5308,  "CIP CTRL"	}
        , { 0x5309,  "CIP SHARPENTH THRESHOLD 1"	}
        , { 0x530A,  "CIP SHARPENTH THRESHOLD 2"	}
        , { 0x530B,  "CIP SHARPENTH OFFSET1"	}
        , { 0x530C,  "CIP SHARPENTH OFFSET2"	}
        , { 0x530D,  "CIP EDGE MT AUTO"	}
        , { 0x530E,  "CIP DNS THRESHOLD AUTO"	}
        , { 0x530F,  "CIP SHARPEN THRESHOLD AUTO"	}

        , { 0x5380,  "CMX CTRL"	}
        , { 0x5381,  "CMX1"	}
        , { 0x5382,  "CMX2"	}
        , { 0x5383,  "CMX3"	}
        , { 0x5384,  "CMX4"	}
        , { 0x5385,  "CMX5"	}
        , { 0x5386,  "CMX6"	}
        , { 0x5387,  "CMX7"	}
        , { 0x5388,  "CMX8"	}
        , { 0x5389,  "CMX9"	}
        , { 0x538A,  "CMXSIGN"	}
        , { 0x538B,  "CMXSIGN"	}

        , { 0x5480,   "GAMMA CONTROL00"	}
        , { 0x5481,   "GAMMA YST00"	}
        , { 0x5482,   "GAMMA YST01"	}
        , { 0x5483,   "GAMMA YST02"	}
        , { 0x5484,   "GAMMA YST03"	}
        , { 0x5485,   "GAMMA YST04"	}
        , { 0x5486,   "GAMMA YST05"	}
        , { 0x5487,   "GAMMA YST06"	}
        , { 0x5488,   "GAMMA YST07"	}
        , { 0x5489,   "GAMMA YST08"	}
        , { 0x548A,   "GAMMA YST09"	}
        , { 0x548B,   "GAMMA YST0A"	}
        , { 0x548C,   "GAMMA YST0B"	}
        , { 0x548D,   "GAMMA YST0C"	}
        , { 0x548E,   "GAMMA YST0D"	}
        , { 0x548F,   "GAMMA YST0E"	}
        , { 0x5490,   "GAMMA YST0F"	}

        , { 0x5580,   "SDE CTRL0"	}
        , { 0x5581,   "SDE CTRL1"	}
        , { 0x5582,   "SDE CTRL2"	}
        , { 0x5583,   "SDE CTRL3"	}
        , { 0x5584,   "SDE CTRL4"	}
        , { 0x5585,   "SDE CTRL5"	}
        , { 0x5586,   "SDE CTRL6"	}
        , { 0x5587,   "SDE CTRL7"	}

        , { 0x5588,  "SDE CTRL8"	}
        , { 0x5589,  "SDE CTRL9"	}
        , { 0x558A,  "SDE CTRL10"	}
        , { 0x558B,  "SDE CTRL11"	}
        , { 0x558C,  "SDE CTRL12"	}

        , { 0x5600,   "SCALE CTRL 0"	}
        , { 0x5601,   "SCALE CTRL 1"	}
        , { 0x5602,   "SCALE CTRL 2"	}
        , { 0x5603,   "SCALE CTRL 3"	}
        , { 0x5604,   "SCALE CTRL 4"	}
        , { 0x5605,   "SCALE CTRL 5"	}
        , { 0x5606,   "SCALE CTRL 6"	}

        , { 0x5680,     "X START"	}
        , { 0x5681,     "X START"	}
        , { 0x5682,     "Y START"	}
        , { 0x5683,     "Y START"	}
        , { 0x5684,     "X WINDOW"	}
        , { 0x5685,     "X WINDOW"	}
        , { 0x5686,     "Y WINDOW"	}
        , { 0x5687,     "Y WINDOW"	}
        , { 0x5688,     "WEIGHT00"	}

        , { 0x5689,    "WEIGHT01"	}
        , { 0x568A,    "WEIGHT02"	}
        , { 0x568B,    "WEIGHT03"	}
        , { 0x568C,    "WEIGHT04"	}
        , { 0x568D,    "WEIGHT05"	}
        , { 0x568E,    "WEIGHT06"	}
        , { 0x568F,    "WEIGHT07"	}
        , { 0x5690,    "AVG CTRL10"	}
        , { 0x5691,    "AVG WIN 00"	}
        , { 0x5692,    "AVG WIN 01"	}
        , { 0x5693,    "AVG WIN 02"	}
        , { 0x5694,    "AVG WIN 03"	}
        , { 0x5695,    "AVG WIN 10"	}
        , { 0x5696,    "AVG WIN 11"	}
        , { 0x5697,    "AVG WIN 12"	}
        , { 0x5698,    "AVG WIN 13"	}
        , { 0x5699,    "AVG WIN 20"	}
        , { 0x569A,    "AVG WIN 21"	}
        , { 0x569B,    "AVG WIN 22"	}
        , { 0x569C,    "AVG WIN 23"	}
        , { 0x569D,    "AVG WIN 30"	}
        , { 0x569E,    "AVG WIN 31"	}
        , { 0x569F,    "AVG WIN 32"	}
        , { 0x56A0,    "AVG WIN 33"	}
        , { 0x56a1,    "AVG READOUT"	}
        , { 0x56a2,    "AVG WEIGHT SUM"	}
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

}

const std::vector< std::pair< const uint16_t, const char * > >&
ov5640::regs()
{
    return __regs;
}

const std::vector< std::pair< const uint16_t, const uint8_t > >&
ov5640::cfg_init()
{
    return __cfg_init;
}
