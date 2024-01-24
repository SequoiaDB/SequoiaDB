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

   Source File Name = utilLobID.cpp

   Descriptive Name = Implementation of lob id.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2019/07/09  LinYoubin Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilLobID.hpp"

#include <sstream>

using namespace std ;

namespace engine
{
   // number is Odd or not in one byte
   INT32 _utilLobID::_isOddArray[256] = {
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0} ;

   _utilSerialAllocator _utilLobID::_serialAllocator ;

   _utilLobID::_utilLobID()
   {
      _seconds = 0 ;
      _oddCheck = 0 ;
      _id = 0 ;
      _serial = 0 ;
   }

   _utilLobID::~_utilLobID()
   {
   }

   INT32 _utilLobID::_parseHexValue( const CHAR *hexValue, UINT8 *serialValue ) const
   {
      INT32 rc = SDB_OK ;
      INT32 index = 0 ;

      if ( ossStrlen( hexValue ) != UTIL_LOBID_HEX_FORMAT_LEN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( index = 0 ; index < UTIL_LOBID_ARRAY_LEN ; ++index )
      {
         INT32 hi ;
         INT32 lo ;
         rc = _fromHex( hexValue[0], hi ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _fromHex( hexValue[1], lo ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         serialValue[index] = ( UINT8 )( hi << 4 | lo ) ;
         hexValue += 2 ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilLobID::initFromByteArray( const BYTE *array, INT32 arrayLen )
   {
      INT32 rc = SDB_OK ;
      INT32 i = 0 ;
      INT32 index = 0 ;
      if ( NULL == array || arrayLen < UTIL_LOBID_ARRAY_LEN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _seconds = 0 ;
      for ( i = 5; i >= 0; --i )
      {
         _seconds = _seconds | ((UINT64)array[index++] << (i * 8)) ;
      }

      _oddCheck = array[index++] ;

      rc = _checkOddBit() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _id = 0 ;
      for ( i = 1; i >= 0; --i )
      {
         _id = _id | ((UINT16)array[index++] << (i * 8)) ;
      }

      _serial = 0 ;
      for ( i = 2; i >= 0; --i )
      {
         _serial = _serial | ((UINT32)array[index++] << (i * 8)) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilLobID::init( const CHAR *hexValue )
   {
      INT32 rc = SDB_OK ;
      UINT8 serialValue[UTIL_LOBID_ARRAY_LEN + 1] = { 0 } ;

      if ( NULL == hexValue )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _parseHexValue( hexValue, serialValue ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = initFromByteArray( serialValue, UTIL_LOBID_ARRAY_LEN ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _utilLobID::init( INT64 seconds, UINT16 id )
   {
      _seconds = seconds ;
      _setOddCheckBit() ;

      _id = id ;
      _serial = _serialAllocator.fetchAndIncrement() ;

      return SDB_OK ;
   }

   // used for matcher bound
   void _utilLobID::initOnlySeconds( INT64 seconds )
   {
      _seconds = seconds ;
   }

   INT32 _utilLobID::parseSeconds( const BYTE *array, INT32 arrayLen,
                                   INT64 &seconds )
   {
      INT32 rc = SDB_OK ;
      INT32 i = 0 ;
      INT32 index = 0 ;
      if ( NULL == array || arrayLen < UTIL_LOBID_ARRAY_LEN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      seconds = 0 ;
      for ( i = 5; i >= 0; --i )
      {
         seconds = seconds | ((UINT64)array[index++] << (i * 8)) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // _seconds(8bytes) 7 6 5 4 3 2 1 0
   //                      | | | | | |
   // _oddCheck(8bits) 7 6 5 4 3 2 1 0
   void _utilLobID::_setOddCheckBit()
   {
      _oddCheck = 0;

      for ( INT32 i = 5; i >= 0; --i )
      {
         INT32 subValue = (UINT8)( _seconds >> (i * 8) ) & 0x0FF ;
         if ( !_isOddArray[subValue] )
         {
            _oddCheck = _oddCheck | 1 << i ;
         }
      }
   }

   BOOLEAN _utilLobID::_bitIsOne( UINT8 value, INT32 pos )
   {
      if ((value & (1 << pos)) != 0)
      {
         return TRUE ;
      }

      return FALSE ;
    }

   // _seconds(8bytes) 7 6 5 4 3 2 1 0
   //                      | | | | | |
   // _oddCheck(8bits) 7 6 5 4 3 2 1 0
   INT32 _utilLobID::_checkOddBit()
   {
      INT32 rc = SDB_OK ;
      for ( INT32 i = 5; i >= 0; --i )
      {
         INT32 s = (INT32) ((_seconds >> (i * 8)) & 0xFF);
         if ( (_isOddArray[s] && !_bitIsOne(_oddCheck, i))
            || (!_isOddArray[s] && _bitIsOne(_oddCheck, i)) )
         {
            continue ;
         }

         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _utilLobID::_toSerialValue( UINT8 *value ) const
   {
      INT32 index = 0 ;

      value[index++] = (UINT8) ( _seconds >> 40 & 0xFF ) ;
      value[index++] = (UINT8) ( _seconds >> 32 & 0xFF ) ;
      value[index++] = (UINT8) ( _seconds >> 24 & 0xFF ) ;
      value[index++] = (UINT8) ( _seconds >> 16 & 0xFF ) ;
      value[index++] = (UINT8) ( _seconds >> 8 & 0xFF ) ;
      value[index++] = (UINT8) ( _seconds & 0xFF ) ;

      value[index++] = _oddCheck ;

      value[index++] = (UINT8) ( _id >> 8 & 0xFF ) ;
      value[index++] = (UINT8) ( _id & 0xFF ) ;

      value[index++] = (UINT8) ( _serial >> 16 & 0xFF ) ;
      value[index++] = (UINT8) ( _serial >> 8 & 0xFF ) ;
      value[index++] = (UINT8) ( _serial & 0xFF ) ;
   }

   INT32 _utilLobID::_fromHex( const CHAR c, INT32 &reslut ) const
   {
      INT32 rc = SDB_OK ;

      if ( '0' <= c && c <= '9' )
      {
         reslut =  c - '0' ;
      }
      else if ( 'a' <= c && c <= 'f' )
      {
         reslut = c - 'a' + 10 ;
      }
      else if ( 'A' <= c && c <= 'F' )
      {
         reslut = c - 'A' + 10 ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }

      return rc ;
   }

   INT64 _utilLobID::getSeconds() const
   {
      return _seconds ;
   }

   INT32 _utilLobID::toByteArray( BYTE *result, INT32 resultLen ) const
   {
      INT32 rc = SDB_OK ;
      if ( NULL == result || resultLen < UTIL_LOBID_ARRAY_LEN )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _toSerialValue( result ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   string _utilLobID::toString() const
   {
      static const char hexchars[] = "0123456789abcdef" ;

      stringstream ss ;
      UINT8 serialValue[UTIL_LOBID_ARRAY_LEN] = {0} ;

      _toSerialValue( serialValue ) ;

      for ( INT32 i = 0; i < UTIL_LOBID_ARRAY_LEN; ++i )
      {
         CHAR c = serialValue[i] ;
         CHAR high = hexchars[(c & 0xF0) >> 4] ;
         CHAR low = hexchars[(c & 0x0F)] ;

         ss << high ;
         ss << low ;
      }

      return ss.str() ;
   }

   _utilSerialAllocator::_utilSerialAllocator() : _atomicSerial(0)
   {
      _atomicSerial.init( ossRand() ) ;
   }

   _utilSerialAllocator::~_utilSerialAllocator()
   {
   }

   UINT32 _utilSerialAllocator::fetchAndIncrement()
   {
      return _atomicSerial.inc() ;
   }
}

