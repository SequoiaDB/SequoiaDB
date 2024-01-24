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
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrFile.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "ossCmdRunner.hpp"
#include "omagentDef.hpp"
#include "sptUsrFileContent.hpp"
#include "sptUsrRemote.hpp"
#include <boost/algorithm/string.hpp>
#include "../bson/lib/md5.hpp"

#if defined(_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace std ;
using namespace bson ;

namespace engine
{
   #define SPT_FILE_PROPERTY_FILENAME   "_filename"
   #define FILE_TRANSFORM_UNIT_512K 524288
   #define SPT_READ_LEN 1024

JS_MEMBER_FUNC_DEFINE( _sptUsrFile, read )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, seek )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, write )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, truncate )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, readLine )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, readContent )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, writeContent )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, close )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, getInfo )
JS_MEMBER_FUNC_DEFINE( _sptUsrFile, toString )
JS_CONSTRUCT_FUNC_DEFINE( _sptUsrFile, construct )
JS_DESTRUCT_FUNC_DEFINE( _sptUsrFile, destruct )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, remove )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, exist )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, copy )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, move )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, mkdir )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getFileObj )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, md5 )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, readFile )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, find )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, list )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, chmod )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, chown )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, chgrp )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getPathType )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getUmask )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, setUmask )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, isEmptyDir )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getStat )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getFileSize )
JS_STATIC_FUNC_DEFINE( _sptUsrFile, getPermission )

