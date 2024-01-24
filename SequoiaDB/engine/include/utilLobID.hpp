/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

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

