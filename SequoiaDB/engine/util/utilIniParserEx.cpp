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

   Source File Name = utilIniParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/19/2019  HJW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilIniParserEx.hpp"
#include "utilTypeCast.h"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include <sstream>

#define UTIL_INI_ERROR_DUP_SECTION   "Duplicate section name"
#define UTIL_INI_ERROR_DUP_ITEM      "Duplicate item key"

static INT32 _appendList( list<string> &strList, string &fullName,
                          UINT32 flags ) ;

static INT32 _getNumberValue( utilIniHandler &handler, const CHAR *section,
                              const CHAR *key, INT32 &type,
                              utilNumberVal &value ) ;

static void _sectionToString( stringstream &ss, utilIniSection *section,
                              UINT32 flags ) ;
static void _itemToString( stringstream &ss, utilIniItem *item, UINT32 flags ) ;
static void _toString( stringstream &ss, utilIniString *strObj ) ;
static void _toComment( stringstream &ss, utilIniString *strObj ) ;

static BOOLEAN parseHex( const CHAR *pStr, UINT32 *code ) ;
static INT32 utf16ToUtf8( const CHAR *pStr, INT32 length, CHAR **pOut ) ;

utilIniParserEx::utilIniParserEx() : _buffer( NULL )
{
   utilIniInit( &_handler, 0 ) ;
}

utilIniParserEx::~utilIniParserEx()
{
   utilIniRelease( &_handler ) ;
   SAFE_OSS_FREE ( _buffer ) ;
}

