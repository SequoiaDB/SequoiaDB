/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sptUsrFileCommon.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/24/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossFile.hpp"
#include "ossCmdRunner.hpp"
#include "ossIO.hpp"
#include "ossPath.hpp"
#include "sptUsrFileCommon.hpp"
#include <boost/algorithm/string.hpp>
#include "../bson/lib/md5.hpp"
#include <string>
#if defined(_LINUX)
#include <sys/stat.h>
#include <unistd.h>
#endif

#define SPT_MD5_READ_LEN 1024
#define SPT_READ_LEN     1024
#define SPT_BUFFER_INIT_SIZE 1024
using namespace std ;
using namespace bson ;

namespace engine
{
   _sptUsrFileCommon::_sptUsrFileCommon()
   {
   }

   _sptUsrFileCommon::~_sptUsrFileCommon()
   {
      if( _file.isOpened() )
      {
         ossClose( _file ) ;
      }
   }

   INT32 _sptUsrFileCommon::open( const string &filename, BSONObj optionObj,
                                 string &err )
   {
      INT32 rc = SDB_OK ;
      UINT32 permission = OSS_RWXU ;
      UINT32 iMode = OSS_READWRITE | OSS_CREATE ;

      _filename = filename ;

      {
         SDB_OSS_FILETYPE type ;
         rc = ossGetPathType( filename.c_str(), &type ) ;
         if ( SDB_OK == rc && SDB_OSS_DIR == type )
         {
            rc = SDB_INVALIDARG ;
            err = "Filename must not be dir" ;
            goto error ;
         }
      }
      if ( optionObj.hasField( SPT_FILE_COMMON_FIELD_PERMISSION ) )
      {
         INT32 mode = 0 ;
         if( NumberInt !=
             optionObj.getField( SPT_FILE_COMMON_FIELD_PERMISSION ).type() )
         {
            err = "Permission must be INT32" ;
            goto error ;
         }
         mode = optionObj.getIntField( SPT_FILE_COMMON_FIELD_PERMISSION ) ;

         permission = 0 ;

         if ( mode & 0x0001 )
         {
            permission |= OSS_XO ;
         }
         if ( mode & 0x0002 )
         {
            permission |= OSS_WO ;
         }
         if ( mode & 0x0004 )
         {
            permission |= OSS_RO ;
         }
         if ( mode & 0x0008 )
         {
            permission |= OSS_XG ;
         }
         if ( mode & 0x0010 )
         {
            permission |= OSS_WG ;
         }
         if ( mode & 0x0020 )
         {
            permission |= OSS_RG ;
         }
         if ( mode & 0x0040 )
         {
            permission |= OSS_XU ;
         }
         if ( mode & 0x0080 )
         {
            permission |= OSS_WU ;
         }
         if ( mode & 0x0100 )
         {
            permission |= OSS_RU ;
         }
      }

      if( optionObj.hasField( "mode" ) )
      {
         if( NumberInt != optionObj.getField( "mode" ).type() )
         {
            err = "mode must be int" ;
            goto error ;
         }
         iMode = optionObj.getIntField( "mode" ) ;
      }

      rc = ossOpen( _filename.c_str(),
                    iMode,
                    permission,
                    _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc:%d",
                 _filename.c_str(), rc ) ;
         err = "Failed to open file: " + _filename ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::read( const bson::BSONObj &optionObj, string &err,
                                  CHAR** buf, SINT64 &readLen )
   {
      INT32 rc = SDB_OK ;
      SINT64 size = SPT_READ_LEN ;

      SDB_ASSERT( NULL == (*buf), "*buf must be null" ) ;

      if( optionObj.hasField( SPT_FILE_COMMON_FIELD_SIZE ) )
      {
         BSONElement element = optionObj.getField( SPT_FILE_COMMON_FIELD_SIZE ) ;
         if( NumberInt != element.type() &&
             NumberLong != element.type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Size must be number" ;
            goto error ;
         }
         size = element.numberLong() ;
         if( size < 0 )
         {
            rc = SDB_INVALIDARG ;
            err = "Size must be zero or positive number" ;
            goto error ;
         }
      }

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "The file is not opened." ) ;
         err = "File is not opened" ;
         rc = SDB_IO ;
         goto error ;
      }

      *buf = ( CHAR* )SDB_OSS_MALLOC( size + 1 ) ;
      if ( NULL == *buf )
      {
         PD_LOG( PDERROR, "Failed to allocate mem." ) ;
         err = "Failed to allocate mem" ;
         rc = SDB_OOM ;
         goto error ;
      }

