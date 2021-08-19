/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = utilCompressorLZW.hpp

   Descriptive Name = Implementation of LZW compression.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_COMPRESSOR_LZW__
#define UTIL_COMPRESSOR_LZW__

#include "utilCompressor.hpp"
#include "utilLZWDictionary.hpp"

namespace engine
{
   class _utilCompressorLZW : public utilCompressor
   {
      public:
         _utilCompressorLZW();

         INT32 compressBound( UINT32 srcLen,
                              UINT32 &maxCompressedLen,
                              const utilDictHandle dictionary = NULL ) ;

         INT32 compress( const CHAR *source, UINT32 sourceLen,
                         CHAR *dest, UINT32 &destLen,
                         const utilDictHandle dictionary = NULL,
                         const utilCompressStrategy *strategy = NULL ) ;

         INT32 getUncompressedLen( const CHAR *source, UINT32 sourceLen,
                                   UINT32 &length) ;

         INT32 decompress( const CHAR *source, UINT32 sourceLen,
                           CHAR *dest, UINT32 &destLen,
                           const utilDictHandle dictionary = NULL ) ;

         virtual OSS_INLINE BOOLEAN needDictionay () const
         {
            return TRUE ;
         }

      private:
         INT32 _compressLevelOne( utilLZWContext &context,
                                  const CHAR* source,
                                  UINT32 sourceLen,
                                  UINT32 maxSize ) ;
         INT32 _compressLevelTwo( utilLZWContext &context,
                                  const CHAR* source,
                                  UINT32 sourceLen,
                                  UINT32 maxSize ) ;
         INT32 _compressLevelThree( utilLZWContext &context,
                                    const CHAR* source,
                                    UINT32 sourceLen,
                                    UINT32 maxSize,
                                    BOOLEAN &varLenCode ) ;

         void _decodeFixLenCode( utilLZWContext &context,
                                 CHAR *dest, UINT32 &destLen ) ;
         void _decodeVarLenCode( utilLZWContext &context,
                                 CHAR *dest, UINT32 &destLen ) ;

         OSS_INLINE LZW_CODE _readCode( _utilLZWContext *ctx ) ;
         OSS_INLINE LZW_CODE _readVarLenCode( _utilLZWContext *ctx ) ;
         OSS_INLINE UINT8 _readByte( _utilLZWContext *ctx ) ;
         OSS_INLINE void _writeCode( _utilLZWContext *ctx, LZW_CODE code ) ;
         OSS_INLINE UINT8 _writeVarLenCode( _utilLZWContext *ctx,
                                            LZW_CODE code ) ;
         OSS_INLINE void _writeByte( _utilLZWContext *ctx, UINT8 ch ) ;
         OSS_INLINE void _writeBits( _utilLZWContext *ctx,
                                     UINT32 bits, UINT32 bitNum ) ;

         OSS_INLINE void _writeBitsExt( _utilLZWContext *ctx,
                                        UINT32 bits,
                                        UINT32 bitNum,
                                        UINT32 codeLen ) ;
         OSS_INLINE void _flushBits( _utilLZWContext *ctx ) ;
   };
   typedef _utilCompressorLZW utilCompressorLZW ;

   OSS_INLINE LZW_CODE _utilCompressorLZW::_readCode( _utilLZWContext *ctx )
   {
      UINT32 bits = 0 ;
      UINT32 codeSize = ctx->getDictionary()->getCodeSize() ;

      while ( ctx->_bitBuf._n < codeSize )
      {
         UINT8 ch = _readByte( ctx ) ;
         ctx->_bitBuf._buf = ( ctx->_bitBuf._buf << 8 ) | ch ;
         ctx->_bitBuf._n += 8 ;
      }

      ctx->_bitBuf._n -= codeSize ;
      bits = ( ctx->_bitBuf._buf >> ctx->_bitBuf._n )
             & (( 1 << codeSize ) - 1 ) ;

      return bits ;
   }

