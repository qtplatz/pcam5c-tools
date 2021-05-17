/*
 * MIT License
 *
 * Copyright (c) 2018 Toshinobu Hondo
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

#include <array>
#include <cstdint>
#include <optional>

namespace i2c_linux {

    class i2c {
        i2c( const i2c& ) = delete;
        i2c& operator = ( const i2c& ) = delete;
        int fd_;
        int address_;
        std::string device_;

    public:

        i2c();
        ~i2c();

        bool open( const char * device = "/dev/i2c-0", int address = 0x3c ); // i2c_addr >> 1

        inline operator bool () const { return fd_ != (-1); }

        inline int address() const { return address_; }
        inline const std::string& device() const { return device_; }

        bool write( const uint8_t * data, size_t ) const;
        bool read( uint8_t * data, size_t ) const;

        inline bool read_reg16( const uint16_t& reg, uint8_t * data, size_t size ) const {
            std::array< uint8_t, sizeof(reg) > ereg = { uint8_t(reg >> 8u), uint8_t(reg & 0xff) };
            return write( ereg.data(), ereg.size() ) && read( data, size );
        }

        std::optional< uint8_t > read_reg( const uint16_t& reg ) {
            uint8_t value(0);
            std::array< uint8_t, sizeof(reg) > ereg = { uint8_t(reg >> 8u), uint8_t(reg & 0xff) };
            if ( write( ereg.data(), ereg.size() ) && read( &value, 1 ) )
                return value;
            return {};
        }

        inline bool write_reg16( const uint16_t& reg, uint8_t data ) const {
            std::array< uint8_t, sizeof(reg) + 1 > a= { uint8_t(reg >> 8u), uint8_t(reg & 0xff), data };
            return write( a.data(), a.size() );
        }
    };

}
