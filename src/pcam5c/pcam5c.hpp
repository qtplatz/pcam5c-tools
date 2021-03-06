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

#pragma once

#include <boost/json.hpp>
#include <chrono>
#include <optional>
#include <vector>

using namespace std::chrono_literals;

namespace i2c_linux {
    class i2c;
}

class pcam5c {
    void  pprint( std::ostream&,  const std::pair< const uint16_t, boost::json::object >& reg, uint8_t value ) const;
    void  pprint( std::ostream&,  uint16_t reg, uint8_t value ) const;
public:
    bool read_all( i2c_linux::i2c& );
    bool startup( i2c_linux::i2c& );
    void read_regs( i2c_linux::i2c&, const std::vector< std::string >& );
    //bool write_reg( i2c_linux::i2c&, uint16_t reg, uint8_t val, bool verbose = true ) const;
    bool write_reg( i2c_linux::i2c&, const std::pair<uint16_t,uint8_t>&, bool verbose = true ) const;

    bool gpio_state() const;
    bool gpio_value( bool ) const;
    bool gpio_reset( std::chrono::milliseconds twait = 5ms ) const; // off --> on (ov5640 manual describes 1ms wait)

    std::optional< uint32_t > get_sysclk( i2c_linux::i2c& );
    std::optional< uint32_t > get_light_freq(  i2c_linux::i2c& iic );
};
