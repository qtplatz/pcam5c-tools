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

#include "i2c.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>

using namespace i2c_linux;

i2c::i2c() : fd_( -1 )
           , address_( 0 )
{
}

i2c::~i2c()
{
    if ( fd_ >= 0 )
        ::close( fd_ );
}

bool
i2c::open( const char * device, int address )
{
    if ( fd_ > 0 )
        ::close( fd_ );

    if ( (fd_ = ::open( device, O_RDWR ) ) >= 0) {

        address_ = address;
        device_ = device;

        if ( ::ioctl( fd_, I2C_SLAVE, address ) < 0 ) {
            ::close( fd_ );
            fd_ = (-1);
            return false;
        }

    } else {
        ::perror( "i2c::open" );
        return false;
    }
    return true;
}

bool
i2c::write( const uint8_t * data, size_t size ) const
{
    if ( fd_ >= 0 ) {
        auto rcode = ::write( fd_, data, size );
        if ( rcode < 0 ) {
            ::perror("i2c::write");
        }
        return rcode == size;
    }
    return false;
}

bool
i2c::read( uint8_t * data, size_t size ) const
{
    if ( fd_ >= 0 ) {
        auto rcode = ::read( fd_, data, size );
        if ( rcode < 0 ) {
            ::perror("i2c::read");
        }
        return rcode == size;
    }
    return false;
}