   OSS_INLINE LZW_CODE _utilCompressorLZW::_readVarLenCode( _utilLZWContext *ctx )
   {
      UINT8 ch ;
      UINT32 bits = 0 ;
      UINT32 codeSize = 0 ;
      UINT32 lenIdx = 0 ;
      UINT8 varLenFlagSize = ctx->getDictionary()->getVarLenFlagSize() ;

      if ( ctx->_bitBuf._n < varLenFlagSize )
      {
         /* fetch one one more character to get the length of the next code */
         ch = _readByte( ctx ) ;
         ctx->_bitBuf._buf = ( ctx->_bitBuf._buf << 8 ) | ch ;
         ctx->_bitBuf._n += 8 ;
      }

      /* The first 2 bits the buffer is the length of the next code. */
      lenIdx = ( UINT32 )
               ( ( ctx->_bitBuf._buf >> ( ctx->_bitBuf._n - varLenFlagSize ) )
               & ( ( 1 << varLenFlagSize ) - 1 ) ) ;
      codeSize = ctx->getDictionary()->getVarLenSize( lenIdx ) ;
      /*
       * Once get the length of the next code, shift the length flag out of the
       * buffer, and get the remaining bits of the code, if they are not in the
       * buffer yet.
       */
      ctx->_bitBuf._n -= varLenFlagSize ;
      while ( ctx->_bitBuf._n < codeSize )
      {
         ch = _readByte( ctx ) ;
         ctx->_bitBuf._buf = ( ctx->_bitBuf._buf << 8 ) | ch ;
         ctx->_bitBuf._n += 8 ;
      }

      ctx->_bitBuf._n -= codeSize ;
      bits = ( ctx->_bitBuf._buf >> ctx->_bitBuf._n )
             & (( 1 << codeSize ) - 1 ) ;

      return bits ;
   }

   OSS_INLINE UINT8 _utilCompressorLZW::_readByte( _utilLZWContext *ctx )
   {
      return ctx->_stream[ctx->_streamPos++] ;
   }

   OSS_INLINE void _utilCompressorLZW::_writeCode( _utilLZWContext *ctx,
                                                   LZW_CODE code )
   {
      _writeBits( ctx, code, ctx->getDictionary()->getCodeSize() ) ;
   }

   OSS_INLINE UINT8 _utilCompressorLZW::_writeVarLenCode( _utilLZWContext *ctx,
                                                          LZW_CODE code )
   {
      UINT8 lenIndex = 0 ;
      UINT8 splitSize = 0 ;
      ctx->getDictionary()->getVarLenInfo( code, lenIndex, splitSize ) ;
      _writeBitsExt( ctx, code, splitSize, lenIndex ) ;

      return splitSize ;
   }

   OSS_INLINE void _utilCompressorLZW::_writeByte( _utilLZWContext *ctx,
                                                   UINT8 ch )
   {
      ctx->_stream[ctx->_streamPos++] = ch ;
   }

   OSS_INLINE void _utilCompressorLZW::_writeBits( _utilLZWContext *ctx,
                                                   UINT32 bits,
                                                   UINT32 bitNum)
   {
      ctx->_bitBuf._buf = ( ctx->_bitBuf._buf << bitNum )
                          | ( bits & (( 1 << bitNum ) - 1 ) ) ;

      bitNum += ctx->_bitBuf._n ;

      while ( bitNum >= 8 )
      {
         UINT8 ch ;
         bitNum -= 8 ;
         ch = ctx->_bitBuf._buf >> bitNum ;
         _writeByte( ctx, ch ) ;
      }

      ctx->_bitBuf._n = bitNum ;
   }

   OSS_INLINE void _utilCompressorLZW::_writeBitsExt( _utilLZWContext *ctx,
                                                      UINT32 bits,
                                                      UINT32 bitNum,
                                                      UINT32 codeLen )
   {
      UINT8 varLenFlagSize = ctx->getDictionary()->getVarLenFlagSize() ;
      ctx->_bitBuf._buf =
         ( ctx->_bitBuf._buf << ( bitNum + varLenFlagSize ) )
         | ( codeLen << bitNum )
         | ( bits & (( 1 << bitNum ) - 1 ) ) ;

      bitNum = ctx->_bitBuf._n + bitNum + varLenFlagSize ;

      while ( bitNum >= 8 )
      {
         UINT8 ch ;
         bitNum -= 8 ;
         ch = ctx->_bitBuf._buf >> bitNum ;
         _writeByte( ctx, ch ) ;
      }

      ctx->_bitBuf._n = bitNum ;
   }

   OSS_INLINE void _utilCompressorLZW::_flushBits( _utilLZWContext *ctx )
   {
      if ( ctx->_bitBuf._n )
      {
         _writeBits( ctx, 0, 8 - ctx->_bitBuf._n ) ;
      }
   }

}

#endif /* UTIL_COMPRESSOR_LZW__ */