      rc = ossReadN( &_file, size, *buf, readLen ) ;
      if ( SDB_OK != rc )
      {
         err = getErrDesp( rc );
         PD_LOG( PDERROR, "Failed to read file:%d", rc ) ;
         goto error ;
      }
      (*buf)[ readLen ] = '\0' ;
   done:
      return rc ;
   error:
      if( *buf != NULL )
      {
         SDB_OSS_FREE( *buf ) ;
         *buf = NULL ;
      }
      goto done ;
   }

   INT32 _sptUsrFileCommon::readLine( std::string &err, CHAR** buf, SINT64 &readLen )
   {
      INT32 rc = SDB_OK ;
      SINT64 size = SPT_BUFFER_INIT_SIZE ;
      SINT64 totalRead = 0 ;

      SDB_ASSERT( NULL == (*buf), "*buf must be null" ) ;

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "The file is not opened." ) ;
         err = "File is not opened" ;
         rc = SDB_IO ;
         goto error ;
      }

      *buf = ( CHAR* )SDB_OSS_MALLOC( size + 1 ) ;
      {
         CHAR readChar ;
         SINT64 readSize ;
         rc = ossReadN( &_file, 1, &readChar, readSize ) ;
         while( SDB_OK == rc && readSize )
         {
            if( size <= totalRead )
            {
               *buf = ( CHAR* )SDB_OSS_REALLOC( *buf, size*2 + 1 ) ;
               size *= 2 ;
            }
            (*buf)[ totalRead ] = readChar ;
            totalRead++ ;
            if( readChar == '\n' )
            {
               break ;
            }
            rc = ossReadN( &_file, 1, &readChar, readSize ) ;
         }
         if( totalRead == 0 ||
             ( SDB_OK != rc && SDB_EOF != rc ) )
         {
            PD_LOG( PDERROR, "Failed to read file, %d", rc ) ;
            goto error ;
         }

         rc = SDB_OK ;
         (*buf)[ totalRead ] = '\0' ;
         readLen = totalRead ;
      }
   done:
      return rc ;
   error:
      if( *buf != NULL )
      {
         SDB_OSS_FREE( *buf ) ;
         *buf = NULL ;
      }
      goto done ;
   }

   INT32 _sptUsrFileCommon::write( const CHAR* buf, SINT64 size, string &err )
   {
      INT32 rc = SDB_OK ;

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "The file is not opened." ) ;
         err = "File is not opened" ;
         rc = SDB_IO ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get content, rc: %d", rc ) ;

      if( NULL != buf )
      {
         rc = ossWriteN( &_file, buf, size ) ;
         if ( SDB_OK != rc )
         {
            err = getErrDesp( rc ) ;
            PD_LOG( PDERROR, "Failed to write to file:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::seek( INT64 seekSize, const BSONObj &optionObj,
                                  string &err )
   {
      INT32 rc = SDB_OK ;
      OSS_SEEK whence ;
      string whenceStr = "b" ;

      if ( !_file.isOpened() )
      {
         PD_LOG( PDERROR, "The file is not opened." ) ;
         err = "File is not opened" ;
         rc = SDB_IO ;
         goto error ;
      }

      if( optionObj.hasField( SPT_FILE_COMMON_FIELD_WHERE ) )
      {
         if( String != optionObj.getField( SPT_FILE_COMMON_FIELD_WHERE ).type() )
         {
            err = "Where must be string(b/c/e)" ;
            rc = SDB_INVALIDARG ;
         }
         whenceStr = optionObj.getStringField( SPT_FILE_COMMON_FIELD_WHERE ) ;
      }

      if ( "b" == whenceStr )
      {
         whence = OSS_SEEK_SET ;
      }
      else if ( "c" == whenceStr )
      {
         whence = OSS_SEEK_CUR ;
      }
      else if ( "e" == whenceStr )
      {
         whence = OSS_SEEK_END ;
      }
      else
      {
         err = "Where must be (b/c/e)" ;
         PD_LOG( PDERROR, "Invalid arg whence:%s", whenceStr.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = ossSeek( &_file, seekSize, whence ) ;
      if ( SDB_OK != rc )
      {
         err = getErrDesp( rc ) ;
         PD_LOG( PDERROR, "Failed to seek:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::close( string &err )
   {
      INT32 rc = SDB_OK ;
      if ( _file.isOpened() )
      {
         rc = ossClose( _file ) ;
         if ( SDB_OK != rc )
         {
            err = "Failed to close file" ;
            PD_LOG( PDERROR, "failed to close file:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::remove( const string &path, string &err )
   {
      INT32 rc = SDB_OK ;

      rc = ossDelete( path.c_str() ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to remove file:" + path ;
         PD_LOG( PDERROR, "Failed to remove file:%s, rc:%d",
                 path.c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::exist( const string &path, string &err,
                                   BOOLEAN &isExist )
   {
      INT32 rc = SDB_OK ;

      isExist = FALSE ;
      rc = ossAccess( path.c_str() ) ;
      if ( SDB_OK != rc && SDB_FNE != rc )
      {
         err = "Access file failed" ;
         goto error ;
      }
      else if ( SDB_OK == rc )
      {
         isExist = TRUE ;
      }
      rc = SDB_OK ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::copy( const string &src, const string &dst,
                                 const BSONObj &optionObj, string &err )
   {
      INT32 rc = SDB_OK ;
      UINT32 permission = OSS_DEFAULTFILE ;
      BOOLEAN isReplace = TRUE ;

      if ( optionObj.hasField( SPT_FILE_COMMON_FIELD_IS_REPLACE ) )
      {
         if( Bool != optionObj.getField( SPT_FILE_COMMON_FIELD_IS_REPLACE ).type() )
         {
            err = "IsReplace must be bool" ;
            goto error ;
         }
         isReplace = optionObj.getBoolField( SPT_FILE_COMMON_FIELD_IS_REPLACE ) ;
      }

      if ( optionObj.hasField( SPT_FILE_COMMON_FIELD_MODE ) )
      {
         INT32 mode = 0 ;
         if( NumberInt != optionObj.getField( SPT_FILE_COMMON_FIELD_MODE ).type() )
         {
            err = "Mode must be NumberInt" ;
            goto error ;
         }
         mode = optionObj.getIntField( SPT_FILE_COMMON_FIELD_MODE ) ;
         permission = 0 ;
         if ( mode & 0x0001 )
         {
            permission |= OSS_XO ;
         }
         if ( mode & 0x0002 )
         {
            permission |= OSS_WO ;
         }
         if ( mode & 0x0004 )
         {
            permission |= OSS_RO ;
         }
         if ( mode & 0x0008 )
         {
            permission |= OSS_XG ;
         }
         if ( mode & 0x0010 )
         {
            permission |= OSS_WG ;
         }
         if ( mode & 0x0020 )
         {
            permission |= OSS_RG ;
         }
         if ( mode & 0x0040 )
         {
            permission |= OSS_XU ;
         }
         if ( mode & 0x0080 )
         {
            permission |= OSS_WU ;
         }
         if ( mode & 0x0100 )
         {
            permission |= OSS_RU ;
         }
      }
#if defined (_LINUX)
      else
      {
         struct stat fileStat ;
         mode_t fileMode ;
         permission = 0 ;
         if ( stat( src.c_str(), &fileStat ) )
         {
            err = "Failed to get src file stat" ;
            rc = SDB_SYS ;
         }
         fileMode = fileStat.st_mode ;
         if ( fileMode & S_IRUSR )
         {
            permission |= OSS_RU ;
         }
         if ( fileMode & S_IWUSR )
         {
            permission |= OSS_WU ;
         }
         if ( fileMode & S_IXUSR )
         {
            permission |= OSS_XU ;
         }
         if ( fileMode & S_IRGRP )
         {
            permission |= OSS_RG ;
         }
         if ( fileMode & S_IWGRP )
         {
            permission |= OSS_WG ;
         }
         if ( fileMode & S_IXGRP )
         {
            permission |= OSS_XG ;
         }
         if ( fileMode & S_IROTH )
         {
            permission |= OSS_RO ;
         }
         if ( fileMode & S_IWOTH )
         {
            permission |= OSS_WO ;
         }
         if ( fileMode & S_IXOTH )
         {
            permission |= OSS_XO ;
         }
      }
#endif

      rc = ossFileCopy( src.c_str(), dst.c_str(), permission, isReplace ) ;
      if ( rc )
      {
         err = "Copy file failed" ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::mkdir( const string &name, const BSONObj &optionObj,
                                  string &err )
   {
      INT32 rc = SDB_OK ;
      UINT32 permission = OSS_DEFAULTDIR ;

      if ( optionObj.hasField( SPT_FILE_COMMON_FIELD_MODE ) )
      {
         INT32 mode = 0 ;
         if( NumberInt != optionObj.getField( SPT_FILE_COMMON_FIELD_MODE ).type() )
         {
            err = "Mode must be NumberInt" ;
            goto error ;
         }
         mode = optionObj.getIntField( SPT_FILE_COMMON_FIELD_MODE ) ;

         permission = 0 ;
         if ( mode & 0x0001 )
         {
            permission |= OSS_XO ;
         }
         if ( mode & 0x0002 )
         {
            permission |= OSS_WO ;
         }
         if ( mode & 0x0004 )
         {
            permission |= OSS_RO ;
         }
         if ( mode & 0x0008 )
         {
            permission |= OSS_XG ;
         }
         if ( mode & 0x0010 )
         {
            permission |= OSS_WG ;
         }
         if ( mode & 0x0020 )
         {
            permission |= OSS_RG ;
         }
         if ( mode & 0x0040 )
         {
            permission |= OSS_XU ;
         }
         if ( mode & 0x0080 )
         {
            permission |= OSS_WU ;
         }
         if ( mode & 0x0100 )
         {
            permission |= OSS_RU ;
         }
      }

      rc = ossMkdir( name.c_str(), permission ) ;
      if ( SDB_FE == rc )
      {
         rc = SDB_OK ;
      }
      else if ( rc )
      {
         err = "Create dir failed" ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::move( const string &src, const string &dst, string &err )
   {
      INT32 rc = SDB_OK ;

      rc = ossRenamePath( src.c_str(), dst.c_str() ) ;
      if ( rc )
      {
         err = "Rename path failed" ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::find( const BSONObj &optionObj, string &err,
                                 BSONObj &retObj )
   {
      INT32 rc           = SDB_OK ;
      string findType    = "n" ;
      UINT32 exitCode    = 0 ;
      string             mode ;
      string             rootDir ;
      _ossCmdRunner      runner ;
      string             value ;
      string             pathname ;
      string             outStr ;
      BSONObjBuilder     builder ;
      stringstream       cmd ;

      if ( TRUE == optionObj.hasField( SPT_FILE_COMMON_FIELD_VALUE ) )
      {
         if ( String != optionObj.getField( SPT_FILE_COMMON_FIELD_VALUE ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Value must be string" ;
            goto error ;
         }
         value = optionObj.getStringField( SPT_FILE_COMMON_FIELD_VALUE ) ;
      }

      if ( TRUE == optionObj.hasField( SPT_FILE_COMMON_FIELD_MODE ) )
      {
         if ( String != optionObj.getField( SPT_FILE_COMMON_FIELD_MODE ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Mode must be string" ;
            goto error ;
         }
         findType = optionObj.getStringField( SPT_FILE_COMMON_FIELD_MODE ) ;
      }

      /* get the way to find file:
         -name:   filename
         -user:   user uname
         -group:  group gname
         -perm:   permission
      */
      if ( "n" == findType )
      {
         if ( string::npos != value.find( "/", 0 ) )
         {
            rc = SDB_INVALIDARG ;
            err = "Value shouldn't contain '/'" ;
            goto error ;
         }
         mode = " -name" ;
      }
      else if ( "u" == findType )
      {
         mode = " -user" ;
      }
      else if ( "p" == findType )
      {
         mode = " -perm" ;
      }
      else if ( "g" == findType )
      {
         mode = " -group" ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         err = "Mode must be required type" ;
         goto error ;
      }

      if ( TRUE == optionObj.hasField( SPT_FILE_COMMON_FIELD_PATHNAME ) )
      {
         if ( String != optionObj.getField( SPT_FILE_COMMON_FIELD_PATHNAME ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Pathname must be string" ;
            goto error ;
         }
         pathname = optionObj.getStringField( SPT_FILE_COMMON_FIELD_PATHNAME ) ;

      }

#if defined (_LINUX)
      cmd << "find" ;

      if ( FALSE == pathname.empty() )
      {
         cmd << " " << pathname ;
      }

      if ( FALSE == value.empty() )
      {
         cmd << mode << " " << value ;
      }
#elif defined (_WINDOWS)
      if ( " -name" != mode )
      {
         goto done ;
      }

      if ( !pathname.empty() &&
           '\\' != pathname[ pathname.size() - 1 ] )
      {
         pathname += "\\" ;
      }
      cmd << "cmd /C dir /b /s "<< pathname << value ;
#endif
      rc = runner.exec( cmd.str().c_str(), exitCode,
                     FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd.str() << ",rc: "
            << rc
            << ",exit: "
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "Failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         if ( '\n' == outStr[ outStr.size() - 1 ] )
         {
            outStr.erase( outStr.size()-1, 1 ) ;
         }

         rc = exitCode ;
         err = outStr ;
         goto error ;
      }

      rc = _extractFindInfo( outStr.c_str(), builder ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract find info" ;
         goto error ;
      }

      retObj = builder.obj() ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::list( const BSONObj &optionObj, string &err,
                                 BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      UINT32 exitCode    = 0 ;
      BOOLEAN showDetail = FALSE ;

      BSONObjBuilder     builder ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;

#if defined (_LINUX)
      cmd << "ls -A -l" ;
#elif defined (_WINDOWS)
      cmd << "cmd /C dir /-C /A" ;
#endif

      showDetail = optionObj.getBoolField( SPT_FILE_COMMON_FIELD_DETAIL ) ;
      if ( TRUE == optionObj.hasField( SPT_FILE_COMMON_FIELD_PATHNAME ) )
      {
         if ( String != optionObj.getField( SPT_FILE_COMMON_FIELD_PATHNAME ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Pathname must be string" ;
         }
         cmd << " " << optionObj.getStringField( SPT_FILE_COMMON_FIELD_PATHNAME ) ;
      }

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "Failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }

      rc = _extractListInfo( outStr.c_str(), builder, showDetail ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to extract list info" ;
         goto error ;
      }

      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::chmod( string &pathname, INT32 mode,
                                  BSONObj optionObj, string &err )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      string outStr ;
      BOOLEAN isRecursive = FALSE ;

      isRecursive = optionObj.getBoolField( SPT_FILE_COMMON_FIELD_IS_RECURSIVE ) ;

      mode = mode & 0xfff ;
      cmd << "chmod" ;
      if ( TRUE == isRecursive )
      {
         cmd << " -R" ;
      }
      cmd << " " << oct << mode << " " << pathname ;

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFileCommon::chown( string &pathname, BSONObj ownerObj,
                                  BSONObj optionObj, string &err )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32  exitCode    = 0 ;
      string username = "" ;
      string groupname = "" ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      string outStr ;
      BOOLEAN isRecursive = FALSE ;

      isRecursive = optionObj.getBoolField( SPT_FILE_COMMON_FIELD_IS_RECURSIVE ) ;

      if ( FALSE == ownerObj.hasField( SPT_FILE_COMMON_FIELD_USERNAME ) &&
           FALSE == ownerObj.hasField( SPT_FILE_COMMON_FIELD_GROUPNAME ) )
      {
         rc = SDB_INVALIDARG ;
         err = "Username or groupname must be config" ;
         goto error ;
      }

      if ( TRUE == ownerObj.hasField( SPT_FILE_COMMON_FIELD_USERNAME ) )
      {
         if ( String != ownerObj.getField( SPT_FILE_COMMON_FIELD_USERNAME ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Username must be string" ;
            goto error ;
         }
         username = ownerObj.getStringField( SPT_FILE_COMMON_FIELD_USERNAME ) ;
      }

      if ( TRUE == ownerObj.hasField( SPT_FILE_COMMON_FIELD_GROUPNAME ) )
      {
         if ( String != ownerObj.getField( SPT_FILE_COMMON_FIELD_GROUPNAME ).type() )
         {
            rc = SDB_INVALIDARG ;
            err = "Groupname must be string" ;
            goto error ;
         }
         groupname = ownerObj.getStringField( SPT_FILE_COMMON_FIELD_GROUPNAME ) ;
      }

      cmd << "chown" ;
      if ( TRUE == isRecursive )
      {
         cmd << " -R" ;
      }
      cmd << " " << username << ":" << groupname << " " << pathname ;

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFileCommon::chgrp( string &pathname, string groupname,
                                  BSONObj optionObj, string &err )
   {
#if defined (_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      stringstream cmd ;
      _ossCmdRunner runner ;
      string outStr ;
      BOOLEAN isRecursive = FALSE ;

      isRecursive = optionObj.getBoolField( SPT_FILE_COMMON_FIELD_IS_RECURSIVE ) ;

      cmd << "chgrp" ;
      if ( TRUE == isRecursive )
      {
         cmd << " -R" ;
      }

      cmd << " " << groupname << " " << pathname ;

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to read result" ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFileCommon::getUmask( string &err, string &retStr )
   {
#if defined(_LINUX)
      INT32 rc = SDB_OK ;
      UINT32 exitCode = 0 ;
      string cmd = "umask" ;
      _ossCmdRunner runner ;

      rc = runner.exec( cmd.c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( retStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "Failed to read msg from cmd \"" << cmd << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }

      if( '\n' == retStr[ retStr.size() - 1 ] )
      {
         retStr.erase( retStr.size()-1, 1 ) ;
      }

   done:
      return rc ;
   error:
      goto done ;

#elif defined (_WINDOWS)
      retStr = "" ;
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFileCommon::setUmask( INT32 mask, string &err )
   {
#if defined(_LINUX)
      INT32              userMask = 0 ;

      userMask = 0 ;
      if ( mask & 0x0001 )
      {
         userMask |= S_IXOTH ;
      }
      if ( mask & 0x0002 )
      {
         userMask |= S_IWOTH ;
      }
      if ( mask & 0x0004 )
      {
         userMask |= S_IROTH ;
      }
      if ( mask & 0x0008 )
      {
         userMask |= S_IXGRP ;
      }
      if ( mask & 0x0010 )
      {
         userMask |= S_IWGRP ;
      }
      if ( mask & 0x0020 )
      {
         userMask |= S_IRGRP ;
      }
      if ( mask & 0x0040 )
      {
         userMask |= S_IXUSR ;
      }
      if ( mask & 0x0080 )
      {
         userMask |= S_IWUSR ;
      }
      if ( mask & 0x0100 )
      {
         userMask |= S_IRUSR ;
      }

      umask( userMask ) ;
      return SDB_OK ;

#elif defined (_WINDOWS)
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFileCommon::getPathType( const string &pathname,
                                        string &err,
                                        string &pathType )
   {
      INT32 rc = SDB_OK ;
      CHAR   realPath[ OSS_MAX_PATHSIZE + 1] = { '\0' } ;
      SDB_OSS_FILETYPE type ;

      if ( NULL == ossGetRealPath( pathname.c_str(),
                                   realPath,
                                   OSS_MAX_PATHSIZE + 1 ) )
      {
         rc = SDB_SYS ;
         err = "Failed to build real path" ;
         goto error ;
      }

      rc = ossGetPathType( realPath, &type ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to get path type" ;
         goto error ;
      }

      switch( type )
      {
         case SDB_OSS_FIL:
            pathType = "FIL" ;
            break ;
         case SDB_OSS_DIR:
            pathType = "DIR" ;
            break ;
         case SDB_OSS_SYM:
            pathType = "SYM" ;
            break ;
         case SDB_OSS_DEV:
            pathType = "DEV" ;
            break ;
         case SDB_OSS_PIP:
            pathType = "PIP" ;
            break ;
         case SDB_OSS_SCK:
            pathType = "SCK" ;
            break ;
         default:
            pathType = "UNK" ;
            break ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFileCommon::getPermission( const string &pathname,
                                          string &err,
                                          INT32 &permission )
   {
#if defined( _LINUX )
      INT32 rc = SDB_OK ;
      struct stat fileStat ;
      mode_t fileMode ;

      permission = 0 ;
      if ( stat( pathname.c_str(), &fileStat ) )
      {
         rc = SDB_SYS ;
         err = "Failed to get src file stat" ;
         goto error ;
      }
      fileMode = fileStat.st_mode ;
      if ( fileMode & S_IRUSR )
      {
         permission |= OSS_RU ;
      }
      if ( fileMode & S_IWUSR )
      {
         permission |= OSS_WU ;
      }
      if ( fileMode & S_IXUSR )
      {
         permission |= OSS_XU ;
      }
      if ( fileMode & S_IRGRP )
      {
         permission |= OSS_RG ;
      }
      if ( fileMode & S_IWGRP )
      {
         permission |= OSS_WG ;
      }
      if ( fileMode & S_IXGRP )
      {
         permission |= OSS_XG ;
      }
      if ( fileMode & S_IROTH )
      {
         permission |= OSS_RO ;
      }
      if ( fileMode & S_IWOTH )
      {
         permission |= OSS_WO ;
      }
      if ( fileMode & S_IXOTH )
      {
         permission |= OSS_XO ;
      }

   done:
      return rc ;
   error:
      goto done ;
#elif defined( _WINDOWS )
      return SDB_OK ;
#endif
   }

   INT32 _sptUsrFileCommon::isEmptyDir( const std::string &pathname,
                                        std::string &err, BOOLEAN &isEmpty )
   {
      INT32 rc = SDB_OK ;
      multimap< string, string > mapFiles ;
      SDB_OSS_FILETYPE type ;

      rc = ossAccess( pathname.c_str() ) ;
      if ( SDB_OK != rc )
      {
         err = "Pathname not exist" ;
         goto error ;
      }

      rc = ossGetPathType( pathname.c_str(), &type ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to get path type" ;
         goto error ;
      }
      if ( SDB_OSS_DIR != type )
      {
         rc = SDB_INVALIDARG ;
         err = "Pathname must be dir" ;
         goto error ;
      }

      rc = ossEnumFiles( pathname, mapFiles, NULL, 1 ) ;
      if( SDB_OK != rc )
      {
         err = "Failed to enum dir" ;
         goto error ;
      }

      isEmpty = FALSE ;
      if( mapFiles.empty() )
      {
         isEmpty = TRUE ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   INT32 _sptUsrFileCommon::getStat( const string &pathname, string &err,
                                    BSONObj &retObj )
   {
      INT32              rc = SDB_OK ;
      UINT32             exitCode = 0 ;
      stringstream       cmd ;
      _ossCmdRunner      runner ;
      string             outStr ;
      vector<string>     splited ;
      BSONObjBuilder     builder ;
      string             fileType ;

      cmd << "stat -c\"%n|%s|%U|%G|%x|%y|%z|%A\" " << pathname ;

      rc = runner.exec( cmd.str().c_str(), exitCode,
                        FALSE, -1, FALSE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to exec cmd, rc:%d, exit:%d",
                 rc, exitCode ) ;
         stringstream ss ;
         ss << "Failed to exec cmd " << cmd.str() << ",rc:"
            << rc
            << ",exit:"
            << exitCode ;
         err = ss.str() ;
         goto error ;
      }

      rc = runner.read( outStr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read msg from cmd runner:%d", rc ) ;
         stringstream ss ;
         ss << "Failed to read msg from cmd \"" << cmd.str() << "\", rc:"
            << rc ;
         err = ss.str() ;
         goto error ;
      }
      else if ( SDB_OK != exitCode )
      {
         rc = exitCode ;
         err = outStr ;
         goto error ;
      }

      /* extract result
      format: xxx|xxx|xxx|xxx|xxx|xxx|xxx|xxx
      explain: separate by '|'
      col[0]: filename   e.g: /home/users/wjm/trunk/bin
      col[1]: size       e.g: 4096
      col[2]: usre name  e.g: root
      col[3]: group name  e.g: root
      col[4]: time of last access  e.g:2016-10-11 15:27:48.839198876 +0800
      col[5]: time of last modification  e.g:2016-10-11 15:27:48.839198876 +0800
      col[6]: time of last change e.g:2016-10-11 15:27:48.839198876 +0800
      col[7]: access rights in human readable form   e.g: drwxrwxrwx
      */
      try
      {
         boost::algorithm::split( splited, outStr, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         err = "Failed to split result" ;
         PD_LOG( PDERROR, "Failed to split result" ) ;
         goto error ;
      }
      for( vector<string>::iterator itr = splited.begin();
           itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      for( vector<string>::iterator itr = splited.begin();
           itr != splited.end(); itr++ )
      {
         vector<string> token ;
         try
         {
            boost::algorithm::split( token, *itr, boost::is_any_of( "|" ) ) ;
         }
         catch( std::exception )
         {
            rc = SDB_SYS ;
            err = "Failed to split result" ;
            PD_LOG( PDERROR, "Failed to split result" ) ;
            goto error ;
         }
         for( vector<string>::iterator itr = token.begin();
              itr != token.end(); )
         {
            if ( itr->empty() )
            {
               itr = token.erase( itr ) ;
            }
            else
            {
               itr++ ;
            }
         }
         if( 8 != token.size() )
         {
            continue ;
         }

         switch( token[ 7 ][ 0 ] )
         {
            case '-':
               fileType = "regular file" ;
               break ;
            case 'd':
               fileType = "directory" ;
               break ;
            case 'c':
               fileType = "character special file" ;
               break ;
            case 'b':
               fileType = "block special file" ;
               break ;
            case 'l':
               fileType = "symbolic link" ;
               break ;
            case 's':
               fileType = "socket" ;
               break ;
            case 'p':
               fileType = "pipe" ;
               break ;
            default:
               fileType = "unknow" ;
         }
         builder.append( "name", token[ 0 ] ) ;
         builder.append( "size", token[ 1 ] ) ;
         builder.append( "mode", token[ 7 ].substr( 1 ) ) ;
         builder.append( "user", token[ 2 ] ) ;
         builder.append( "group", token[ 3 ] ) ;
         builder.append( "accessTime", token[ 4 ] ) ;
         builder.append( "modifyTime", token[ 5 ] ) ;
         builder.append( "changeTime", token[ 6 ] ) ;
         builder.append( "type", fileType ) ;
         retObj = builder.obj() ;
         goto done ;
      }

      rc = SDB_SYS ;
      err = "Failed to build result" ;
      goto error ;

   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)
   INT32 _sptUsrFileCommon::getStat( const string &pathname, string &err,
                                    BSONObj &retObj )
   {
      INT32              rc = SDB_OK ;
      string             fileType ;
      SDB_OSS_FILETYPE   ossFileType ;
      CHAR               realPath[ OSS_MAX_PATHSIZE + 1 ] = { '\0' } ;
      BSONObjBuilder     builder ;
      INT64              fileSize ;
      stringstream       fileSizeStr ;

      if ( NULL == ossGetRealPath( pathname.c_str(),
                                   realPath,
                                   OSS_MAX_PATHSIZE + 1 ) )
      {
         rc = SDB_SYS ;
         err = "Failed to build real path" ;
         goto error ;
      }

      rc = ossGetPathType( realPath, &ossFileType ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to get file type" ;
         goto error ;
      }
      switch( ossFileType )
      {
         case SDB_OSS_FIL:
            fileType = "regular file" ;
            break ;
         case SDB_OSS_DIR:
            fileType = "directory" ;
            break ;
         default:
            fileType = "unknow" ;
      }

      rc = ossGetFileSizeByName( realPath, &fileSize ) ;
      if ( SDB_OK != rc )
      {
         err = "Failed to get file size" ;
         goto error ;
      }
      fileSizeStr << fileSize ;

      builder.append( "name", pathname ) ;
      builder.append( "size", fileSizeStr.str() ) ;
      builder.append( "mode", "" ) ;
      builder.append( "user", "" ) ;
      builder.append( "group", "" ) ;
      builder.append( "accessTime", "" ) ;
      builder.append( "modifyTime", "" ) ;
      builder.append( "changeTime", "" ) ;
      builder.append( "type", fileType ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }
#endif

   INT32 _sptUsrFileCommon::readFile( const string &filename,
                                      string &err, CHAR **pBuf, INT64 &readSize )
   {
      ossFile file ;
      INT64 size ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT ( pBuf, "Invalid arguments" ) ;

      rc = file.open ( filename.c_str() , OSS_READONLY | OSS_SHAREREAD, OSS_DEFAULTFILE ) ;
      if ( rc != SDB_OK )
      {
         err = "Can't open file: %s" + filename ;
         goto error ;
      }

      rc = file.getFileSize( size ) ;
      if ( rc != SDB_OK )
      {
         err = "Failed to get file's size" ;
         goto error ;
      }

      if ( *pBuf )
      {
         SDB_OSS_FREE( *pBuf ) ;
         *pBuf = NULL ;
      }
      *pBuf = (CHAR *) SDB_OSS_MALLOC ( size + 1 ) ;
      if ( ! *pBuf )
      {
         rc = SDB_OOM ;
         err = "Failed to alloc memory" ;
         goto error ;
      }

      rc = file.readN( *pBuf, size, readSize ) ;
      if ( rc != SDB_OK )
      {
         err = "Failed to read file" ;
         goto error ;
      }
      (*pBuf)[ readSize ] = 0 ;

   done :
      file.close() ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sptUsrFileCommon::md5( const string &filename,
                                string &err, string &code )
   {
      INT32 rc = SDB_OK ;
      SINT64 bufSize = SPT_MD5_READ_LEN ;
      SINT64 hasRead = 0 ;
      CHAR readBuf[ SPT_MD5_READ_LEN + 1 ] = { 0 } ;
      OSSFILE file ;
      stringstream ss ;
      BOOLEAN isOpen = FALSE ;
      md5_state_t st ;
      md5_init( &st ) ;
      md5::md5digest digest ;

      rc = ossOpen( filename.c_str(), OSS_READONLY | OSS_SHAREREAD,
                    OSS_DEFAULTFILE, file ) ;
      if ( rc )
      {
         ss << "Open file[" << filename.c_str() << "] failed: " << rc ;
         goto error ;
      }
      isOpen = TRUE ;

      while ( TRUE )
      {
         rc = ossReadN( &file, bufSize, readBuf, hasRead ) ;
         if ( SDB_EOF == rc || 0 == hasRead )
         {
            rc = SDB_OK ;
            break ;
         }
         else if ( rc )
         {
            ss << "Read file[" << filename.c_str() << "] failed, rc: " << rc ;
            goto error ;
         }
         md5_append( &st, (const md5_byte_t *)readBuf, hasRead ) ;
      }
      md5_finish( &st, digest ) ;
      code = md5::digestToString( digest ) ;;
   done:
      if ( TRUE == isOpen )
         ossClose( file ) ;
      return rc ;
   error:
      err = ss.str() ;
      goto done ;
   }

   INT32 _sptUsrFileCommon::_extractFindInfo( const CHAR* buf,
                                             BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;

      /*
      content format:
         xxxxxxxxx1
         xxxxxxxxx2
         xxxxxxxxx3
      */
      try
      {
         boost::algorithm::split( splited, buf,
                                  boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }

      for( UINT32 index = 0; index < splited.size(); index++ )
      {
         BSONObjBuilder objBuilder ;
         objBuilder.append( "pathname", splited[ index ] ) ;
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            objBuilder.obj() ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Fail to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#if defined (_LINUX)
   INT32 _sptUsrFileCommon::_extractListInfo( const CHAR* buf,
                                             BSONObjBuilder &builder,
                                             BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > fileVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /*
      content format:
         total 2
         drwxr-xr-x  7 root      root       4096 Oct 11 15:28 20000
         drwxr-xr-x  7 root      root       4096 Oct 11 15:05 30000
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of("\r\n") ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }
      splited.erase( splited.begin() ) ;

      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            if ( 9 > columns.size() )
            {
               continue ;
            }
            else
            {
               for ( UINT32 index = 9; index < columns.size(); index++ )
               {
                  columns[ 8 ] += " " + columns[ index ] ;
               }
            }
            fileObjBuilder.append( "name", columns[ 8 ] ) ;
            fileObjBuilder.append( "size", columns[ 4 ] ) ;
            fileObjBuilder.append( "mode", columns[ 0 ] ) ;
            fileObjBuilder.append( "user", columns[ 2 ] ) ;
            fileObjBuilder.append( "group", columns[ 3 ] ) ;
            fileObjBuilder.append( "lasttime",
                                   columns[ 5 ] + " " +
                                   columns[ 6 ] + " " +
                                   columns[ 7 ] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            if ( 9 > columns.size() )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to build result" ) ;
               goto error ;
            }
            else
            {
               for ( UINT32 index = 9; index < columns.size(); index++ )
               {
                  columns[ 8 ] += " " + columns[ index ] ;
               }
            }
            fileObjBuilder.append( "name", columns[ 8 ] ) ;
            fileObjBuilder.append( "mode", columns[ 0 ] ) ;
            fileObjBuilder.append( "user", columns[ 2 ] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }

      for( UINT32 index = 0; index < fileVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            fileVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#elif defined (_WINDOWS)

   INT32 _sptUsrFileCommon::_extractListInfo( const CHAR* buf,
                                             BSONObjBuilder &builder,
                                             BOOLEAN showDetail )
   {
      INT32 rc = SDB_OK ;
      vector<string> splited ;
      vector< BSONObj > fileVec ;

      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "buf can't be null, rc: %d", rc ) ;
         goto error ;
      }

      /*
       xxxxxxxxx
      xxxxxxxxxxx

      C:\Users\wujiaming\Documents\NetSarang\Xshell\Sessions xxxx

      content format:
         2016/10/11  13:26              3410 xxxxxx
         2016/05/18  08:56              3391 xxxxxxx
              12 xxxxx          37488 xxxx
               2 xxxxx    20122185728 xxxx
      */
      try
      {
         boost::algorithm::split( splited, buf, boost::is_any_of( "\r\n" ) ) ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                 rc, e.what() ) ;
         goto error ;
      }

      for ( vector<string>::iterator itr = splited.begin();
            itr != splited.end();  )
      {
         if ( itr->empty() )
         {
            itr = splited.erase( itr ) ;
         }
         else
         {
            itr++ ;
         }
      }
      splited.erase( splited.end() - 2, splited.end() ) ;
      splited.erase( splited.begin() , splited.begin() + 5 ) ;

      if( TRUE == showDetail )
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of( " " ) ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            if ( 4 > columns.size() )
            {
               continue ;
            }
            else
            {
               for ( UINT32 index = 4; index < columns.size(); index++ )
               {
                  columns[ 3 ] += " " + columns[ index ] ;
               }
            }
            if ( "<DIV>" == columns[ 2 ] )
            {
               columns[ 2 ] = "" ;
            }
            fileObjBuilder.append( "name", columns[ 3 ] ) ;
            fileObjBuilder.append( "size", columns[ 2 ] ) ;
            fileObjBuilder.append( "mode", "" ) ;
            fileObjBuilder.append( "user", "" ) ;
            fileObjBuilder.append( "group", "" ) ;
            fileObjBuilder.append( "lasttime",
                                   columns[ 0 ] + " " +columns[ 1 ] ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }
      else
      {
         for ( vector<string>::iterator itrSplit = splited.begin();
            itrSplit != splited.end(); itrSplit++ )
         {
            vector<string> columns ;
            BSONObjBuilder fileObjBuilder ;

            try
            {
               boost::algorithm::split( columns, *itrSplit,
                                        boost::is_any_of(" ") ) ;
            }
            catch( std::exception &e )
            {
               rc = SDB_SYS ;
               PD_LOG( PDERROR, "Failed to split result, rc: %d, detail: %s",
                       rc, e.what() ) ;
               goto error ;
            }
            for ( vector<string>::iterator itrCol = columns.begin();
                  itrCol != columns.end();  )
            {
               if ( itrCol->empty() )
               {
                  itrCol = columns.erase( itrCol ) ;
               }
               else
               {
                  itrCol++ ;
               }
            }

            if ( 4 > columns.size() )
            {
               continue ;
            }
            else
            {
               for ( UINT32 index = 4; index < columns.size(); index++ )
               {
                  columns[ 3 ] += " " + columns[ index ] ;
               }
            }
            fileObjBuilder.append( "name", columns[ 3 ] ) ;
            fileObjBuilder.append( "mode", "" ) ;
            fileObjBuilder.append( "user", "" ) ;
            fileVec.push_back( fileObjBuilder.obj() ) ;
         }
      }

      for( UINT32 index = 0; index < fileVec.size(); index++ )
      {
         try
         {
            builder.append( boost::lexical_cast<string>( index ).c_str(),
                            fileVec[ index ] ) ;
         }
         catch( std::exception &e )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to build retObj, rc: %d, detail: %s",
                    rc, e.what() ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

#endif

   INT32 _sptUsrFileCommon::getFileSize( const string &filename,
                                         string &err, INT64 &size )
   {
      INT32 rc = SDB_OK ;
      rc = ossGetFileSizeByName( filename.c_str(), &size ) ;
      if( SDB_OK != rc )
      {
         err = "Failed to get file size" ;
         PD_LOG( PDERROR, "Failed to get file size, filename: %s, rc: %d",
                 filename.c_str(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}
