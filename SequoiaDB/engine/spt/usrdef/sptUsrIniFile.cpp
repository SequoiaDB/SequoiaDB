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

   Source File Name = sptUsrFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2019  HJW Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrIniFile.hpp"
#include "pd.hpp"
#include "ossCmdRunner.hpp"
#include "omagentDef.hpp"
#include "sptUsrRemote.hpp"

#if defined(_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace std ;
using namespace bson ;

namespace engine
{
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, setValue )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, getValue )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, setSectionComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, getSectionComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, setComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, getComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, setLastComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, getLastComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, enableItem )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, disableItem )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, disableAllItem )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, toString )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, toObj )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, save )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, getFileName )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, getFlags )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, convertComment )
JS_MEMBER_FUNC_DEFINE( _sptUsrIniFile, comment2String )
JS_CONSTRUCT_FUNC_DEFINE( _sptUsrIniFile, construct )
JS_DESTRUCT_FUNC_DEFINE( _sptUsrIniFile, destruct )

JS_BEGIN_MAPPING( _sptUsrIniFile, "IniFile" )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_setValue",             setValue, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getValue",             getValue, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_setSectionComment",    setSectionComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getSectionComment",    getSectionComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_setComment",           setComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getComment",           getComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_setLastComment",       setLastComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getLastComment",       getLastComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_enableItem",           enableItem, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_disableItem",          disableItem, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_disableAllItem",       disableAllItem, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_toString",             toString, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_toObj",                toObj, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_save",                 save, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getFileName",          getFileName, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getFlags",             getFlags, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_convertComment",       convertComment, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_comment2String",       comment2String, 0 )
   JS_ADD_CONSTRUCT_FUNC( construct )
   JS_ADD_DESTRUCT_FUNC( destruct )
