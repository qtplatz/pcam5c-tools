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

#include "uio.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>

uio::~uio()
{
}

uio::uio( const std::string& device ) : path_( device )
{
}

void
uio::dump() const
{
    std::ifstream is( path_, std::ios::binary );
    if ( is ) {
        std::array< uint32_t, 16 > regs;
        if ( is.read( reinterpret_cast< char * >(regs.data()), regs.size() * sizeof( uint32_t ) ) ) {
            size_t i(0);
            for ( const auto& reg: regs ) {
                if ( ( i % 4 ) == 0 )
                    std::cout << boost::format( "\n%04x: " ) % (i * sizeof(uint32_t));
                std::cout << boost::format( "\t0x%08x" ) % reg;
                ++i;
            }
            std::cout << std::endl;
        }
    }
}

std::optional< uint32_t >
uio::read( uint32_t addr ) const
{
    std::ifstream is( path_, std::ios::binary );
    if ( is ) {
        is.seekg( addr / sizeof(uint32_t ) );
        uint32_t data;
        if ( is.read( reinterpret_cast< char * >(&data), sizeof( data ) ) ) {
            return data;
        }
    }
    return {};
}

bool
uio::read( uint32_t * data, size_t counts, uint32_t addr ) const
{
    std::ifstream is( path_, std::ios::binary );
    if ( is ) {
        is.seekg( addr / sizeof(uint32_t ) );
        if ( is.read( reinterpret_cast< char * >( data ), sizeof( data ) * counts ) ) {
            return true;
        }
    }
    return false;
}

bool
uio::write( uint32_t addr, uint32_t value ) const
{
    std::ofstream os( path_, std::ios::binary );
    if ( os ) {
        os.seekp( addr / sizeof(uint32_t ) );
        if ( os.write( reinterpret_cast< char * >(&value), sizeof( value ) ) ) {
            return true;
        }
    }
    return false;
}

// static
void
uio::pprint( const boost::json::object& json, const uint32_t * data, size_t size )
{
    auto regs = json.at( "regs" ).as_array();
    for ( const auto& reg: regs ) {
        auto addr = reg.at( "addr" ).as_int64();
        auto name = reg.at( "name" ).as_string();
        auto flds = reg.if_object()->if_contains( "flds" );
        // std::cout << "addr: " << addr << ", name: " << name;
        // if ( flds ) {
        //     std::cout << ", flds: " << *flds;
        // }
        if ( addr / 4 < size ){
            const auto value = data[ addr / 4 ];
            std::cout << boost::format( "0x%04x:\t0x%08x\t%s" ) % addr % value % name;
            if ( flds ) {
                if ( auto a = flds->if_array() ) {
                    for ( const auto& bits: *a ) {
                        if ( auto bfld = bits.if_array() ) {
                            if ( bfld->size() == 3 ) {
                                uint32_t msb = (*bfld)[0].as_int64();
                                uint32_t lsb = (*bfld)[1].as_int64();
                                uint32_t nb = msb - lsb + 1;
                                uint32_t mask = (1 << nb) - 1;
                                std::cout << boost::format( "\t[%d:%d]%s=0x%x;" ) % msb % lsb % (*bfld)[2].as_string() % ((value >> lsb) & mask);
                            } else if ( bfld->size() == 2 ) {
                                uint32_t lsb = (*bfld)[0].as_int64();
                                std::cout << boost::format( "\t[%d]=%d %s" ) % lsb % ((value >> lsb)&1) % (*bfld)[1].as_string();
                            }
                        }
                    }
                }
            }
        }
        std::cout << std::endl;
    }
}
