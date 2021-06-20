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

#include "csi2rx.hpp"
#include "uio.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/json.hpp>

namespace {
    using namespace boost::json;
    boost::json::object core_register = {
        { "regs"
          , array{
                // {{"addr", 0x0000}, {"name", "CONTROL"}, { "flds",{{31,2,"resv"},{1,"DPHY_EN"},{0,"SRST"}} }/*flds*/  }
                {{"addr", 0x0000},  {"name",  "Core Configuration Register" }       }
                ,{{"addr", 0x0004}, {"name", "Protocol Configuration Register" }    }
                ,{{"addr", 0x0008}, {"name", "Reserved" }                           }
                ,{{"addr", 0x000c}, {"name", "Reserved" }                           }
                ,{{"addr", 0x0010}, {"name", "Core Status Register" }               }
                ,{{"addr", 0x0014}, {"name", "Reserved" }                           }
                ,{{"addr", 0x0018}, {"name", "Reserved" }                           }
                ,{{"addr", 0x001c}, {"name", "Reserved" }                           }
                ,{{"addr", 0x0020}, {"name", "Global Interrupt Enable Register" }   }
                ,{{"addr", 0x0024}, {"name", "Interrupt Status Register" }          }
                ,{{"addr", 0x0028}, {"name", "Interrupt Enable Register" }          }
                ,{{"addr", 0x002c}, {"name", "Reserved" }                           }
                ,{{"addr",  0x30}, {"name", "Generic Short Packet Register" }       }
                ,{{"addr",  0x34}, {"name", "VCX Frame Error Register" }            } // Reserved in 2.1,
                ,{{"addr",  0x38}, {"name", "Reserved" }                            }
                ,{{"addr",  0x3c}, {"name", "Clock Lane Information Register" }     }
                ,{{"addr",  0x40}, {"name", "Lane0 Information" }                   }
                ,{{"addr",  0x44}, {"name", "Lane1 Information" }                   }
                ,{{"addr",  0x48}, {"name", "Lane2 Information" }                   }
                ,{{"addr",  0x4c}, {"name", "Lane3 Information" }                   }
                ,{{"addr",  0x50}, {"name", "Reserved" }                            }
                ,{{"addr",  0x54}, {"name", "Reserved" }                            }
                ,{{"addr",  0x58}, {"name", "Reserved" }                            }
                ,{{"addr",  0x5c}, {"name", "Reserved" }                            }
                ,{{"addr",  0x60}, {"name", "VC0 image info 1" }                    }
                ,{{"addr",  0x64}, {"name", "VC0 image info 2" }                    }
                ,{{"addr",  0x68}, {"name", "VC1 image info 1" }                    }
                ,{{"addr",  0x6c}, {"name", "VC1 image info 2" }                    }
                ,{{"addr",  0x70}, {"name", "VC2 image info 1" }                    }
                ,{{"addr",  0x74}, {"name", "VC2 image info 2" }                    }
                ,{{"addr",  0x78}, {"name", "VC3 image info 1" }                    }
                ,{{"addr",  0x7c}, {"name", "VC3 image info 2" }                    }
            }
        }
    };

#if 0
    struct core_register { uint32_t addr; const char * name; };
    const core_register __core_register [] = {
        { 0x00,   "Core Configuration Register" }
        , { 0x04, "Protocol Configuration Register" }
        , { 0x08, "Reserved" }
        , { 0x0c, "Reserved" }
        , { 0x10, "Core Status Register" }
        , { 0x14, "Reserved" }
        , { 0x18, "Reserved" }
        , { 0x1c, "Reserved" }
        , { 0x20, "Global Interrupt Enable Register" }
        , { 0x24, "Interrupt Status Register" }
        , { 0x28, "Interrupt Enable Register" }
        , { 0x2c, "Reserved" }
        , { 0x30, "Generic Short Packet Register" }
        , { 0x34, "VCX Frame Error Register" } // Reserved in 2.1,
        , { 0x38, "Reserved" }
        , { 0x3c, "Clock Lane Information Register" }
        , { 0x40, "Lane0 Information" }
        , { 0x44, "Lane1 Information" }
        , { 0x48, "Lane2 Information" }
        , { 0x4c, "Lane3 Information" }
        , { 0x50, "Reserved" }
        , { 0x54, "Reserved" }
        , { 0x58, "Reserved" }
        , { 0x5c, "Reserved" }
        , { 0x60, "VC0 image info 1" }
        , { 0x64, "VC0 image info 2" }
        , { 0x68, "VC1 image info 1" }
        , { 0x6c, "VC1 image info 2" }
        , { 0x70, "VC2 image info 1" }
        , { 0x74, "VC2 image info 2" }
        , { 0x78, "VC3 image info 1" }
        , { 0x7c, "VC3 image info 2" }
    };
#endif
}

csi2rx::~csi2rx()
{
}

csi2rx::csi2rx( const std::string& device ) : uio( device )
{
}


void
csi2rx::dump() const
{
    std::array< uint32_t, 0x7c / 4 > data;
    if ( read( data.data(), data.size(), 0 ) ) {
        pprint( core_register, data.data(), data.size() );
        // for ( const auto& reg: __core_register ) {
        //     if ( (reg.addr / 4) < data.size() ) {
        //         std::cout << boost::format( "[%02x]: 0x%08x\t%s\n" )
        //             % reg.addr % data[ reg.addr / 4 ] % reg.name;
        //     }
        // }
    } else {
        uio::dump();
    }
}
