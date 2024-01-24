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

   Source File Name = utilIniParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/19/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_INI_PARSEREX_HPP_
#define UTIL_INI_PARSEREX_HPP_

#include "utilIniParser.h"
#include "../bson/bson.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include <string>
#include <list>

using namespace std ;
using namespace bson;

#define UTIL_INI_EX_TYPE_STRING  0
#define UTIL_INI_EX_TYPE_INT32   1
#define UTIL_INI_EX_TYPE_INT64   2
#define UTIL_INI_EX_TYPE_DOUBLE  3
#define UTIL_INI_EX_TYPE_BOOLEAN 4

//Supporting unicode
#define UTIL_INI_EX_UNICODE         0x00010000

//The same section name and key are not allowed
#define UTIL_INI_EX_STRICTMODE      0x00020000

#define UTIL_INI_EX_FLAGS_DEFAULT (UTIL_INI_FLAGS_DEFAULT|\
UTIL_INI_EX_STRICTMODE)

class utilIniParserEx : public SDBObject
{
public:
   utilIniParserEx() ;
   ~utilIniParserEx() ;

   INT32 parse( OSSFILE &file, UINT32 flags ) ;
   INT32 parse( const CHAR *fileName, UINT32 flags ) ;
   INT32 parse( const CHAR *content, INT32 length, UINT32 flags ) ;
   INT32 parseBson( const BSONObj &obj, UINT32 flags ) ;

   INT32 getType( const CHAR *section, const CHAR *key, INT32 &type ) ;
   INT32 getValue( const CHAR *section, const CHAR *key, string  &value ) ;
   INT32 getValue( const CHAR *section, const CHAR *key, INT32   &value ) ;
   INT32 getValue( const CHAR *section, const CHAR *key, INT64   &value ) ;
   INT32 getValue( const CHAR *section, const CHAR *key, FLOAT64 &value ) ;
   INT32 getBoolValue( const CHAR *section, const CHAR *key, BOOLEAN &value ) ;

   INT32 getSectionComment( const CHAR *section, string &comment ) ;
   INT32 getItemPreComment( const CHAR *section, const CHAR *key,
                            string &comment ) ;
   INT32 getItemPosComment( const CHAR *section, const CHAR *key,
                            string &comment ) ;
   INT32 getLastComment( string &comment ) ;

   INT32 setValue( const CHAR *section, const CHAR *key, const string &value ) ;
   INT32 setValue( const CHAR *section, const CHAR *key, const INT32 value ) ;
   INT32 setValue( const CHAR *section, const CHAR *key, const INT64 value ) ;
   INT32 setValue( const CHAR *section, const CHAR *key, const FLOAT64 value ) ;
   INT32 setBoolValue( const CHAR *section, const CHAR *key,
                       const BOOLEAN value ) ;

   INT32 setSectionComment( const CHAR *section, const string &comment ) ;
   INT32 setItemPreComment( const CHAR *section, const CHAR *key,
                            const string &comment ) ;
   INT32 setItemPosComment( const CHAR *section, const CHAR *key,
                            const string &comment ) ;
   INT32 setLastComment( const string &comment ) ;

   INT32 setCommentItem( const CHAR *section, const CHAR *key,
                         BOOLEAN isComment ) ;

   void commentAllItems() ;

   INT32 printFormat( const CHAR *fileName ) ;

   INT32 toString( string &iniText, UINT32 flags = 0 ) ;
   INT32 toBson( BSONObj &obj ) ;
   INT32 save( const CHAR *fileName, UINT32 flags = 0 ) ;
   INT32 getError( string &errMsg ) ;

   UINT32 flags() ;

private:
   INT32 _checkItem() ;
   void _buildBson( BSONObjBuilder &builder, const string &name,
                    const string &key ) ;

private:
   CHAR           *_buffer ;
   utilIniHandler  _handler ;
} ;

#endif /* UTIL_INI_PARSER_HPP_ */