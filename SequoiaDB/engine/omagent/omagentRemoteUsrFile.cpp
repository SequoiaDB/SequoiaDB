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

   Source File Name = omagentRemoteUsrFile.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#include "omagentRemoteUsrFile.hpp"
#include "omagentMgr.hpp"
#include "omagentDef.hpp"
#include "omagentSession.hpp"
#include "ossCmdRunner.hpp"
#include "ossPrimitiveFileOp.hpp"
#include "sptUsrFileCommon.hpp"
#include <boost/algorithm/string.hpp>
#include "../bson/lib/md5.hpp"
#if defined(_LINUX)
#include <sys/stat.h>
#endif
using namespace bson ;
#define SPT_READ_LEN 1024
#define SPT_MD5_READ_LEN 1024

namespace engine
{
   #define OMA_REMOTE_FIELD_NAME_FID            "FID"
   #define OMA_REMOTE_FIELD_NAME_FILENAME       "Filename"
   #define OMA_REMOTE_FIELD_NAME_DIRNAME        "Dirname"
   #define OMA_REMOTE_FIELD_NAME_SIZE           "Size"
   #define OMA_REMOTE_FIELD_NAME_SEEK_SIZE      "SeekSize"
   #define OMA_REMOTE_FIELD_NAME_CONTENT        "Content"
   #define OMA_REMOTE_FIELD_NAME_PATHNAME       "Pathname"
   #define OMA_REMOTE_FIELD_NAME_PERMISSION     "Permission"
   #define OMA_REMOTE_FIELD_NAME_READ_LEN       "ReadLen"
   #define OMA_REMOTE_FIELD_NAME_SRC            "Src"
   #define OMA_REMOTE_FIELD_NAME_DST            "Dst"
   #define OMA_REMOTE_FIELD_NAME_MODE           "Mode"
   #define OMA_REMOTE_FIELD_NAME_GROUPNAME      "Groupname"
   #define OMA_REMOTE_FIELD_NAME_MASK           "Mask"
   #define OMA_REMOTE_FIELD_NAME_IS_EXIST       "IsExist"
   #define OMA_REMOTE_FIELD_NAME_PATH_TYPE      "PathType"
   #define OMA_REMOTE_FIELD_NAME_IS_EMPTY       "IsEmpty"
   #define OMA_REMOTE_FIELD_NAME_MD5            "MD5"
   #define OMA_REMOTE_FIELD_NAME_IS_BINARY      "IsBinary"

   // function to get current thread omagent session
   static omaSession* _getThreadOmaSession()
   {
      ISession *pSession = NULL ;
      omaSession *pAgentSession = NULL ;
      pmdEDUCB *cb = NULL ;

      cb = pmdGetThreadEDUCB() ;
      if( NULL == cb )
      {
         PD_LOG_MSG( PDERROR, "Failed to get thread edu cb" ) ;
         goto error ;
      }
      pSession = cb->getSession() ;
      if( NULL == pSession )
      {
         PD_LOG( PDERROR, "Failed to get session" ) ;
         goto error ;
      }
      if( pSession->sessionType() != SDB_SESSION_OMAGENT )
      {
         PD_LOG_MSG( PDERROR, "Session is not omagent session" ) ;
         goto error ;
      }

      pAgentSession = dynamic_cast< omaSession* >( pSession ) ;
   done:
      return pAgentSession ;
   error:
      pAgentSession = NULL ;
      goto done ;
   }

