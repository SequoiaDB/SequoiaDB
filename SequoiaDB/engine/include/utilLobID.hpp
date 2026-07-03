/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilLobID.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2019/07/05  LinYoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_LOBID_HPP__
#define UTIL_LOBID_HPP__

#include "ossUtil.h"
#include <string>

using namespace std ;

#define UTIL_LOBID_ARRAY_LEN        12
#define UTIL_LOBID_HEX_FORMAT_LEN   (2 * UTIL_LOBID_ARRAY_LEN)

namespace engine
{
   /*
      Lob bucket hash algorithm type. The caller ( dms ) maps the lobm file
      version to one of these types, so that util does not depend on dms
      version constants.
   */
   enum UTIL_LOB_HASH_TYPE
   {
      UTIL_LOB_HASH_DJB2 = 0,  /// legacy, for old lobm files
      UTIL_LOB_HASH_MD5        /// md5 based, same as coord clsPartition
   } ;

   /*
      Calculate the lob bucket hash from oid( 12 bytes ) and sequence.
      UTIL_LOB_HASH_DJB2: keep the same result as the old ossHash algorithm.
      UTIL_LOB_HASH_MD5 : md5( oid ++ sequence ), identical to clsPartition.
   */
   UINT32 utilLobHash( const BYTE *oid,
                       UINT32 sequence,
                       UTIL_LOB_HASH_TYPE hashType ) ;

   /*
      CRC32C ( Castagnoli ) checksum, used to verify lob data page validity.
   */
   UINT32 utilCrc32c( const void *data, UINT32 len ) ;

   /*
      _utilLobID define
   */

   class _utilSerialAllocator : public SDBObject
   {
   public:
      _utilSerialAllocator() ;
      ~_utilSerialAllocator() ;

   public:
      UINT32 fetchAndIncrement() ;

   private:
      _ossAtomic32 _atomicSerial ;
   } ;

   class _utilLobID : public SDBObject
   {
   public:
      _utilLobID() ;
      ~_utilLobID() ;

   public:
      INT32 init( const CHAR *hexValue ) ;
      INT32 initFromByteArray( const BYTE *array, INT32 arrayLen ) ;
      INT32 init( INT64 seconds, UINT16 id ) ;

      void initOnlySeconds( INT64 seconds ) ;
      string toString() const ;
      INT32 toByteArray( BYTE *result, INT32 resultLen ) const ;

      INT64 getSeconds() const ;

      static INT32 parseSeconds( const BYTE *array, INT32 arrayLen,
                                 INT64 &seconds ) ;

   private:
      void _setOddCheckBit() ;
      INT32 _checkOddBit() ;
      void _toSerialValue( UINT8 *serialValue ) const ;
      INT32 _parseHexValue( const CHAR *hexValue, UINT8 *serialValue ) const ;
      INT32 _fromHex( const CHAR c, INT32 &reslut ) const ;
      BOOLEAN _bitIsOne( UINT8 value, INT32 pos ) ;

   private:
      INT64 _seconds ;   // just use lower 6 bytes
      UINT8 _oddCheck ;  // just use lower 6 bits
      UINT16 _id ;
      UINT32 _serial ;   // serial number

   private:
      // number is Odd or not in one byte
      static INT32 _isOddArray[256] ;
      static _utilSerialAllocator _serialAllocator ;
   } ;
}

#endif // UTIL_LOBID_HPP__

