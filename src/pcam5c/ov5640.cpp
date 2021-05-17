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