JS_BEGIN_MAPPING( _sptUsrFile, "File" )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_read", read, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_write", write, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_truncate", truncate, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_readLine", readLine, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_readContent", readContent, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_writeContent", writeContent, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_close", close, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_seek", seek, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_getInfo", getInfo, 0 )
   JS_ADD_MEMBER_FUNC_WITHATTR( "_toString", toString, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_getFileObj", getFileObj, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_readFile", readFile, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_getPathType", getPathType, 0 )
   JS_ADD_STATIC_FUNC( "remove", remove )
   JS_ADD_STATIC_FUNC( "exist", exist )
   JS_ADD_STATIC_FUNC( "copy", copy )
   JS_ADD_STATIC_FUNC( "move", move )
   JS_ADD_STATIC_FUNC( "mkdir", mkdir )
   JS_ADD_STATIC_FUNC( "setUmask", setUmask )
   JS_ADD_STATIC_FUNC_WITHATTR( "_getUmask", getUmask, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_list", list, 0 )
   JS_ADD_STATIC_FUNC_WITHATTR( "_find", find, 0 )
   JS_ADD_STATIC_FUNC( "chmod", chmod )
   JS_ADD_STATIC_FUNC( "chown", chown )
   JS_ADD_STATIC_FUNC( "chgrp", chgrp )
   JS_ADD_STATIC_FUNC( "isEmptyDir", isEmptyDir )
   JS_ADD_STATIC_FUNC( "stat", getStat )
   JS_ADD_STATIC_FUNC( "md5", md5 )
   JS_ADD_STATIC_FUNC( "_getPermission", getPermission )
   JS_ADD_STATIC_FUNC( "getSize", getFileSize )
   JS_ADD_CONSTRUCT_FUNC( construct )
   JS_ADD_DESTRUCT_FUNC( destruct )
JS_MAPPING_END()

   _sptUsrFile::_sptUsrFile()
   {
   }

   _sptUsrFile::~_sptUsrFile()
   {
   }

   INT32 _sptUsrFile::construct( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string filename ;
      INT32 permission = 0 ;
      INT32 iMode = 0 ;
      BSONObjBuilder opBuilder ;
      string err ;

      // get filename
      rc = arg.getString( 0, filename ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Filename must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Filename must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filename, rc: %d", rc ) ;

      // get mode
      if ( arg.argc() > 1 )
      {
         rc = arg.getNative( 1, (void*)&permission, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "Permission must be INT32" ) ;
            goto error ;
         }
         opBuilder.append( SPT_FILE_COMMON_FIELD_PERMISSION, permission ) ;
      }

      if( arg.argc() > 2 )
      {
         rc = arg.getNative( 2, &iMode, SPT_NATIVE_INT32 ) ;
         if( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Mode must be int" ) ;
            goto error ;
         }
         opBuilder.append( SPT_FILE_COMMON_FIELD_MODE, iMode ) ;
      }

      rc = _fileCommon.open( filename, opBuilder.obj(), err ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         PD_LOG( PDERROR, "Failed to open file:%s, rc:%d",
                 filename.c_str(), rc ) ;
         goto error ;
      }
      rval.addSelfProperty( SPT_FILE_PROPERTY_FILENAME )->setValue( filename ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::getFileObj( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      // new File Object and set return value
      _sptUsrFile * fileObj = SDB_OSS_NEW _sptUsrFile() ;
      if ( !fileObj )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Create object failed" ) ;
         goto error ;
      }
      else
      {
         rval.setUsrObjectVal<_sptUsrFile>( fileObj ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::destruct()
   {
      INT32 rc = SDB_OK ;
      string err ;
      rc = _fileCommon.close( err ) ;
      return rc ;
   }

   INT32 _sptUsrFile::read( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 len = 0 ;
      CHAR *buf = NULL ;

      rc = _readContentLocal( arg, detail, &buf, len ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      rval.getReturnVal().setValue( buf ) ;
   done:
      if ( NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
         buf = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::readLine( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 len = 0 ;
      CHAR *buf = NULL ;
      string err ;
      rc = _fileCommon.readLine( err, &buf, len ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err.c_str() ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( buf ) ;
   done:
      if( NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
         buf = NULL ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::write( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string content ;
      string err ;

      // get content
      rc = arg.getString( 0, content ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Content must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Content must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get content, rc: %d", rc ) ;

      rc = _fileCommon.write( content.c_str(), content.size(), err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err.c_str() ) ;
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::readContent( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      SINT64 len = 0 ;
      CHAR *buf = NULL ;
      sptUsrFileContent *fileContent = NULL ;

      if( arg.isUserObj( 0, _sptUsrRemote::__desc ) )
      {
         rc = _readContentRemote( arg, detail, &buf, len ) ;
      }
      else
      {
         rc = _readContentLocal( arg, detail, &buf, len ) ;
      }
      if( SDB_OK != rc )
      {
         goto error ;
      }

      fileContent = SDB_OSS_NEW sptUsrFileContent() ;
      rc = fileContent->init( buf, len ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to init FileContent obj" ) ;
         goto error ;
      }

      rval.setUsrObjectVal<sptUsrFileContent>( fileContent ) ;
   done:
      if( NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::writeContent( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      if( arg.isUserObj( 0, _sptUsrRemote::__desc ) )
      {
         rc = _writeContentRemote( arg, detail ) ;
      }
      else
      {
         rc = _writeContentLocal( arg, detail ) ;
      }
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::_readContentLocal( const _sptArguments &arg,
                                         bson::BSONObj &detail,
                                         CHAR** buf, SINT64 &len )
   {
      INT32 rc = SDB_OK ;
      SINT64 readLen = SPT_READ_LEN ;
      BSONObjBuilder optionBuilder ;
      string err ;

      //get read length
      rc = arg.getNative( 0, &readLen, SPT_NATIVE_INT64 ) ;
      if( SDB_OK == rc )
      {
         optionBuilder.append( SPT_FILE_COMMON_FIELD_SIZE, readLen ) ;
      }
      else if( SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Size must be number" ) ;
         PD_LOG( PDERROR, "Failed to get size, rc: %d", rc ) ;
         goto error ;
      }

      rc = _fileCommon.read( optionBuilder.obj(), err, buf, len ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err.c_str() ) ;
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::_readContentRemote( const _sptArguments &arg,
                                          bson::BSONObj &detail,
                                          CHAR** buf, SINT64 &len )
   {
      INT32 rc = SDB_OK ;
      _sptUsrRemote *pRemote = NULL ;
      SINT64 size = SPT_READ_LEN ;
      UINT32 fID = 0 ;
      SINT64 hasRead = 0 ;
      SINT64 readTimes = 0 ;
      const CHAR* retBuf = NULL ;

      rc = arg.getUserObj( 0, _sptUsrRemote::__desc, ( const void** )&pRemote ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "RemoteObj must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "RemoteObj must be Remote" ) ;
         goto error ;
      }

      rc = arg.getNative( 1, (void*)&fID, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "FID must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "FID must be numberInt" ) ;
         goto error ;
      }

      rc = arg.getNative( 2, (void*)&size, SPT_NATIVE_INT64 ) ;
      if( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Size must be number" ) ;
         goto error ;
      }
      if( size < 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Size must be zero or positive number" ) ;
         goto error ;
      }

      *buf = ( CHAR* )SDB_OSS_MALLOC( size + 1 ) ;
      if( NULL == *buf )
      {
         rc = SDB_OOM ;
         detail = BSON( SPT_ERR << "Failed to alloc buff" ) ;
         goto error ;
      }

      hasRead = 0 ;
      readTimes = 0 ;
      while( hasRead < size )
      {
         BSONObj retObj ;
         SINT64 readLen = 0 ;
         SINT32 realRead = 0 ;
         BSONElement ele ;
         if( size - hasRead > FILE_TRANSFORM_UNIT_512K )
         {
            readLen = FILE_TRANSFORM_UNIT_512K ;
         }
         else
         {
            readLen = size - hasRead ;
         }

         rc = pRemote->runCommand( OMA_REMOTE_FILE_READ,
                                   BSON( "IsBinary" << TRUE ),
                                   BSON( "FID" << fID ),
                                   BSON( "Size" << readLen ), detail, retObj ) ;
         if( SDB_OK != rc )
         {
            if( 0 < readTimes && SDB_EOF == rc )
            {
               rc = SDB_OK ;
               break ;
            }
            goto error ;
         }

         if( FALSE == retObj.hasField( "Content" ) )
         {
            rc = SDB_OUT_OF_BOUND ;
            detail = BSON( SPT_ERR << "RetObj must has field: 'Content'" ) ;
            goto error ;
         }
         ele = retObj.getField( "Content" ) ;
         if( BinData != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            detail = BSON( SPT_ERR << "Content must be binary" ) ;
            goto error ;
         }
         retBuf = ele.binData( realRead );

         ossMemcpy( ( *buf ) + hasRead, retBuf, realRead ) ;
         readTimes++ ;
         hasRead += realRead ;
      }

      (*buf)[hasRead] = '\0' ;
      len = hasRead ;
   done:
      return rc ;
   error:
      if( *buf )
      {
         SDB_OSS_FREE( *buf ) ;
         *buf = NULL ;
      }
      goto done ;
   }

   INT32 _sptUsrFile::_writeContentLocal( const _sptArguments &arg,
                                          bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      sptUsrFileContent *pFileContent = NULL ;
      string err ;

      rc = arg.getUserObj( 0, _sptUsrFileContent::__desc,
                           (const void**)&pFileContent ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "FileContent must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "FileContent must be FileContent" ) ;
         goto error ;
      }

      rc = _fileCommon.write( pFileContent->getBuf(), pFileContent->getLength(),
                             err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err ) ;
         detail = BSON( SPT_ERR << "Failed to write fileContent to file" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::_writeContentRemote( const _sptArguments &arg,
                                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      _sptUsrRemote *pRemote = NULL ;
      sptUsrFileContent *pFileContent = NULL ;
      UINT32 fID = 0 ;
      SINT64 size = 0 ;
      SINT64 hasWrite = 0 ;
      BSONObj retObj ;

      rc = arg.getUserObj( 0, _sptUsrRemote::__desc, ( const void** )&pRemote ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "RemoteObj must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "RemoteObj must be Remote" ) ;
         goto error ;
      }

      rc = arg.getNative( 1, (void*)&fID, SPT_NATIVE_INT32 ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "FID must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "FID must be numberInt" ) ;
         goto error ;
      }

      rc = arg.getUserObj( 2, _sptUsrFileContent::__desc,
                           (const void**)&pFileContent ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "FileContent must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "FileContent must be FileContent" ) ;
         goto error ;
      }

      size = pFileContent->getLength() ;
      hasWrite = 0 ;
      while( size > hasWrite )
      {
         SINT32 writeSize ;
         BSONObjBuilder builder ;

         if( size - hasWrite > FILE_TRANSFORM_UNIT_512K )
         {
            writeSize = FILE_TRANSFORM_UNIT_512K ;
         }
         else
         {
            writeSize = size - hasWrite ;
         }
         builder.appendBinData( "Content", writeSize, BinDataGeneral,
                                pFileContent->getBuf() + hasWrite ) ;
         rc = pRemote->runCommand( OMA_REMOTE_FILE_WRITE,
                                   BSONObj(),
                                   BSON( "FID" << fID ),
                                   builder.obj(), detail, retObj ) ;
         hasWrite += writeSize ;
      }
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::truncate( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT64 size = 0 ;
      string err ;

      rc = arg.getNative( 0, &size, SPT_NATIVE_INT64 ) ;
      if ( rc && SDB_OUT_OF_BOUND != rc )
      {
         detail = BSON( SPT_ERR << "Size must be native type" ) ;
         goto error ;
      }

      rc = _fileCommon.truncate( size, err ) ;
      if( rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::seek( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT64 seekSize = 0 ;
      string whenceStr ;
      BSONObj optionObj ;
      string err ;

      rc = arg.getNative( 0, &seekSize, SPT_NATIVE_INT64 ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Offset must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Offset must be native type" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get offset, rc: %d", rc ) ;

      rc = arg.getBsonobj( 1, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "OptionObj must be config" ) ;
         goto error ;
      }
      else if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "OptionObj must be obj" );
         goto error ;
      }

      rc = _fileCommon.seek( seekSize, optionObj, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::toString( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                bson::BSONObj & detail )
   {
      rval.getReturnVal().setValue( _fileCommon.getFileName() ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrFile::getInfo( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj remoteInfo ;
      BSONObjBuilder builder ;

      // get information about Object
      rc = arg.getBsonobj( 0, remoteInfo ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "RemoteInfo must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "RemoteInfo must be obj" ) ;
         goto error ;
      }

      // if it is not a remote obj, append local filename
      if ( FALSE == remoteInfo.getBoolField( "isRemote" ) )
      {
         builder.append( "filename", _fileCommon.getFileName() ) ;
      }

      // build result
      builder.append( "type", "File" ) ;
      builder.appendElements( remoteInfo ) ;
      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::remove( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string fullPath ;
      string err ;

      rc = arg.getString( 0, fullPath ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Filepath must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Filepath must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filepath, rc: %d", rc ) ;

      rc = _sptUsrFileCommon::remove( fullPath, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::exist( const _sptArguments & arg,
                             _sptReturnVal & rval,
                             BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string fullPath ;
      string err ;
      BOOLEAN fileExist = FALSE ;

      rc = arg.getString( 0, fullPath ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Filepath must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Filepath must be string" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filepath, rc: %d", rc ) ;

      rc = _sptUsrFileCommon::exist( fullPath, err, fileExist ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( fileExist ? true : false ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::copy( const _sptArguments & arg,
                            _sptReturnVal & rval,
                            BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;
      INT32 isReplace = TRUE ;
      BSONObjBuilder opBuilder ;
      string err ;

      rc = arg.getString( 0, src ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Src is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Src must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, dst ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Dst is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Dst must be string" ) ;
         goto error ;
      }

      if ( arg.argc() > 2 )
      {
         rc = arg.getNative( 2, (void*)&isReplace, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "IsReplace must be BOOLEAN" ) ;
            goto error ;
         }
         opBuilder.appendBool( SPT_FILE_COMMON_FIELD_IS_REPLACE, isReplace ) ;
      }

      if ( arg.argc() > 3 )
      {
         INT32 mode = 0 ;
         rc = arg.getNative( 3, (void*)&mode, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "Mode must be INT32" ) ;
            goto error ;
         }
         opBuilder.append( SPT_FILE_COMMON_FIELD_MODE, mode ) ;
      }


      rc = _sptUsrFileCommon::copy( src, dst, opBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::move( const _sptArguments & arg,
                            _sptReturnVal & rval,
                            BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;
      string err ;

      rc = arg.getString( 0, src ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Src is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Src must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, dst ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Dst is required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Dst must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::move( src, dst, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::mkdir( const _sptArguments & arg,
                             _sptReturnVal & rval,
                             BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string name ;
      BSONObjBuilder opBuilder ;
      string err ;

      rc = arg.getString( 0, name ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Dirname must be required" ) ;
         goto error ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "Dirname must be string" ) ;
         goto error ;
      }

      if ( arg.argc() > 1 )
      {
         INT32 mode = 0 ;
         rc = arg.getNative( 1, (void*)&mode, SPT_NATIVE_INT32 ) ;
         if ( rc )
         {
            detail = BSON( SPT_ERR << "Mode must be INT32" ) ;
            goto error ;
         }
         opBuilder.append( "mode", mode ) ;
      }

      rc = _sptUsrFileCommon::mkdir( name, opBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::close( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _fileCommon.close( err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, err.c_str() ) ;
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // Read all the contents of the file
   INT32 _sptUsrFile::readFile( const _sptArguments & arg,
                                _sptReturnVal & rval,
                                BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      CHAR* buf = NULL ;
      string name ;
      string err ;
      INT64 readSize = 0 ;

      rc = arg.getString( 0, name ) ;
      if( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Filename must be config" ) ;
         goto error ;
      }
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Filename must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::readFile( name, err, &buf, readSize ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( buf ) ;
   done:
      SDB_OSS_FREE( buf ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::find( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj retObj ;
      BSONObj optionObj ;
      string err ;

      rc = arg.getBsonobj( 0, optionObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "OptionObj must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "OptionObj must be BSONObj" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::find( optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }

      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::list( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj optionObj ;
      BSONObj retObj ;
      string err ;

      if ( 1 <= arg.argc() )
      {
         rc = arg.getBsonobj( 0, optionObj ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "OptionObj must be BSONObj" ) ;
            goto error ;
         }
      }

      rc = _sptUsrFileCommon::list( optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::chmod( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isRecursive = FALSE ;
      BSONObjBuilder opBuilder ;
      string pathname ;
      INT32 mode = 0 ;
      string err ;

      // read argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be string" ) ;
         goto error ;
      }

      rc = arg.getNative( 1, &mode, SPT_NATIVE_INT32 ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Mode must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Mode must be INT32" ) ;
         goto error ;
      }

      if ( 3 <= arg.argc() )
      {
         rc = arg.getNative( 2, &isRecursive, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "IsRecursive must be bool" ) ;
            goto error ;
         }
         opBuilder.appendBool( SPT_FILE_COMMON_FIELD_IS_RECURSIVE, isRecursive ) ;
      }

      rc = _sptUsrFileCommon::chmod( pathname, mode, opBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::chown( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isRecursive = FALSE ;
      string pathname ;
      BSONObj ownerObj ;
      BSONObjBuilder opBuilder ;
      string err ;

      // raed arugment
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be string" ) ;
         goto error ;
      }

      rc = arg.getBsonobj( 1, ownerObj ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "OptionObj must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "OptionObj must be BSONObj" ) ;
         goto error ;
      }

      if ( 3 <= arg.argc() )
      {
         rc = arg.getNative( 2, &isRecursive, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "IsRecursive must be bool" ) ;
            goto error ;
         }
         opBuilder.appendBool( SPT_FILE_COMMON_FIELD_IS_RECURSIVE, isRecursive ) ;
      }

      rc = _sptUsrFileCommon::chown( pathname, ownerObj, opBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::chgrp( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isRecursive = FALSE ;
      string pathname ;
      string groupname ;
      string err ;
      BSONObjBuilder opBuilder ;

      // get argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be config" ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be string" ) ;
         goto error ;
      }

      rc = arg.getString( 1, groupname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Groupname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Groupname must be string" ) ;
         goto error ;
      }

      if ( 3 <= arg.argc() )
      {
         rc = arg.getNative( 2, &isRecursive, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "IsRecursive must be bool" ) ;
            goto error ;
         }
         opBuilder.appendBool( SPT_FILE_COMMON_FIELD_IS_RECURSIVE, isRecursive ) ;
      }

      rc = _sptUsrFileCommon::chgrp( pathname, groupname, opBuilder.obj(), err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::getUmask( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string err ;
      string retStr ;

      rc = _sptUsrFileCommon::getUmask( err, retStr ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( retStr ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::setUmask( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      INT32 mask = 0 ;
      string err ;

      // get argument
      rc = arg.getNative( 0, (void*)&mask, SPT_NATIVE_INT32 ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Umask must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Umask must be INT32" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::setUmask( mask, err ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::getPathType( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string pathType ;
      string err ;

      // get pathname
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::getPathType( pathname, err, pathType ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( pathType ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::getPermission( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      INT32 permission = 0 ;
      string err ;

      // get pathname
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::getPermission( pathname, err, permission ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto done ;
      }
      rval.getReturnVal().setValue( permission ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::isEmptyDir( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isEmpty = FALSE ;
      string pathname ;
      string err ;

      // get pathname
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Pathname must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::isEmptyDir( pathname, err, isEmpty ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto done ;
      }
      rval.getReturnVal().setValue( isEmpty ? true : false ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::getStat( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string err ;
      BSONObj retObj ;

      // get argument
      rc = arg.getString( 0, pathname ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "Filename must be config" ) ;
         goto error ;
      }
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Filename must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::getStat( pathname, err, retObj ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( retObj ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFile::md5( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string filename ;
      string code ;
      string err ;

      // check, we need 1 argument filename
      if ( 0 == arg.argc() )
      {
         rc = SDB_OUT_OF_BOUND ;
         err = "Filename must be config" ;
         goto error ;
      }
      rc = arg.getString( 0, filename ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         err = "Filename must be string" ;
         goto error ;
      }

      rc = _sptUsrFileCommon::md5( filename, err, code ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      rval.getReturnVal().setValue( code ) ;
   done:
      return rc ;
   error:
      detail = BSON( SPT_ERR << err ) ;
      goto done ;
   }

   INT32 _sptUsrFile::getFileSize( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string filename ;
      INT64 size = 0 ;
      string err ;

      if( 0 == arg.argc() )
      {
         rc = SDB_OUT_OF_BOUND ;
         detail = BSON( SPT_ERR << "Filename must be config" ) ;
         goto error ;
      }

      rc = arg.getString( 0, filename ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Filename must be string" ) ;
         goto error ;
      }

      rc = _sptUsrFileCommon::getFileSize( filename, err, size ) ;
      if( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << err ) ;
         goto error ;
      }
      rval.getReturnVal().setValue( size ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

}