JS_MAPPING_END()

   _sptUsrIniFile::_sptUsrIniFile()
   {
   }

   _sptUsrIniFile::~_sptUsrIniFile()
   {
   }

   INT32 _sptUsrIniFile::construct( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc    = SDB_OK ;
      INT32 flags = UTIL_INI_EX_FLAGS_DEFAULT ;
      UINT32 argc = arg.argc() ;
      string content ;

      if ( 0 == argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument filename" ) ;
         goto error ;
      }

      rc = arg.getString( 0, _fileName ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Filename must be string" ) ;
         goto error ;
      }

      if ( 1 < argc )
      {
         rc = arg.getNative( 1, (void*)&flags, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "flags must be INT32" ) ;
            goto error ;
         }
      }

      if ( 2 < argc )
      {
         rc = arg.getString( 2, content ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "content must be string" ) ;
            goto error ;
         }
      }

      if ( 0 < content.length() )
      {
         rc = _parser.parse( content.c_str(), content.length(), flags ) ;
      }
      else
      {
         rc = _parser.parse( _fileName.c_str(), flags ) ;
      }

      if ( rc )
      {
         _setError( detail, "Failed to parse file" ) ;

         PD_LOG( PDERROR, "Failed to parse file:%s, rc:%d",
                 _fileName.c_str(), rc ) ;
         goto error ;
      }

      _flags = _parser.flags() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::destruct()
   {
      INT32 rc = SDB_OK ;
      return rc ;
   }

   INT32 _sptUsrIniFile::setValue( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 argc = arg.argc() ;
      string name ;
      string key ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument key" ) ;
         goto error ;
      }
      else if ( 3 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument value" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, key ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Key must be string" ) ;
         goto error ;
      }

      if ( arg.isString( 2 ) )
      {
         string value ;

         rc = arg.getString( 2, value ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _parser.setValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to set value" ) ;
            goto error ;
         }
      }
      else if ( arg.isBoolean( 2 ) )
      {
         INT32 value ;

         rc = arg.getNative( 2, (void*)&value, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _parser.setBoolValue( name.c_str(), key.c_str(),
                                    value ? TRUE : FALSE ) ;
         if ( rc )
         {
            _setError( detail, "Failed to set value" ) ;
            goto error ;
         }
      }
      else if ( arg.isInt( 2 ) || arg.isLong( 2 ) )
      {
         INT64 value ;

         rc = arg.getNative( 2, (void*)&value, SPT_NATIVE_INT64 ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _parser.setValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to set value" ) ;
            goto error ;
         }
      }
      else if ( arg.isDouble( 2 ) )
      {
         FLOAT64 value ;

         rc = arg.getNative( 2, (void*)&value, SPT_NATIVE_FLOAT64 ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _parser.setValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to set value" ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         _setError( detail, "Invalid parameter type" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::getValue( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc    = SDB_OK ;
      INT32 type  = UTIL_INI_EX_TYPE_STRING ;
      UINT32 argc = arg.argc() ;
      string name ;
      string key ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument key" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, key ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Key must be string" ) ;
         goto error ;
      }

      rc = _parser.getType( name.c_str(), key.c_str(), type ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         _setError( detail, "Failed to get value" ) ;
         goto error ;
      }

      if ( UTIL_INI_EX_TYPE_INT32 == type )
      {
         INT32 value ;

         rc = _parser.getValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to get value" ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( value ) ;
      }
      else if ( UTIL_INI_EX_TYPE_INT64 == type )
      {
         INT64 value ;

         rc = _parser.getValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to get value" ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( value ) ;
      }
      else if ( UTIL_INI_EX_TYPE_DOUBLE == type )
      {
         FLOAT64 value ;

         rc = _parser.getValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to get value" ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( value ) ;
      }
      else if ( UTIL_INI_EX_TYPE_BOOLEAN == type )
      {
         BOOLEAN value ;

         rc = _parser.getBoolValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to get value" ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( value ? true : false ) ;
      }
      else
      {
         string value ;

         rc = _parser.getValue( name.c_str(), key.c_str(), value ) ;
         if ( rc )
         {
            _setError( detail, "Failed to get value" ) ;
            goto error ;
         }

         rval.getReturnVal().setValue( value ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::setSectionComment( const _sptArguments &arg,
                                            _sptReturnVal &rval,
                                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 argc = arg.argc() ;
      string name ;
      string comment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument comment" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, comment ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Comment must be string" ) ;
         goto error ;
      }

      rc = _parser.setSectionComment( name.c_str(), comment ) ;
      if ( rc )
      {
         _setError( detail, "Failed to set section comment" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::getSectionComment( const _sptArguments &arg,
                                            _sptReturnVal &rval,
                                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 argc = arg.argc() ;
      string name ;
      string comment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = _parser.getSectionComment( name.c_str(), comment ) ;
      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         _setError( detail, "Failed to get section comment" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( comment ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::setComment( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc    = SDB_OK ;
      INT32 pos   = 1 ;
      UINT32 argc = arg.argc() ;
      string name ;
      string key ;
      string comment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument key" ) ;
         goto error ;
      }
      else if ( 3 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument comment" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, key ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Key must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 2, comment ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Comment must be string" ) ;
         goto error ;
      }

      if ( 3 < argc )
      {
         if ( FALSE == arg.isBoolean( 3 ) )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Pos must be boolean" ) ;
            goto error ;
         }

         rc = arg.getNative( 3, (void*)&pos, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( pos )
      {
         rc = _parser.setItemPreComment( name.c_str(), key.c_str(),
                                         comment ) ;
      }
      else
      {
         rc = _parser.setItemPosComment( name.c_str(), key.c_str(),
                                         comment ) ;
      }

      if ( rc )
      {
         _setError( detail, "Failed to set item comment" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::getComment( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 pos = 1 ;
      UINT32 argc = arg.argc() ;
      string name ;
      string key ;
      string comment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument key" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, key ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Key must be string" ) ;
         goto error ;
      }

      if ( 2 < argc )
      {
         if ( FALSE == arg.isBoolean( 2 ) )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Pos must be boolean" ) ;
            goto error ;
         }

         rc = arg.getNative( 2, (void*)&pos, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      if ( pos )
      {
         rc = _parser.getItemPreComment( name.c_str(), key.c_str(), comment ) ;
      }
      else
      {
         rc = _parser.getItemPosComment( name.c_str(), key.c_str(), comment ) ;
      }

      if ( SDB_FIELD_NOT_EXIST == rc )
      {
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         _setError( detail, "Failed to get item comment" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( comment ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::setLastComment( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 argc = arg.argc() ;
      string comment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument comment" ) ;
         goto error ;
      }

      rc = arg.getString( 0, comment ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Comment must be string" ) ;
         goto error ;
      }

      rc = _parser.setLastComment( comment ) ;
      if ( rc )
      {
         _setError( detail, "Failed to set last comment" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::getLastComment( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string comment ;

      rc = _parser.getLastComment( comment ) ;
      if ( rc )
      {
         _setError( detail, "Failed to get last comment" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( comment ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::enableItem( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 argc = arg.argc() ;
      string name ;
      string key ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument key" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, key ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Key must be string" ) ;
         goto error ;
      }

      rc = _parser.setCommentItem( name.c_str(), key.c_str(), FALSE ) ;
      if ( rc )
      {
         _setError( detail, "Failed to enable item" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::disableItem( const _sptArguments &arg,
                                      _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      UINT32 argc = arg.argc() ;
      string name ;
      string key ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument section" ) ;
         goto error ;
      }
      else if ( 2 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument key" ) ;
         goto error ;
      }

      rc = arg.getString( 0, name ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Section must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, key ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Key must be string" ) ;
         goto error ;
      }

      rc = _parser.setCommentItem( name.c_str(), key.c_str(), TRUE ) ;
      if ( rc )
      {
         _setError( detail, "Failed to disable item" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::disableAllItem( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      _parser.commentAllItems() ;

      return SDB_OK ;
   }

   INT32 _sptUsrIniFile::toString( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string text ;

      rc = _parser.toString( text ) ;
      if ( rc )
      {
         _setError( detail, "Failed to convert ini text" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( text ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::toObj( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      rc = _parser.toBson( obj ) ;
      if ( rc )
      {
         _setError( detail, "Failed to convert object" ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( obj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::save( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      rc = _parser.save( _fileName.c_str() ) ;
      if ( rc )
      {
         _setError( detail, "Failed to save file" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::getFileName( const _sptArguments &arg, _sptReturnVal &rval,
                                      bson::BSONObj &detail )
   {
      rval.getReturnVal().setValue( _fileName ) ;

      return SDB_OK ;
   }

   INT32 _sptUsrIniFile::getFlags( const _sptArguments &arg, _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      rval.getReturnVal().setValue( _flags ) ;

      return SDB_OK ;
   }

   INT32 _sptUsrIniFile::convertComment( const _sptArguments &arg, _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc      = SDB_OK ;
      INT32 length  = 0 ;
      UINT32 argc   = arg.argc() ;
      string comment ;
      string newComment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument Comment" ) ;
         goto error ;
      }

      rc = arg.getString( 0, comment ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Comment must be string" ) ;
         goto error ;
      }

      length = comment.length() ;

      if ( 0 < length )
      {
         INT32 i = 0 ;
         const CHAR *p = NULL ;
         string annotator ;

         if ( UTIL_INI_SEMICOLON & _flags )
         {
            annotator = "; " ;
         }
         else if ( UTIL_INI_HASHMARK & _flags )
         {
            annotator = "# " ;
         }
         else
         {
            annotator = "; " ;
         }

         p = comment.c_str() ;

         for ( i = 0; i < length; ++i )
         {
            newComment += p[i] ;

            if ( '\n' == p[i] )
            {
               newComment += annotator ;
            }
         }

         newComment = annotator + newComment ;
      }

      rval.getReturnVal().setValue( newComment ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrIniFile::comment2String( const _sptArguments &arg, _sptReturnVal &rval,
                                         bson::BSONObj &detail )
   {
      INT32 rc      = SDB_OK ;
      INT32 length  = 0 ;
      UINT32 argc   = arg.argc() ;
      string comment ;
      string newComment ;

      if ( 1 > argc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Missing argument Comment" ) ;
         goto error ;
      }

      rc = arg.getString( 0, comment ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "Comment must be string" ) ;
         goto error ;
      }

      length = comment.length() ;

      if ( 0 < length )
      {
         BOOLEAN isCommentChar = FALSE ;
         INT32 i = 0 ;
         const CHAR *p = comment.c_str() ;

         for ( i = 0; i < length; ++i )
         {
            if ( TRUE == isCommentChar && ( ' ' == p[i] || '\t' == p[i] ) )
            {
               continue ;
            }
            else
            {
               isCommentChar = FALSE ;

               if ( i == 0 && UTIL_INI_SEMICOLON & _flags && ';' == p[i] )
               {
                  isCommentChar = TRUE ;
                  continue ;
               }
               else if ( i == 0 && UTIL_INI_HASHMARK & _flags && '#' == p[i] )
               {
                  isCommentChar = TRUE ;
                  continue ;
               }
               else if ( i > 0 && UTIL_INI_SEMICOLON & _flags && ';' == p[i] &&
                         '\n' == p[i-1] )
               {
                  isCommentChar = TRUE ;
                  continue ;
               }
               else if ( i > 0 && UTIL_INI_HASHMARK & _flags && '#' == p[i] &&
                         '\n' == p[i-1] )
               {
                  isCommentChar = TRUE ;
                  continue ;
               }
               else
               {
                  newComment += p[i] ;
               }
            }
         }
      }

      rval.getReturnVal().setValue( newComment ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptUsrIniFile::_setError( bson::BSONObj &detail, const CHAR *errMsg )
   {
      string errInfo ;

      _parser.getError( errInfo ) ;

      if ( 0 < errInfo.length() )
      {
         errInfo = ": " + errInfo ;
         errInfo = errMsg + errInfo ;

         detail = BSON( SPT_ERR << errInfo ) ;
      }
      else
      {
         detail = BSON( SPT_ERR << errMsg ) ;
      }
   }

   INT32 _sptUsrIniFile::_exist( const string &path, BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      isExist = FALSE ;

      rc = ossAccess( path.c_str() ) ;
      if ( rc )
      {
         if ( SDB_FNE == rc )
         {
            rc = SDB_OK ;
            isExist = FALSE ;
         }
         else
         {
            goto error ;
         }
      }
      else
      {
         isExist = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}