   /*
      _remoteFileOpen implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileOpen )

   _remoteFileOpen::_remoteFileOpen()
   {
   }

   _remoteFileOpen::~_remoteFileOpen()
   {
   }

   const CHAR* _remoteFileOpen::name()
   {
      return OMA_REMOTE_FILE_OPEN ;
   }

   INT32 _remoteFileOpen::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string filename ;
      _sptUsrFileCommon *fileCommon = NULL ;
      omaSession *pAgentSession = NULL ;
      string err ;
      UINT32 fID = 0 ;

      // get argument
      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_FILENAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Filename must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_FILENAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Filename must be string" ) ;
         goto error ;
      }
      filename = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_FILENAME ) ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      rc = pAgentSession->newFileObj( fID, &fileCommon ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "Failed to new sptUsrFileCommon obj" ) ;
         goto error ;
      }

      rc = fileCommon->open( filename, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         // Need to release file obj if failed to open file
         pAgentSession->releaseFileObj( fID ) ;
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_FID << fID ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileRead implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileRead )

   _remoteFileRead::_remoteFileRead():
      _FID( 0 ), _size( 1024 ), _isBinary( FALSE )
   {
   }

   _remoteFileRead::~_remoteFileRead()
   {
   }

   INT32 _remoteFileRead::init( const CHAR* pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // Check flags to determine if need to return data using binary type
      if( _optionObj.hasField( OMA_REMOTE_FIELD_NAME_IS_BINARY ) )
      {
         BSONElement ele ;
         ele = _optionObj.getField( OMA_REMOTE_FIELD_NAME_IS_BINARY ) ;
         if( Bool == ele.type() )
         {
            _isBinary = _optionObj.getBoolField( OMA_REMOTE_FIELD_NAME_IS_BINARY ) ;
         }
         else
         {
            _isBinary = _optionObj.getIntField( OMA_REMOTE_FIELD_NAME_IS_BINARY ) ;
         }
      }

      if( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FID ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "FID must be config" ) ;
         goto error ;
      }
      if( NumberInt != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FID ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "FID must be numberInt" ) ;
         goto error ;
      }
      _FID = _matchObj.getIntField( OMA_REMOTE_FIELD_NAME_FID ) ;

      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_SIZE ) )
      {
         _size = SPT_READ_LEN ;
      }
      else
      {
         BSONElement element  = _valueObj.getField( OMA_REMOTE_FIELD_NAME_SIZE ) ;
         if( NumberInt != element.type() &&
             NumberLong != element.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size must be number" ) ;
            goto error ;
         }
         else
         {
            _size = element.numberLong() ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileRead::name()
   {
      return OMA_REMOTE_FILE_READ ;
   }

   INT32 _remoteFileRead::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      CHAR *buf = NULL ;
      SINT64 readLen = 0 ;
      omaSession *pAgentSession = NULL ;
      BSONObjBuilder builder ;
      _sptUsrFileCommon *fileCommon = NULL ;
      string err ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      fileCommon = pAgentSession->getFileObjByID( _FID ) ;
      if( NULL == fileCommon )
      {
         rc = SDB_IO ;
         PD_LOG_MSG( PDERROR, "File is not opened" ) ;
         goto error ;
      }

      // read content
      rc = fileCommon->read( BSON( SPT_FILE_COMMON_FIELD_SIZE << _size ),
                             err, &buf, readLen ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      if( _isBinary )
      {
         builder.appendBinData( OMA_REMOTE_FIELD_NAME_CONTENT, readLen,
                                BinDataGeneral, buf ) ;
      }
      else
      {
         builder.append( OMA_REMOTE_FIELD_NAME_CONTENT, buf ) ;
      }

      retObj = builder.obj() ;
   done:
      if ( NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileWrite implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileWrite )

   _remoteFileWrite::_remoteFileWrite():
      _FID( 0 ), _size( 0 ), _content( NULL )
   {
   }

   _remoteFileWrite::~_remoteFileWrite()
   {
   }

   INT32 _remoteFileWrite::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      ele = _valueObj.getField( OMA_REMOTE_FIELD_NAME_CONTENT );

      if( String == ele.type() )
      {
         _content = ele.valuestr() ;
         _size = ossStrlen( _content ) ;
      }
      else if( BinData == ele.type() )
      {
         _content = ele.binData( _size ) ;
      }
      else
      {
         PD_LOG_MSG( PDERROR, "Content must be binary or string" ) ;
      }

      if( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FID ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "FID must be config" ) ;
         goto error ;
      }
      if( NumberInt != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FID ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "FID must be numberInt" ) ;
         goto error ;
      }
      _FID = _matchObj.getIntField( OMA_REMOTE_FIELD_NAME_FID ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileWrite::name()
   {
      return OMA_REMOTE_FILE_WRITE ;
   }

   INT32 _remoteFileWrite::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      omaSession *pAgentSession = NULL ;
      _sptUsrFileCommon *fileCommon = NULL ;
      string err ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      fileCommon = pAgentSession->getFileObjByID( _FID ) ;
      if( NULL == fileCommon )
      {
         rc = SDB_IO ;
         PD_LOG_MSG( PDERROR, "File is not opened" ) ;
         goto error ;
      }

      // write content
      rc = fileCommon->write( _content, _size, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileTruncate implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileTruncate )

   _remoteFileTruncate::_remoteFileTruncate(): _FID( 0 ),
                                               _size( 0 )
   {
   }

   _remoteFileTruncate::~_remoteFileTruncate()
   {
   }

   INT32 _remoteFileTruncate::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get argument, rc: %d", rc ) ;
         goto error ;
      }

      // get FID
      if( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FID ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "FID must be config" ) ;
         goto error ;
      }

      if( NumberInt != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FID ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "FID must be numberInt" ) ;
         goto error ;
      }

      _FID = _matchObj.getIntField( OMA_REMOTE_FIELD_NAME_FID ) ;

      // get Size
      if( _valueObj.hasField( OMA_REMOTE_FIELD_NAME_SIZE ) )
      {
         BSONElement element ;

         element = _valueObj.getField( OMA_REMOTE_FIELD_NAME_SIZE ) ;

         if( NumberInt != element.type() && NumberLong != element.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Size must be number" ) ;
            goto error ;
         }

         _size = element.numberLong() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileTruncate::name()
   {
      return OMA_REMOTE_FILE_TRUNCATE ;
   }

   INT32 _remoteFileTruncate::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      omaSession *pAgentSession = NULL ;
      _sptUsrFileCommon *fileCommon = NULL ;
      string err ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      fileCommon = pAgentSession->getFileObjByID( _FID ) ;
      if( NULL == fileCommon )
      {
         goto done ;
      }

      rc = fileCommon->truncate( _size, err ) ;
      if( rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileSeek implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileSeek )

   _remoteFileSeek::_remoteFileSeek(): _FID( 0 ), _seekSize( 0 )
   {
   }

   _remoteFileSeek::~_remoteFileSeek()
   {
   }

   INT32 _remoteFileSeek::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;
      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // get FID
      if( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FID ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "FID must be config" ) ;
         goto error ;
      }
      if( NumberInt != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FID ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "FID must be numberInt" ) ;
         goto error ;
      }
      _FID = _matchObj.getIntField( OMA_REMOTE_FIELD_NAME_FID ) ;

      // get SeekSize
      if( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_SEEK_SIZE ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "SeekSize must be config" ) ;
         goto error ;
      }

      {
         BSONElement element = _valueObj.getField( OMA_REMOTE_FIELD_NAME_SEEK_SIZE ) ;
         if( NumberInt != element.type() &&
             NumberLong != element.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "SeekSize must be number" ) ;
            goto error ;
         }
         _seekSize = element.numberLong() ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileSeek::name()
   {
      return OMA_REMOTE_FILE_SEEK ;
   }

   INT32 _remoteFileSeek::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      omaSession *pAgentSession = NULL ;
      _sptUsrFileCommon *fileCommon = NULL ;
      string err ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      fileCommon = pAgentSession->getFileObjByID( _FID ) ;
      if( NULL == fileCommon )
      {
         rc = SDB_IO ;
         PD_LOG_MSG( PDERROR, "File is not opened" ) ;
         goto error ;
      }

      rc = fileCommon->seek( _seekSize, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileClose implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileClose )

   _remoteFileClose::_remoteFileClose(): _FID( 0 )
   {
   }

   _remoteFileClose::~_remoteFileClose()
   {
   }

   INT32 _remoteFileClose::init( const CHAR * pInfomation )
   {
      INT32 rc = SDB_OK ;
      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // get FID
      if( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FID ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "FID must be config" ) ;
         goto error ;
      }
      if( NumberInt != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FID ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "FID must be numberInt" ) ;
         goto error ;
      }
      _FID = _matchObj.getIntField( OMA_REMOTE_FIELD_NAME_FID ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileClose::name()
   {
      return OMA_REMOTE_FILE_CLOSE ;
   }

   INT32 _remoteFileClose::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      omaSession *pAgentSession = NULL ;
      _sptUsrFileCommon *fileCommon = NULL ;
      string err ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      fileCommon = pAgentSession->getFileObjByID( _FID ) ;
      if( NULL == fileCommon )
      {
         goto done ;
      }

      rc = fileCommon->close( err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      pAgentSession->releaseFileObj( _FID ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileRemove implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileRemove )

   _remoteFileRemove::_remoteFileRemove()
   {
   }

   _remoteFileRemove::~_remoteFileRemove()
   {
   }

   const CHAR* _remoteFileRemove::name()
   {
      return OMA_REMOTE_FILE_REMOVE ;
   }

   INT32 _remoteFileRemove::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string filepath ;
      string err ;

      // get pathname
      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Filepath must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Filepath must be string" ) ;
         goto error ;
      }
      filepath = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      rc = _sptUsrFileCommon::remove( filepath, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileExist implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileExist )

   _remoteFileExist::_remoteFileExist()
   {
   }

   _remoteFileExist::~_remoteFileExist()
   {
   }

   const CHAR* _remoteFileExist::name()
   {
      return OMA_REMOTE_FILE_ISEXIST ;
   }

   INT32 _remoteFileExist::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string filepath ;
      BOOLEAN fileExist = FALSE ;
      string err ;
      BSONObjBuilder builder ;

      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Filepath must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Filepath must be string" ) ;
         goto error ;
      }
      filepath = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      rc = _sptUsrFileCommon::exist( filepath, err, fileExist ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      builder.appendBool( OMA_REMOTE_FIELD_NAME_IS_EXIST, fileExist ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileCopy implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileCopy )

   _remoteFileCopy::_remoteFileCopy()
   {
   }

   _remoteFileCopy::~_remoteFileCopy()
   {
   }

   const CHAR* _remoteFileCopy::name()
   {
      return OMA_REMOTE_FILE_COPY ;
   }

   INT32 _remoteFileCopy::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;
      string err ;

      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_SRC ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "src is required" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_SRC ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Src must be string" ) ;
         goto error;
      }
      src = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_SRC ) ;

      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_DST) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Dst is required" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_DST ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Dst must be string" ) ;
         goto error;
      }
      dst = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_DST ) ;

      rc = _sptUsrFileCommon::copy( src, dst, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileMove implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileMove )

   _remoteFileMove::_remoteFileMove()
   {
   }

   _remoteFileMove::~_remoteFileMove()
   {
   }

   const CHAR* _remoteFileMove::name()
   {
      return OMA_REMOTE_FILE_MOVE ;
   }

   INT32 _remoteFileMove::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string src ;
      string dst ;
      string err ;

      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_SRC ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Src is required" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_SRC ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Src must be string" ) ;
         goto error ;
      }
      src = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_SRC ) ;

      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_DST ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Dst is required" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_DST ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Dst must be string" ) ;
         goto error ;
      }
      dst = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_DST ) ;

      rc = _sptUsrFileCommon::move( src, dst, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileMkdir implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileMkdir )

   _remoteFileMkdir::_remoteFileMkdir()
   {
   }

   _remoteFileMkdir::~_remoteFileMkdir()
   {
   }

   const CHAR* _remoteFileMkdir::name()
   {
      return OMA_REMOTE_FILE_MKDIR ;
   }

   INT32 _remoteFileMkdir::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string name ;
      string err ;

      // get argument
      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_DIRNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Dirname is required" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_DIRNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Dirname must be string" ) ;
         goto error ;
      }
      name = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_DIRNAME ) ;

      rc = _sptUsrFileCommon::mkdir( name, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileFind implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileFind )

   _remoteFileFind::_remoteFileFind()
   {
   }

   _remoteFileFind::~_remoteFileFind()
   {
   }

   const CHAR* _remoteFileFind::name()
   {
      return OMA_REMOTE_FILE_FIND ;
   }

   INT32 _remoteFileFind::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      // get argument
      if ( TRUE == _optionObj.isEmpty() )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "OptionObj must be config") ;
         goto error ;
      }

      rc = _sptUsrFileCommon::find( _optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileChmod implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileChmod )

   _remoteFileChmod::_remoteFileChmod()
   {
   }

   _remoteFileChmod::~_remoteFileChmod()
   {
   }

   const CHAR* _remoteFileChmod::name()
   {
      return OMA_REMOTE_FILE_CHMOD ;
   }

   INT32 _remoteFileChmod::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      INT32 mode = 0 ;
      string err ;

      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_MODE ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Mode must be config" ) ;
         goto error ;
      }
      if ( NumberInt != _valueObj.getField( OMA_REMOTE_FIELD_NAME_MODE ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Mode must be INT32" ) ;
         goto error ;
      }
      mode = _valueObj.getIntField( OMA_REMOTE_FIELD_NAME_MODE ) ;

      rc = _sptUsrFileCommon::chmod( pathname, mode, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileChown implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileChown )

   _remoteFileChown::_remoteFileChown()
   {
   }

   _remoteFileChown::~_remoteFileChown()
   {
   }

   const CHAR* _remoteFileChown::name()
   {
      return OMA_REMOTE_FILE_CHOWN ;
   }

   INT32 _remoteFileChown::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string err ;

      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      rc = _sptUsrFileCommon::chown( pathname, _valueObj, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileChgrp implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileChgrp )

   _remoteFileChgrp::_remoteFileChgrp()
   {
   }

   _remoteFileChgrp::~_remoteFileChgrp()
   {
   }

   const CHAR* _remoteFileChgrp::name()
   {
      return OMA_REMOTE_FILE_CHGRP ;
   }

   INT32 _remoteFileChgrp::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string groupname ;
      string err ;
      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_GROUPNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Groupname must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_GROUPNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Groupname must be string" ) ;
         goto error ;
      }
      groupname = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_GROUPNAME ) ;

      rc = _sptUsrFileCommon::chgrp( pathname, groupname, _optionObj, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileGetUmask implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetUmask )

   _remoteFileGetUmask::_remoteFileGetUmask()
   {

   }

   _remoteFileGetUmask::~_remoteFileGetUmask()
   {
   }

   const CHAR* _remoteFileGetUmask::name()
   {
      return OMA_REMOTE_FILE_GET_UMASK ;
   }

   INT32 _remoteFileGetUmask::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string outStr ;
      string err ;

      rc = _sptUsrFileCommon::getUmask( err, outStr ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_MASK << outStr.c_str() ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileSetUmask implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileSetUmask )

   _remoteFileSetUmask::_remoteFileSetUmask()
   {
   }

   _remoteFileSetUmask::~_remoteFileSetUmask()
   {
   }

   const CHAR* _remoteFileSetUmask::name()
   {
      return OMA_REMOTE_FILE_SET_UMASK ;
   }

   INT32 _remoteFileSetUmask::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;
      INT32 mask = 0 ;

      // get argument
      if ( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_MASK ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Mask must be config" ) ;
         goto error ;
      }
      if ( NumberInt != _valueObj.getField( OMA_REMOTE_FIELD_NAME_MASK ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Mask must be INT32" ) ;
         goto error ;
      }
      mask = _valueObj.getIntField( OMA_REMOTE_FIELD_NAME_MASK ) ;

      rc = _sptUsrFileCommon::setUmask( mask, err ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileList implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileList )

   _remoteFileList::_remoteFileList()
   {
   }

   _remoteFileList::~_remoteFileList()
   {
   }

   const CHAR* _remoteFileList::name()
   {
      return OMA_REMOTE_FILE_LIST ;
   }

   INT32 _remoteFileList::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string err ;

      rc = _sptUsrFileCommon::list( _optionObj, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileGetPathType implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetPathType )

   _remoteFileGetPathType::_remoteFileGetPathType()
   {
   }

   _remoteFileGetPathType::~_remoteFileGetPathType()
   {
   }

   const CHAR* _remoteFileGetPathType::name()
   {
      return OMA_REMOTE_FILE_GET_PATH_TYPE ;
   }

   INT32 _remoteFileGetPathType::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string pathType ;
      string err ;

      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      rc = _sptUsrFileCommon::getPathType( pathname, err, pathType ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_PATH_TYPE << pathType ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileIsEmptyDir implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileIsEmptyDir )

   _remoteFileIsEmptyDir::_remoteFileIsEmptyDir()
   {
   }

   _remoteFileIsEmptyDir::~_remoteFileIsEmptyDir()
   {
   }

   const CHAR* _remoteFileIsEmptyDir::name()
   {
      return OMA_REMOTE_FILE_IS_EMPTYDIR ;
   }

   INT32 _remoteFileIsEmptyDir::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isEmpty = FALSE ;
      string pathname ;
      string err ;

      // get pathname
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Pathname must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Pathname must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      rc = _sptUsrFileCommon::isEmptyDir( pathname, err, isEmpty ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_IS_EMPTY << isEmpty ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileStat implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileStat )

   _remoteFileStat::_remoteFileStat()
   {
   }

   _remoteFileStat::~_remoteFileStat()
   {
   }

   const CHAR* _remoteFileStat::name()
   {
      return OMA_REMOTE_FILE_STAT ;
   }

   INT32 _remoteFileStat::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string err ;

      // get argument
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FILENAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FILENAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Filename must be string" ) ;
         goto error ;
      }
      pathname = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_FILENAME ) ;

      rc = _sptUsrFileCommon::getStat( pathname, err, retObj ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileMd5 implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileMd5 )

   _remoteFileMd5::_remoteFileMd5()
   {
   }

   _remoteFileMd5::~_remoteFileMd5()
   {
   }

   const CHAR* _remoteFileMd5::name()
   {
      return OMA_REMOTE_FILE_MD5 ;
   }

   INT32 _remoteFileMd5::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string filename ;
      string err ;
      string code ;

      // check, we need 1 argument filename
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FILENAME ) )
      {
         rc = SDB_INVALIDARG  ;
         err = "Filename must be config" ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FILENAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         err = "filename must be string" ;
         goto error ;
      }
      filename = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_FILENAME ) ;

      rc = _sptUsrFileCommon::md5( filename, err, code ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_MD5 << code.c_str() ) ;
   done:
      return rc ;
   error:
      PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
      goto done ;
   }


   /*
      _remoteFileGetSize implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetSize )

   _remoteFileGetSize::_remoteFileGetSize()
   {
   }

   _remoteFileGetSize::~_remoteFileGetSize()
   {
   }

   const CHAR* _remoteFileGetSize::name()
   {
      return OMA_REMOTE_FILE_GET_CONTENT_SIZE ;
   }

   INT32 _remoteFileGetSize::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      INT64 size = 0 ;
      string name ;
      string err ;

      // get filename
      if ( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FILENAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Filename must be config" ) ;
         goto error ;
      }
      if ( String != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FILENAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Filename must be string" ) ;
         goto error ;
      }
      name = _matchObj.getStringField( OMA_REMOTE_FIELD_NAME_FILENAME ) ;

      rc = _sptUsrFileCommon::getFileSize( name, err, size ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_SIZE << size ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileGetPermission implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileGetPermission )

   _remoteFileGetPermission::_remoteFileGetPermission()
   {
   }

   _remoteFileGetPermission::~_remoteFileGetPermission()
   {
   }

   const CHAR* _remoteFileGetPermission::name()
   {
      return OMA_REMOTE_FILE_GET_PERMISSION ;
   }

   INT32 _remoteFileGetPermission::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string pathname ;
      string err ;
      INT32 permission = 0 ;

      // get pathname
      if( FALSE == _valueObj.hasField( OMA_REMOTE_FIELD_NAME_PATHNAME ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "Pathname must be config" ) ;
         goto error ;
      }
      else if( String != _valueObj.getField( OMA_REMOTE_FIELD_NAME_PATHNAME ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Pathname must be string" ) ;
         goto error ;
      }
      pathname = _valueObj.getStringField( OMA_REMOTE_FIELD_NAME_PATHNAME ) ;

      rc = _sptUsrFileCommon::getPermission( pathname, err, permission ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      retObj = BSON( OMA_REMOTE_FIELD_NAME_PERMISSION << permission ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteFileReadLine implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteFileReadLine )

   _remoteFileReadLine::_remoteFileReadLine()
   {
   }

   _remoteFileReadLine::~_remoteFileReadLine()
   {
   }

   INT32 _remoteFileReadLine::init( const CHAR* pInfomation )
   {
      INT32 rc = SDB_OK ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      if( FALSE == _matchObj.hasField( OMA_REMOTE_FIELD_NAME_FID ) )
      {
         rc = SDB_OUT_OF_BOUND ;
         PD_LOG_MSG( PDERROR, "FID must be config" ) ;
         goto error ;
      }
      if( NumberInt != _matchObj.getField( OMA_REMOTE_FIELD_NAME_FID ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "FID must be numberInt" ) ;
         goto error ;
      }
      _FID = _matchObj.getIntField( OMA_REMOTE_FIELD_NAME_FID ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _remoteFileReadLine::name()
   {
      return OMA_REMOTE_FILE_READ_LINE ;
   }

   INT32 _remoteFileReadLine::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      CHAR *buf = NULL ;
      SINT64 readLen = 0 ;
      omaSession *pAgentSession = NULL ;
      BSONObjBuilder builder ;
      _sptUsrFileCommon *fileCommon = NULL ;
      string err ;

      pAgentSession = _getThreadOmaSession() ;
      if( NULL == pAgentSession )
      {
         rc = SDB_SYS ;
         PD_LOG_MSG( PDERROR, "Failed to get omagent session" ) ;
         goto error ;
      }

      fileCommon = pAgentSession->getFileObjByID( _FID ) ;
      if( NULL == fileCommon )
      {
         rc = SDB_IO ;
         PD_LOG_MSG( PDERROR, "File is not opened" ) ;
         goto error ;
      }

      // read content
      rc = fileCommon->readLine( err, &buf, readLen ) ;
      if( SDB_OK != rc )
      {
         PD_LOG_MSG( PDERROR, "%s", err.c_str() ) ;
         goto error ;
      }
      builder.append( OMA_REMOTE_FIELD_NAME_CONTENT, buf, readLen + 1 ) ;
      builder.append( OMA_REMOTE_FIELD_NAME_READ_LEN, readLen) ;

      retObj = builder.obj() ;
   done:
      if ( NULL != buf )
      {
         SDB_OSS_FREE( buf ) ;
      }
      return rc ;
   error:
      goto done ;
   }

}
