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

   Source File Name = parser.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/01/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#include "parser.hpp"
#include "ossUtil.hpp"
#include "../../bson/bson.hpp"
#include "mongodef.hpp"

void msgParser::extractMsg( const CHAR *in, const INT32 inLen )
{
   INT32 nsLen = 0 ;
   const CHAR *ptr = NULL ;
   const CHAR *dbName = NULL ;

   if ( NULL != _dataStart )
   {
      //_reset() ;
      _offset = 0 ;
      _currentOp = OP_INVALID ;
      _dataPacket.clear() ;
   }

   _dataStart = in ;
   _dataEnd   = in + inLen ;

   readInt( sizeof( INT32 ), (CHAR *)&_dataPacket.msgLen ) ;
   readInt( sizeof( INT32 ), (CHAR *)&_dataPacket.requestId ) ;
   // skip responseTo
   _dataPacket.responseTo = 0 ;
   skipBytes( sizeof( _dataPacket.responseTo ) ) ;

   readInt( sizeof( SINT16 ), (CHAR *)&_dataPacket.opCode ) ;

   // skip flags and version
   skipBytes( sizeof( CHAR ) * 2 ) ;
   _dataPacket.flags = 0 ;
   _dataPacket.version = 0 ;

   readInt( sizeof( INT32 ), (CHAR *)&_dataPacket.reservedInt ) ;
   dbName = _dataStart + _offset ;

   if ( dbKillCursors == _dataPacket.opCode )
   {
      goto done ;
   }

   while ( *(dbName + nsLen ) )
   {
      ++nsLen ;
   }
   _offset += nsLen + 1 ;

   ptr = ossStrstr( dbName, ".$cmd" ) ;
   if ( NULL != ptr )
   {
      _dataPacket.optionMask |= OPTION_CMD ;
      _dataPacket.csName = std::string( dbName ).substr( 0, ptr - dbName ) ;
   }

   if ( NULL != ( ptr = ossStrstr( dbName, ".system.indexes" ) ) )
   {
      _dataPacket.optionMask |= OPTION_IDX ;
      //_dataPacket.fullName = AUTH_USR_COLLECTION ;
      _dataPacket.csName = std::string( dbName ).substr( 0, ptr - dbName ) ;
   }
   else if ( NULL != ( ptr = ossStrstr( dbName, ".system.users" ) ) )
   {
      _dataPacket.optionMask |= OPTION_USR ;
      _dataPacket.csName = std::string( dbName ).substr( 0, ptr - dbName ) ;
      //_dataPacket.fullName = AUTH_USR_COLLECTION ;
   }
   else if ( NULL != ( ptr = ossStrstr( dbName, ".system.namespaces" ) ) )
   {
      _dataPacket.optionMask |= OPTION_CLS ;
      _dataPacket.csName = std::string( dbName ).substr( 0, ptr - dbName ) ;
      //_dataPacket.fullName = CAT_COLLECTION_INFO_COLLECTION ;
   }


   if ( 0 == _dataPacket.optionMask )
   {
      _dataPacket.fullName = dbName ;
      ptr = ossStrstr( dbName, ".") ;
      if ( NULL != ptr )
      {
         _dataPacket.csName = std::string( dbName ).substr( 0, ptr - dbName ) ;
      }
   }

done:
   return ;
}

void msgParser::readNextObj( bson::BSONObj &obj )
{
   obj = bson::BSONObj( _nextObj ) ;
   _nextObj += obj.objsize() ;
   _offset  += obj.objsize() ;
   if ( _nextObj >= _dataEnd )
   {
      _nextObj = NULL ;
   }
}

void msgParser::readInt( const UINT32 toRead, CHAR *out )
{
   INT32 limit = toRead - 1 ;
   const CHAR *start = _dataStart + _offset ;
   if ( _bigEndian )
   {
      for ( UINT32 i = 0; i < toRead; ++i )
      {
         *( out + i ) = *( start + limit - i ) ;
      }
   }
   else
   {
      ossMemcpy( out, start, toRead ) ;
   }

   _offset += toRead ;
}
