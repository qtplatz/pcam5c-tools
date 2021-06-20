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

#include "d_phyrx.hpp"
#include "uio.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/json.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace {
    using namespace boost::json;
    boost::json::object core_register = {
        { "regs"
          , array{
                {{"addr", 0x0000}, {"name", "CONTROL"}, { "flds",{{31,2,"resv"},{1,"DPHY_EN"},{0,"SRST"}} }/*flds*/  }
                ,{{"addr", 0x0004}, {"name", "Reserved"}}
                ,{{"addr", 0x0008}, {"name", "INIT_VAL"}}
                ,{{"addr", 0x000c}, {"name", "Reserved"}}
                ,{{"addr", 0x0010}, {"name", "HS_TIMEOUT"}}
                ,{{"addr", 0x0014}, {"name", "ESC_TIMEOUT"}}
                ,{{"addr", 0x0018}, {"name", "CL_STATUS"}}
                ,{{"addr", 0x001c}, {"name", "DL1_STATUS"}}
                ,{{"addr", 0x0020}, {"name", "DL2_STATUS"}}
                ,{{"addr", 0x0024}, {"name", "DL3_STATUS"}}
                ,{{"addr", 0x0028}, {"name", "DL4_STATUS"}}
                ,{{"addr", 0x0030}, {"name", "HS_SETTLE"}}
            }
        }
    };
}

d_phyrx::~d_phyrx()
{
}

d_phyrx::d_phyrx( const std::string& device ) : uio( device )
{
}

void
d_phyrx::dump() const
{
#if 0
    auto json = boost::json::serialize( core_register );
    boost::property_tree::ptree pt;
    std::istringstream is( json );
    boost::property_tree::read_json( is, pt );
    boost::property_tree::write_json( std::cout, pt );
#endif
    std::array< uint32_t, 0x30 / 4 > data;
    if ( read( data.data(), data.size(), 0 ) ) {
        pprint( core_register, data.data(), data.size() );
    }
}
