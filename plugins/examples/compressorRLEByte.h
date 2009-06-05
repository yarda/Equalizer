
/* Copyright (c) 2009, Cedric Stalder <cedric.stalder@gmail.com> 
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 
#ifndef EQ_PLUGIN_RLE1BYTECOMPRESSOR
#define EQ_PLUGIN_RLE1BYTECOMPRESSOR

#include "compressorRLE.h"

namespace eq
{
namespace plugin
{
class CompressorRLEByte : public CompressorRLE
{
public:

    CompressorRLEByte(): CompressorRLE( 1 ){}
    virtual ~CompressorRLEByte(){} 
    virtual void compress( void* const inData, 
                           const uint64_t inSize, 
                           const bool useAlpha );

    virtual void decompress( const void** const inData, 
                             const uint64_t* const inSizes, 
                             void* const outData, 
                             const uint64_t* const outSize ); 
    
    static void* getNewCompressor( )
                                   { return new eq::plugin::CompressorRLEByte; }
    
    static void* getNewDecompressor( ){ return 0; }


    static void  getInfo( EqCompressorInfo* const info )
    {
         info->version = EQ_COMPRESSOR_VERSION;
         info->type = EQ_COMPRESSOR_RLE_BYTE;
         info->capabilities = EQ_COMPRESSOR_DATA_1D;
         info->tokenType = EQ_COMPRESSOR_DATATYPE_BYTE;

         info->capabilities = EQ_COMPRESSOR_DATA_1D;
         info->quality = 1.f;
         info->ratio = .8f;
         info->speed = 0.95f;
    }

    static Functions getFunctions()
    {
        Functions functions;
        functions.getInfo            = getInfo;
        functions.newCompressor      = getNewCompressor;       
        return functions;
    }

private:
    void _compress( const uint8_t* input, const uint64_t size, 
                    Result** results );
};
}
}
#endif // EQ_PLUGIN_RLE1BYTECOMPRESSOR
