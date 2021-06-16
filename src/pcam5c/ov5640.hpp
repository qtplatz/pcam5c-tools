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

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>
#include <boost/json.hpp>

namespace i2c_linux { class i2c; }
constexpr static std::pair< uint8_t, uint8_t > ov5640_chipid_t = { 0x56, 0x40 };

struct reg_value {
	uint16_t reg_addr;
	uint8_t val;
	uint8_t mask;
	uint32_t delay_ms;
};


class ov5640 {
public:
    static const std::vector< std::pair< const uint16_t, boost::json::object > >& regs();
    static const std::vector< std::pair< const uint16_t, const uint8_t > >& cfg_init();
    static const std::vector< std::pair< const uint16_t, const uint8_t > >& cfg_1080p_30fps();

    static const std::vector< reg_value >& setting_1080P_1920_1080();
    static const std::vector< reg_value >& init_setting_30fps_VGA();

    std::optional< std::pair<uint8_t, uint8_t> > chipid( i2c_linux::i2c& ) const;
    bool reset( i2c_linux::i2c& ) const;
    bool init( i2c_linux::i2c& ) const;
};
