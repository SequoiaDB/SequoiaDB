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

   Source File Name = parser.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_FAP_MONGO_PARSER_HPP_
#define _SDB_FAP_MONGO_PARSER_HPP_

#include "msg.hpp"
#include "mongodef.hpp"
#include <vector>

inline bson::BSONObj getQueryObj( const bson::BSONObj &obj )
{
   bson::BSONObj query ;
   if ( obj.hasField( "$query") )
   {
      query = obj.getObjectField( "$query" ) ;
   }
   else if ( obj.hasField( "query" ) )
   {
      query = obj.getObjectField( "query" ) ;
   }
   else
   {
      query =  obj;
   }

   return query ;
}

inline bson::BSONObj getSortObj( const bson::BSONObj &obj )
{
   bson::BSONObj sort ;
   if ( obj.hasField( "$orderby" ) )
   {
      sort = obj.getObjectField( "$orderby" ) ;
   }
   else if ( obj.hasField( "orderby" ) )
   {
      sort = obj.getObjectField( "orderby" ) ;
   }

   return sort ;
}

inline bson::BSONObj getHintObj( const bson::BSONObj &obj )
{
   bson::BSONObj hint ;
   if ( obj.hasField( "$hint" ) )
   {
      hint = obj.getObjectField( "$hint" ) ;
   }
   else if ( obj.hasField( "hint" ) )
   {
      hint = obj.getObjectField( "hint" ) ;
   }

   return hint ;
}

inline INT32 getFieldInt( const bson::BSONObj &obj, const CHAR *field )
{
   INT32 limit = 0 ;

   if ( NULL == field )
   {
      goto done ;
   }

   if ( obj.hasField( field ) )
   {
      limit = obj.getIntField( field ) ;
   }

done:
   return limit ;
}

inline void setQueryFlags( const INT32 reserved, SINT32 &flags)
{
   flags |= FLG_QUERY_WITH_RETURNDATA ;

   if ( reserved & QUERY_CURSOR_TAILABLE )
   {
      // unsupported yet
   }

   if ( reserved & QUERY_SLAVE_OK )
   {
      flags |= FLG_QUERY_SLAVEOK ;
   }
   if ( reserved & QUERY_OPLOG_REPLAY )
   {
      flags |= FLG_QUERY_OPLOGREPLAY ;
   }
   if ( reserved & QUERY_NO_CURSOR_TIMEOUT )
   {
      flags |= FLG_QUERY_NOCONTEXTTIMEOUT ;
   }
   if ( reserved & QUERY_AWAIT_DATA )
   {
      flags |= FLG_QUERY_AWAITDATA ;
   }
   if ( reserved & QUERY_PARTIAL_RESULTS )
   {
      flags |= FLG_QUERY_PARTIALREAD ;
   }

   if ( reserved & QUERY_EXHAUST )
   {
      // unsupported yet
   }
}

enum OPTION_MASK
{
   OPTION_CMD = 1 << 0, // using $cmd
   OPTION_IDX = 1 << 1, // using system.indexes
   OPTION_USR = 1 << 2, // using system.users
   OPTION_CLS = 1 << 3, // using system.namespaces
};

class mongoDataPacket : public SDBObject
{
public:
   INT32  optionMask ;
   INT32  msgLen ;
   INT32  requestId ;
   INT32  responseTo ;
   INT32  opCode ;
   CHAR   flags ;
   INT32  reservedInt ;
   INT32  nToSkip ;
   INT32  nToReturn ;
   UINT64 cursorId ;
   std::string csName ;
   std::string clName ;
   std::string fullName ;
   bson::BSONObj all ;
   bson::BSONObj fieldToReturn ;
   std::vector<section> sections ;
   // Only the first mongodb's certification will sent username to fap,
   // But the first and second sequoiadb's certification, we all need to send
   // username to catalog. So we should stored username in mongoDataPacket.
   string userName ;

   mongoDataPacket() : optionMask(0), msgLen(0), requestId(0), responseTo(0),
                       opCode(0), flags(0), reservedInt(0),
                       nToSkip( 0 ), nToReturn( 0 ), cursorId( 0 ),
                       csName( "" ), clName( "" ), fullName( "" )
   {
   }

   BOOLEAN with( OPTION_MASK mask )
   {
      return ( optionMask & mask ) ;
   }

   void clear()
   {
      optionMask = 0 ;
      msgLen = 0 ;
      requestId = 0 ;
      responseTo = 0 ;
      opCode = 0 ;
      flags = 0 ;
      reservedInt = 0 ;
      nToSkip = 0 ;
      nToReturn = 0 ;
      cursorId = 0 ;
      csName.clear() ;
      clName.clear() ;
      fullName.clear() ;
      all = empty ;
      fieldToReturn = empty ;
      sections.clear() ;
      // Don't need to reset userName
   }

private:
   bson::BSONObj empty ;
} ;

class baseCommand ;

class msgParser : public SDBObject
{
public:
   msgParser() : _bigEndian(FALSE), _offset(0), _currentOp(0), _cmd(NULL),
                 _dataStart(NULL), _dataEnd(NULL), _nextObj(NULL)
   {}

   ~msgParser()
   {
      _dataStart = NULL ;
      _dataEnd   = NULL ;
      _nextObj   = NULL ;
   }

   void extractMsg( const CHAR *in, const INT32 inLen ) ;
   void readNextObj( bson::BSONObj &obj ) ;
   void readInt( const UINT32 toRead, CHAR *out ) ;
   void readChar( CHAR *out ) ;
   void readString( std::string &out ) ;

   void skipBytes( const INT32 size )
   {
      if ( _offset + size < 0 )
      {
         return ;
      }

      _offset += size ;
   }

   BOOLEAN more()
   {
      _nextObj = _dataStart + _offset ;
      return ( _nextObj < _dataEnd ) ;
   }

   UINT32 bytesLeft()
   {
      return ( _dataEnd - ( _dataStart + _offset ) ) ;
   }

   void reparse()
   {
      _offset = sizeof( mongoMsgHeader ) + sizeof( INT32 ) ;
   }

   mongoDataPacket& dataPacket()
   {
      return _dataPacket ;
   }

   void setCurrentOp( UINT32 op )
   {
      _currentOp = op ;
   }

   UINT32 currentOperation() const
   {
      return _currentOp ;
   }

   void setEndian( BOOLEAN bigEndian )
   {
      _bigEndian = bigEndian ;
   }

   baseCommand*& command()
   {
      return _cmd ;
   }

private:
   BOOLEAN _bigEndian ;
   UINT32 _offset ;
   UINT32 _currentOp ;
   baseCommand *_cmd ;
   const CHAR  *_dataStart ;
   const CHAR  *_dataEnd ;
   const CHAR  *_nextObj ;
   mongoDataPacket _dataPacket ;
} ;

#endif
