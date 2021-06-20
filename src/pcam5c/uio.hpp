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
#include <fstream>
#include <optional>
#include <boost/json.hpp>

class uio {
    std::string path_;
public:
    ~uio();
    uio( const std::string& device );
    void dump() const;
    std::optional< uint32_t > read( uint32_t addr ) const;
    bool read( uint32_t *, size_t counts, uint32_t addr = 0 ) const;
    bool write( uint32_t addr, uint32_t value ) const;

    inline bool operator()( uint32_t addr, uint32_t value ) const { return write( addr, value ); }
    inline std::optional< uint32_t > operator()( uint32_t addr ) const { return read( addr ); }

    static void pprint( const boost::json::object& json, const uint32_t * data, size_t size );
};