INT32 utilIniParserEx::parse( const CHAR *fileName, UINT32 flags )
{
   INT32 rc = SDB_OK ;
   OSSFILE file ;

   rc = ossOpen( fileName, OSS_READONLY | OSS_SHAREREAD, 0, file ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = parse( file, flags ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   if ( file.isOpened() )
   {
      ossClose( file ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::parse( OSSFILE &file, UINT32 flags )
{
   INT32 rc = SDB_OK ;
   INT64 fileSize  = 0 ;
   SINT64 readSize = 0 ;

   utilIniRelease( &_handler ) ;

   utilIniInit( &_handler, flags ) ;

   if ( FALSE == file.isOpened() )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = ossGetFileSize( &file, &fileSize ) ;
   if ( rc )
   {
      goto error ;
   }

   _buffer = (CHAR *)SDB_OSS_MALLOC( fileSize + 1 ) ;
   if ( NULL == _buffer )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   ossMemset( _buffer, 0, fileSize + 1 ) ;

   rc = ossReadN( &file, fileSize, _buffer, readSize ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = utilIniParse( &_handler, _buffer ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( UTIL_INI_EX_STRICTMODE & flags )
   {
      rc = _checkItem() ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::parse( const CHAR *content, INT32 length, UINT32 flags )
{
   INT32 rc = SDB_OK ;

   utilIniRelease( &_handler ) ;

   utilIniInit( &_handler, flags ) ;

   _buffer = (CHAR *)SDB_OSS_MALLOC( length + 1 ) ;
   if ( NULL == _buffer )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   ossMemcpy( _buffer, content, length ) ;
   _buffer[length] = 0 ;

   rc = utilIniParse( &_handler, _buffer ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( UTIL_INI_EX_STRICTMODE & flags )
   {
      rc = _checkItem() ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::parseBson( const BSONObj &obj, UINT32 flags )
{
   INT32 rc = SDB_OK ;

   utilIniRelease( &_handler ) ;

   utilIniInit( &_handler, flags ) ;

   if ( obj.isEmpty() )
   {
      goto done ;
   }

   {
      BSONObjIterator iter( obj ) ;

      while ( iter.more() )
      {
         BSONElement ele = iter.next() ;
         BSONType type   = ele.type() ;
         const CHAR *name  = NULL ;
         const CHAR *point = NULL ;
         string section ;
         string key ;

         name = ele.fieldName() ;
         point = ossStrchr( name, '.' ) ;

         if ( point )
         {
            INT32 sectionLength = point - name ;
            INT32 length = ossStrlen( name ) ;

            section = string( name, 0, sectionLength ) ;
            key = string( name, sectionLength + 1, length - sectionLength - 1 );
         }
         else
         {
            section = "" ;
            key = name ;
         }

         if ( 0 == key.length() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if( String == type )
         {
            rc = setValue( section.c_str(), key.c_str(), ele.String() ) ;
         }
         else if ( NumberInt == type )
         {
            rc = setValue( section.c_str(), key.c_str(), ele.numberInt() ) ;
         }
         else if ( NumberLong == type )
         {
            rc = setValue( section.c_str(), key.c_str(), ele.numberLong() ) ;
         }
         else if ( NumberDouble == type )
         {
            rc = setValue( section.c_str(), key.c_str(), ele.numberDouble() ) ;
         }
         else if ( Bool == type )
         {
            rc = setBoolValue( section.c_str(), key.c_str(),
                               ele.Bool() ? TRUE : FALSE ) ;
         }
         else
         {
            continue ;
         }

         if ( rc )
         {
            goto error ;
         }
      }
   }

   if ( UTIL_INI_EX_STRICTMODE & flags )
   {
      rc = _checkItem() ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getType( const CHAR *section, const CHAR *key, INT32 &type )
{
   INT32 rc = SDB_OK ;
   INT32 numType   = 0 ;
   INT32 length    = 0 ;
   INT32 tmpValLen = 0 ;
   const CHAR *tmp = NULL ;
   utilNumberVal numValue ;

   rc = utilIniGetValue( &_handler, section, key, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   type = UTIL_INI_EX_TYPE_STRING ;

   if ( 0 < length )
   {
      utilStrToNumber( tmp, length, &numType, &numValue, &tmpValLen ) ;
      if ( length != tmpValLen )
      {
         numType = -1 ;
      }
   }
   else
   {
      type = UTIL_INI_EX_TYPE_STRING ;
      goto done ;
   }

   if( numType == 0 )
   {
      type = UTIL_INI_EX_TYPE_INT32 ;
   }
   else if( numType == 1 )
   {
      type = UTIL_INI_EX_TYPE_INT64 ;
   }
   else if( numType == 2 )
   {
      type = UTIL_INI_EX_TYPE_DOUBLE ;
   }
   else
   {
      BOOLEAN tmpBool = FALSE ;
      string str( tmp, length ) ;

      rc = ossStrToBoolean( str.c_str(),  &tmpBool ) ;
      if ( SDB_OK == rc )
      {
         type = UTIL_INI_EX_TYPE_BOOLEAN ;
      }

      rc = SDB_OK ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getValue( const CHAR *section, const CHAR *key,
                                 string &value )
{
   INT32 rc = SDB_OK ;
   INT32 length = 0 ;
   const CHAR *tmp = NULL ;

   value = "" ;

   rc = utilIniGetValue( &_handler, section, key, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( length > 0 && tmp )
   {
      if ( UTIL_INI_ESCAPE & _handler.flags )
      {
         INT32 i = 0 ;
         BOOLEAN isEscape = FALSE ;
         CHAR *tmpBuf = NULL ;
         stringstream ss ;

         for ( i = 0 ; i < length; ++i )
         {
            if ( isEscape )
            {
               isEscape = FALSE ;

               switch( tmp[i] )
               {
               case '0':
               {
                  break ;
               }
               case 'a':
               {
                  ss << '\a' ;
                  break ;
               }
               case 'b':
               {
                  ss << '\b' ;
                  break ;
               }
               case 't':
               {
                  ss << '\t' ;
                  break ;
               }
               case 'r':
               {
                  ss << '\r' ;
                  break ;
               }
               case 'n':
               {
                  ss << '\n' ;
                  break ;
               }
               case 'u':
               {
                  if ( UTIL_INI_EX_UNICODE & _handler.flags )
                  {
                     INT32 sequenceLength = 0 ;
                     CHAR *tmpBuffer = NULL ;

                     if ( NULL == tmpBuf )
                     {
                        tmpBuf = (CHAR *)SDB_OSS_MALLOC( length + 1 ) ;
                        if ( NULL == tmpBuf )
                        {
                           rc = SDB_OOM ;
                           goto error ;
                        }
                     }

                     tmpBuffer = tmpBuf ;

                     ossMemset( tmpBuf, 0, length + 1 ) ;

                     sequenceLength = utf16ToUtf8( tmp + i - 1, length - i + 1,
                                                   &tmpBuffer ) ;
                     if( 0 == sequenceLength )
                     {
                        sequenceLength = 2 ;

                        tmpBuf[0] = '\\' ;
                        tmpBuf[1] = tmp[i] ;
                        tmpBuf[2] = 0 ;
                     }

                     i += sequenceLength - 2 ;

                     ss << tmpBuf ;
                  }
                  else
                  {
                     ss << '\\' ;
                     ss << tmp[i] ;
                  }
                  break ;
               }
               default:
               {
                  ss << tmp[i] ;
                  break ;
               }
               }
            }
            else if ( '\\' == tmp[i] )
            {
               isEscape = TRUE ;
            }
            else
            {
               ss << tmp[i] ;
            }
         }

         value = ss.str() ;
         SAFE_OSS_FREE( tmpBuf ) ;
      }
      else
      {
         string str( tmp, length ) ;

         value = str ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getValue( const CHAR *section, const CHAR *key,
                                 INT32 &value )
{
   INT32 rc = SDB_OK ;
   INT32 type = 0 ;
   utilNumberVal tmpValue ;

   value = 0 ;

   rc = _getNumberValue( _handler, section, key, type, tmpValue ) ;
   if ( rc )
   {
      goto error ;
   }

   if( type == 0 )
   {
      value = tmpValue.intVal ;
   }
   else if( type == 1 )
   {
      value = (INT32)tmpValue.longVal ;
   }
   else if( type == 2 )
   {
      value = (INT32)tmpValue.doubleVal ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getValue( const CHAR *section, const CHAR *key,
                                 INT64 &value )
{
   INT32 rc = SDB_OK ;
   INT32 type = 0 ;
   utilNumberVal tmpValue ;

   value = 0 ;

   rc = _getNumberValue( _handler, section, key, type, tmpValue ) ;
   if ( rc )
   {
      goto error ;
   }

   if( type == 0 )
   {
      value = (INT64)tmpValue.intVal ;
   }
   else if( type == 1 )
   {
      value = tmpValue.longVal ;
   }
   else if( type == 2 )
   {
      value = (INT64)tmpValue.doubleVal ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getValue( const CHAR *section, const CHAR *key,
                                 FLOAT64 &value )
{
   INT32 rc = SDB_OK ;
   INT32 type = 0 ;
   utilNumberVal tmpValue ;

   value = 0.0 ;

   rc = _getNumberValue( _handler, section, key, type, tmpValue ) ;
   if ( rc )
   {
      goto error ;
   }

   if( type == 0 )
   {
      value = (FLOAT64)tmpValue.intVal ;
   }
   else if( type == 1 )
   {
      value = (FLOAT64)tmpValue.longVal ;
   }
   else if( type == 2 )
   {
      value = tmpValue.doubleVal ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getBoolValue( const CHAR *section, const CHAR *key,
                                     BOOLEAN &value )
{
   INT32 rc = SDB_OK ;
   INT32 length = 0 ;
   const CHAR *tmp = NULL ;

   value = FALSE ;

   rc = utilIniGetValue( &_handler, section, key, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( length > 0 && tmp )
   {
      string str( tmp, length ) ;

      rc = ossStrToBoolean( str.c_str(),  &value ) ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getSectionComment( const CHAR *section, string &comment )
{
   INT32 rc = SDB_OK ;
   INT32 length = 0 ;
   const CHAR *tmp = NULL ;

   comment = "" ;

   rc = utilIniGetSectionComment( &_handler, section, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( length > 0 && tmp )
   {
      string str( tmp, length ) ;

      comment = str ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getItemPreComment( const CHAR *section, const CHAR *key,
                                          string &comment )
{
   INT32 rc = SDB_OK ;
   INT32 length = 0 ;
   const CHAR *tmp = NULL ;

   comment = "" ;

   rc = utilIniGetItemPreComment( &_handler, section, key, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( length > 0 && tmp )
   {
      string str( tmp, length ) ;

      comment = str ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getItemPosComment( const CHAR *section, const CHAR *key,
                                          string &comment )
{
   INT32 rc = SDB_OK ;
   INT32 length = 0 ;
   const CHAR *tmp = NULL ;

   comment = "" ;

   rc = utilIniGetItemPosComment( &_handler, section, key, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( length > 0 && tmp )
   {
      string str( tmp, length ) ;

      comment = str ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getLastComment( string &comment )
{
   INT32 rc = SDB_OK ;
   INT32 length = 0 ;
   const CHAR *tmp = NULL ;

   comment = "" ;

   rc = utilIniGetLastComment( &_handler, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   if ( length > 0 && tmp )
   {
      string str( tmp, length ) ;

      comment = str ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::setValue( const CHAR *section, const CHAR *key,
                                 const string &value )
{
   INT32 length  = value.length() ;
   const CHAR *p = value.c_str() ;

   if ( UTIL_INI_SINGLE_QUOMARK & _handler.flags ||
        UTIL_INI_DOUBLE_QUOMARK & _handler.flags )
   {
      INT32 i = 0 ;
      string newValue = "" ;

      if ( UTIL_INI_ESCAPE & _handler.flags )
      {
         for ( i = 0; i < length; ++i )
         {
            if ( '\\' == p[i] )
            {
               newValue += "\\\\" ;
            }
            else if ( '\0' == p[i] )
            {
               newValue += "\\0" ;
            }
            else if ( '\a' == p[i] )
            {
               newValue += "\\a" ;
            }
            else if ( '\b' == p[i] )
            {
               newValue += "\\b" ;
            }
            else if ( '\t' == p[i] )
            {
               newValue += "\\t" ;
            }
            else if ( '\r' == p[i] )
            {
               newValue += "\\r" ;
            }
            else if ( '\n' == p[i] )
            {
               newValue += "\\n" ;
            }
            else if ( UTIL_INI_SEMICOLON & _handler.flags && ';' == p[i] )
            {
               newValue += "\\;" ;
            }
            else if ( UTIL_INI_HASHMARK & _handler.flags && '#' == p[i] )
            {
               newValue += "\\#" ;
            }
            else if ( UTIL_INI_EQUALSIGN & _handler.flags && '=' == p[i] )
            {
               newValue += "\\=" ;
            }
            else if ( UTIL_INI_COLON & _handler.flags && ':' == p[i] )
            {
               newValue += "\\:" ;
            }
            else if ( UTIL_INI_DOUBLE_QUOMARK & _handler.flags && '"' == p[i] )
            {
               newValue += "\\\"" ;
            }
            else if ( UTIL_INI_SINGLE_QUOMARK & _handler.flags && '\'' == p[i] )
            {
               newValue += "\\'" ;
            }
            else
            {
               newValue += p[i] ;
            }
         }
      }
      else
      {
         newValue = value ;
      }

      return utilIniSetValue( &_handler, section, key,
                              newValue.c_str(), newValue.length() ) ;
   }
   else
   {
      return utilIniSetValue( &_handler, section, key, p, length ) ;
   }
}

INT32 utilIniParserEx::setValue( const CHAR *section, const CHAR *key,
                                 const INT32 value )
{
   INT32 length = 0 ;
   CHAR buff[32] ;

   length = ossSnprintf( buff, 32, "%d", value ) ;
   return utilIniSetValue( &_handler, section, key, buff, length ) ;
}

INT32 utilIniParserEx::setValue( const CHAR *section, const CHAR *key,
                                 const INT64 value )
{
   INT32 length = 0 ;
   CHAR buff[32] ;

   length = ossSnprintf( buff, 32, "%lld", value ) ;
   return utilIniSetValue( &_handler, section, key, buff, length ) ;
}

INT32 utilIniParserEx::setValue( const CHAR *section, const CHAR *key,
                                 const FLOAT64 value )
{
   INT32 length = 0 ;
   CHAR buff[512] ;

   length = ossSnprintf( buff, 512, "%g", value ) ;
   return utilIniSetValue( &_handler, section, key, buff, length ) ;
}

INT32 utilIniParserEx::setBoolValue( const CHAR *section, const CHAR *key,
                                     const BOOLEAN value )
{
   return utilIniSetValue( &_handler, section, key, value ? "true" : "false",
                           value ? 4 : 5 ) ;
}

INT32 utilIniParserEx::setSectionComment( const CHAR *section,
                                          const string &comment )
{
   return utilIniSetSectionComment( &_handler, section, comment.c_str(),
                                    comment.length() ) ;
}

INT32 utilIniParserEx::setItemPreComment( const CHAR *section, const CHAR *key,
                                          const string &comment )
{
   return utilIniSetItemPreComment( &_handler, section, key, comment.c_str(),
                                    comment.length() ) ;
}

INT32 utilIniParserEx::setItemPosComment( const CHAR *section, const CHAR *key,
                         const string &comment )
{
   return utilIniSetItemPosComment( &_handler, section, key, comment.c_str(),
                                    comment.length() ) ;
}

INT32 utilIniParserEx::setLastComment( const string &comment )
{
   return utilIniSetLastComment( &_handler, comment.c_str(),
                                 comment.length() ) ;
}

INT32 utilIniParserEx::setCommentItem( const CHAR *section, const CHAR *key,
                                       BOOLEAN isComment )
{
   return utilIniSetCommentItem( &_handler, section, key, isComment ) ;
}

void utilIniParserEx::commentAllItems()
{
   utilIniSection *section = NULL ;
   utilIniItem *item = NULL ;

   item = _handler.item ;

   while( item )
   {
      item->isComment = TRUE ;

      item = item->next ;
   }

   section = _handler.section ;

   while( section )
   {
      item = section->item ;

      while( item )
      {
         item->isComment = TRUE ;

         item = item->next ;
      }

      section = section->next ;
   }
}

INT32 utilIniParserEx::_checkItem()
{
   INT32 rc = SDB_OK ;
   list<string> sectionList ;
   list<string> itemList ;

   {
      utilIniItem *item = _handler.item ;

      while( item )
      {
         if ( FALSE == item->isComment )
         {
            string key( item->key.str, item->key.length ) ;

            rc = _appendList( itemList, key, _handler.flags ) ;
            if ( rc )
            {
               _handler.errnum = rc ;
               _handler.errMsg = UTIL_INI_ERROR_DUP_ITEM ;
               goto error ;
            }
         }

         item = item->next ;
      }
   }

   {
      utilIniSection *section = _handler.section ;
      utilIniItem *item = NULL ;

      while( section )
      {
         string name( section->name.str, section->name.length ) ;

         rc = _appendList( sectionList, name, _handler.flags ) ;
         if ( rc )
         {
            _handler.errnum = rc ;
            _handler.errMsg = UTIL_INI_ERROR_DUP_SECTION ;
            goto error ;
         }

         item = section->item ;

         while( item )
         {
            if ( FALSE == item->isComment )
            {
               string key( item->key.str, item->key.length ) ;

               key = name + "." + key ;

               rc = _appendList( itemList, key, _handler.flags ) ;
               if ( rc )
               {
                  _handler.errnum = rc ;
                  _handler.errMsg = UTIL_INI_ERROR_DUP_ITEM ;
                  goto error ;
               }
            }

            item = item->next ;
         }

         section = section->next ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}


INT32 utilIniParserEx::printFormat( const CHAR *fileName )
{
   INT32 rc = SDB_OK ;
   stringstream ss ;
   OSSFILE file ;

   rc = ossOpen( fileName, OSS_CREATE | OSS_WRITEONLY | OSS_EXCLUSIVE,
                 0, file ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = ossTruncateFile( &file, 0 ) ;
   if ( rc )
   {
      goto error ;
   }

   ss << "== Handler ==" << endl
      << "> rc          : " << _handler.errnum << endl
      << "> last comment: " ;

   _toComment( ss, &_handler.lastComment ) ;

   ss << endl ;

   {
      utilIniItem *item = _handler.item ;

      while( item )
      {
         ss << endl
            << "-- Item --" << endl
            << "> is comment : " << ( item->isComment ? "true" : "false" ) << endl
            << "> pre comment: " ;

         _toComment( ss, &item->pre_comment ) ;

         ss << endl
            << "> pos comment: " ;

         _toComment( ss, &item->pos_comment ) ;

         ss << endl
            << "> key        : " ;

         _toString( ss, &item->key ) ;

         ss << endl
            << "> value      : " ;

         _toString( ss, &item->value ) ;

         ss << endl ;

         item = item->next ;
      }
   }

   {
      utilIniSection *section = _handler.section ;

      while( section )
      {
         ss << endl
            << "== Section ==" << endl
            << "> comment: " ;

         _toComment( ss, &section->comment ) ;

         ss << endl
            << "> name   : " ;

         _toString( ss, &section->name ) ;

         ss << endl ;

         {
            utilIniItem *item = section->item ;

            while( item )
            {
               ss << endl
                  << "    -- Item --" << endl
                  << "    > isComment  : " << ( item->isComment ? "true" : "false" ) << endl
                  << "    > pre comment: " ;

               _toComment( ss, &item->pre_comment ) ;

               ss << endl
                  << "    > pos comment: " ;

               _toComment( ss, &item->pos_comment ) ;

               ss << endl
                  << "    > key        : " ;

               _toString( ss, &item->key ) ;

               ss << endl
                  << "    > value      : " ;

               _toString( ss, &item->value ) ;

               ss << endl ;

               item = item->next ;
            }
         }

         section = section->next ;
      }
   }

   {
      string str = ss.str() ;

      rc = ossWriteN( &file, str.c_str(), (SINT64)str.length() ) ;
      if ( rc )
      {
         goto error ;
      }
   }

done:
   if ( file.isOpened() )
   {
      ossClose( file ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::toString( string &iniText, UINT32 flags )
{
   INT32 rc = SDB_OK ;
   stringstream ss ;

   if ( 0 == flags )
   {
      flags = _handler.flags ;
   }

   _itemToString( ss, _handler.item, flags ) ;

   _sectionToString( ss, _handler.section, flags ) ;

   if ( 0 < ss.str().length() && 0 < _handler.lastComment.length )
   {
      ss << "\n" ;
   }

   _toString( ss, &_handler.lastComment ) ;

   iniText = ss.str() ;

   return rc ;
}

INT32 utilIniParserEx::toBson( BSONObj &obj )
{
   INT32 rc = SDB_OK ;
   BSONObjBuilder builder ;
   utilIniSection *section = NULL ;
   utilIniItem *item = NULL ;

   item = _handler.item ;

   while( item )
   {
      if ( FALSE == item->isComment )
      {
         string name = "" ;
         string key( item->key.str, item->key.length ) ;

         _buildBson( builder, name, key ) ;
      }

      item = item->next ;
   }

   section = _handler.section ;

   while( section )
   {
      string name( section->name.str, section->name.length ) ;

      item = section->item ;

      while( item )
      {
         if ( FALSE == item->isComment )
         {
            string key( item->key.str, item->key.length ) ;

            _buildBson( builder, name, key ) ;
         }

         item = item->next ;
      }

      section = section->next ;
   }

   obj = builder.obj() ;

   return rc ;
}

INT32 utilIniParserEx::save( const CHAR *fileName, UINT32 flags )
{
   INT32 rc = SDB_OK ;
   string text ;
   OSSFILE file ;

   rc = ossOpen( fileName, OSS_CREATE | OSS_WRITEONLY | OSS_EXCLUSIVE,
                 0, file ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = ossTruncateFile( &file, 0 ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = toString( text, flags ) ;
   if ( rc )
   {
      goto error ;
   }

   rc = ossWriteN( &file, text.c_str(), (SINT64)text.length() ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   if ( file.isOpened() )
   {
      ossClose( file ) ;
   }
   return rc ;
error:
   goto done ;
}

INT32 utilIniParserEx::getError( string &errMsg )
{
   if ( _handler.errMsg )
   {
      errMsg = _handler.errMsg ;
   }
   return SDB_OK ;
}

UINT32 utilIniParserEx::flags()
{
   return _handler.flags ;
}

void utilIniParserEx::_buildBson( BSONObjBuilder &builder, const string &name,
                                  const string &key )
{
   INT32 type = UTIL_INI_EX_TYPE_STRING ;
   string fullName ;

   if ( 0 < name.length() )
   {
      fullName = name + "." + key ;
   }
   else
   {
      fullName = key ;
   }

   getType( name.c_str(), key.c_str(), type ) ;

   if ( UTIL_INI_EX_TYPE_STRING == type )
   {
      string value ;

      getValue( name.c_str(), key.c_str(), value ) ;

      builder.append( fullName, value ) ;
   }
   else if ( UTIL_INI_EX_TYPE_INT32 == type )
   {
      INT32 value ;

      getValue( name.c_str(), key.c_str(), value ) ;

      builder.append( fullName, value ) ;
   }
   else if ( UTIL_INI_EX_TYPE_INT64 == type )
   {
      INT64 value ;

      getValue( name.c_str(), key.c_str(), value ) ;

      builder.append( fullName, value ) ;
   }
   else if ( UTIL_INI_EX_TYPE_DOUBLE == type )
   {
      FLOAT64 value ;

      getValue( name.c_str(), key.c_str(), value ) ;

      builder.append( fullName, value ) ;
   }
   else if ( UTIL_INI_EX_TYPE_BOOLEAN == type )
   {
      BOOLEAN value ;

      getBoolValue( name.c_str(), key.c_str(), value ) ;

      builder.appendBool( fullName, value ) ;
   }
}

static INT32 _appendList( list<string> &strList, string &fullName,
                          UINT32 flags )
{
   INT32 rc = SDB_OK ;
   UINT32 length = fullName.length() ;
   BOOLEAN notCase = UTIL_INI_NOTCASE & flags ;
   list<string>::iterator iter ;

   for ( iter = strList.begin(); iter != strList.end(); ++iter )
   {
      if ( length == iter->length() )
      {
         BOOLEAN result = FALSE ;

         if ( notCase )
         {
            result = ( 0 == ossStrncasecmp( fullName.c_str(), iter->c_str(),
                                            length ) ) ;
         }
         else
         {
            result = ( 0 == ossStrncmp( fullName.c_str(), iter->c_str(),
                                        length ) ) ;
         }

         if ( result )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
   }

   strList.push_back( fullName ) ;

done:
   return rc ;
error:
   goto done ;
}

static INT32 _getNumberValue( utilIniHandler &handler, const CHAR *section,
                              const CHAR *key, INT32 &type,
                              utilNumberVal &value )
{
   INT32 rc = SDB_OK ;
   INT32 length    = 0 ;
   const CHAR *tmp = NULL ;

   rc = utilIniGetValue( &handler, section, key, &tmp, &length ) ;
   if ( rc )
   {
      goto error ;
   }

   utilStrToNumber( tmp, length, &type, &value, NULL ) ;

done:
   return rc ;
error:
   goto done ;
}

static void _sectionToString( stringstream &ss, utilIniSection *section,
                              UINT32 flags )
{
   while ( section )
   {
      if ( 0 < ss.str().length() )
      {
         ss << "\n" ;
      }

      if ( 0 < section->comment.length )
      {
         _toString( ss, &section->comment ) ;

         ss << "\n" ;
      }

      if ( section->name.length > 0 )
      {
         ss << "[" ;

         _toString( ss, &section->name ) ;

         ss << "]\n" ;
      }

      _itemToString( ss, section->item, flags ) ;

      section = section->next ;
   }
}

static void _itemToString( stringstream &ss, utilIniItem *item, UINT32 flags )
{
   BOOLEAN isFirst = TRUE ;

   while( item )
   {
      if ( isFirst )
      {
         isFirst = FALSE ;
      }
      else
      {
         ss << "\n" ;
      }

      if ( 0 < item->pre_comment.length )
      {
         _toString( ss, &item->pre_comment ) ;

         ss << "\n" ;
      }

      if ( item->isComment )
      {
         if ( UTIL_INI_SEMICOLON & flags )
         {
            ss << "; " ;
         }
         else if ( UTIL_INI_HASHMARK & flags )
         {
            ss << "# " ;
         }
         else
         {
            ss << "; " ;
         }
      }

      _toString( ss, &item->key ) ;

      if ( UTIL_INI_EQUALSIGN & flags )
      {
         ss << "=" ;
      }
      else if ( UTIL_INI_COLON & flags )
      {
         ss << ":" ;
      }
      else
      {
         ss << "=" ;
      }

      if ( UTIL_INI_SINGLE_QUOMARK & flags )
      {
         ss << '\'' ;
      }
      else if ( UTIL_INI_DOUBLE_QUOMARK & flags )
      {
         ss << '"' ;
      }

      _toString( ss, &item->value ) ;

      if ( UTIL_INI_SINGLE_QUOMARK & flags )
      {
         ss << '\'' ;
      }
      else if ( UTIL_INI_DOUBLE_QUOMARK & flags )
      {
         ss << '"' ;
      }

      if ( item->pos_comment.length > 0 )
      {
         ss << " " ;

         _toString( ss, &item->pos_comment ) ;
      }

      item = item->next ;
   }
}

/* parse hexadecimal number */
static BOOLEAN parseHex( const CHAR *pStr, UINT32 *code )
{
   INT32 i = 0 ;
   UINT32 h = 0 ;

   for ( i = 0; i < 4; ++i )
   {
      /* parse digit */
      if ( ( pStr[i] >= '0' ) && ( pStr[i] <= '9' ) )
      {
         h += (UINT32) pStr[i] - '0' ;
      }
      else if ( ( pStr[i] >= 'A' ) && ( pStr[i] <= 'F' ) )
      {
         h += (UINT32) 10 + pStr[i] - 'A' ;
      }
      else if ( ( pStr[i] >= 'a' ) && ( pStr[i] <= 'f' ) )
      {
         h += (UINT32) 10 + pStr[i] - 'a' ;
      }
      else
      {
         return FALSE ;
      }

      if( i < 3 )
      {
         h = h << 4 ;
      }
   }

   *code = h ;

   return TRUE ;
}

static void _toString( stringstream &ss, utilIniString *strObj )
{
   for ( INT32 i = 0; i < strObj->length; ++i )
   {
      ss << strObj->str[i] ;
   }
}

static void _toComment( stringstream &ss, utilIniString *strObj )
{
   for ( INT32 i = 0; i < strObj->length; ++i )
   {
      if ( '\r' == strObj->str[i] || '\n' == strObj->str[i] )
      {
         ss << " " ;
      }
      else
      {
         ss << strObj->str[i] ;
      }
   }
}

/* converts a UTF-16 literal to UTF-8
 * A literal can be one or two sequences of the form \uXXXX */
static INT32 utf16ToUtf8( const CHAR *pStr, INT32 length, CHAR **pOut )
{
   BYTE firstByteMark = 0 ;
   INT32 utf8Length = 0 ;
   INT32 utf8Position = 0 ;
   INT32 sequenceLength = 0 ;
   UINT32 firstCode = 0 ;
   UINT64 codepoint = 0 ;
   const CHAR *firstSequence = pStr;

   if ( length < 6 )
   {
      /* input ends unexpectedly */
      goto error ;
   }

   /* get the first utf16 sequence */
   if ( parseHex( firstSequence + 2, &firstCode ) == FALSE )
   {
      goto error ;
   }

   /* check that the code is valid */
   if ( ( ( firstCode >= 0xDC00 ) && ( firstCode <= 0xDFFF ) ) )
   {
      goto error ;
   }

   sequenceLength = 6 ;

   /* UTF16 surrogate pair */
   if ( ( firstCode >= 0xD800 ) && ( firstCode <= 0xDBFF ) )
   {
      UINT32 secondCode = 0;
      const CHAR *secondSequence = firstSequence + 6 ;

      length -= sequenceLength ;
      if ( length < 6 )
      {
         /* input ends unexpectedly */
         goto error ;
      }

      if ( ( secondSequence[0] != '\\' ) || ( secondSequence[1] != 'u' ) )
      {
         /* missing second half of the surrogate pair */
         goto error ;
      }

      /* get the second utf16 sequence */
      if ( parseHex( secondSequence + 2, &secondCode ) )
      {
         goto error ;
      }

      /* check that the code is valid */
      if ( ( secondCode < 0xDC00 ) || ( secondCode > 0xDFFF ) )
      {
         /* invalid second half of the surrogate pair */
         goto error ;
      }

      sequenceLength += 6 ;

      /* calculate the unicode codepoint from the surrogate pair */
      codepoint = 0x10000 +
               ( ( ( firstCode & 0x3FF ) << 10 ) | ( secondCode & 0x3FF ) ) ;
   }
   else
   {
      codepoint = firstCode ;
   }

   /* encode as UTF-8
   * takes at maximum 4 bytes to encode:
   * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
   if ( codepoint < 0x80 )
   {
      /* normal ascii, encoding 0xxxxxxx */
      utf8Length = 1 ;
   }
   else if ( codepoint < 0x800 )
   {
      /* two bytes, encoding 110xxxxx 10xxxxxx */
      utf8Length = 2 ;
      firstByteMark = 0xC0 ; /* 11000000 */
   }
   else if ( codepoint < 0x10000 )
   {
      /* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
      utf8Length = 3 ;
      firstByteMark = 0xE0 ; /* 11100000 */
   }
   else if (codepoint <= 0x10FFFF)
   {
      /* four bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx 10xxxxxx */
      utf8Length = 4 ;
      firstByteMark = 0xF0 ; /* 11110000 */
   }
   else
   {
      /* invalid unicode codepoint */
      goto error ;
   }

   /* encode as utf8 */
   
   for ( utf8Position = utf8Length - 1; utf8Position > 0; --utf8Position )
   {
      /* 10xxxxxx */
      (*pOut)[utf8Position] = (CHAR)( ( codepoint | 0x80 ) & 0xBF ) ;
      codepoint >>= 6 ;
   }

   /* encode first byte */
   if ( utf8Length > 1 )
   {
      (*pOut)[0] = (CHAR)( ( codepoint | firstByteMark ) & 0xFF ) ;
   }
   else
   {
      (*pOut)[0] = (CHAR)( codepoint & 0x7F ) ;
   }

   *pOut += utf8Length ;

done:
    return sequenceLength ;
error:
   sequenceLength = 0 ;
   goto done ;
}
