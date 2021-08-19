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

   Source File Name = rplSimpleSqlOutputter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/04/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rplRecordWriter.hpp"
#include "ossUtil.h"
#include "utilStr.hpp"
#include "rplUtil.hpp"

using namespace std ;
using namespace bson ;
using namespace engine ;

namespace replay
{
   #define RPL_STATUS_FILE_LIST              "fileStatus"
   #define RPL_STATUS_FILE_NAME              "name"
   #define RPL_STATUS_FILE_TABLENAME         "tableName"
   #define RPL_STATUS_FILE_SIZE              "size"

   const CHAR SQL_FILE_NEWLINE[] = "\n" ;
   const CHAR SQL_FILE_TMP_SUFFIX[] = ".tmp" ;

   rplFileWriter::rplFileWriter()
   {
      _size = 0 ;
   }

   rplFileWriter::~rplFileWriter()
   {
      if ( _writer.isOpened() )
      {
         // it's ok to leave the tmp file
         _writer.close() ;
      }
   }

   INT32 rplFileWriter::restore( const CHAR *fileName, INT64 size )
   {
      INT32 rc = SDB_OK ;
      UINT32 mode = OSS_READWRITE ;
      INT64 realSize = 0 ;
      BOOLEAN isExist = FALSE ;

      if ( NULL == fileName )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Input fileName is NULL, rc = %d", rc) ;
      }

      ossStrcpy( _fileName, fileName ) ;
      _fileName[ OSS_MAX_PATHSIZE ] = '\0' ;
      ossSnprintf( _tmpFileName, OSS_MAX_PATHSIZE, "%s%s", fileName,
                   SQL_FILE_TMP_SUFFIX ) ;

      rc = _writer.exists( _tmpFileName, isExist ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check file(%s), rc = %d",
                   _tmpFileName, rc ) ;
      if ( !isExist )
      {
         rc = SDB_FNE ;
         goto error ;
      }

      rc = _writer.open( _tmpFileName, mode, OSS_DEFAULTFILE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open file(%s), rc = %d",
                   _tmpFileName, rc ) ;

      rc = _writer.getFileSize( realSize ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get file size(%s), rc = %d",
                   _tmpFileName, rc ) ;

      if ( realSize < size )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Real size(%llu) is less than expected size(%llu): "
                 "rc = %d", realSize, size, rc ) ;
         goto error ;
      }

      rc = _writer.truncate( size ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to truncate file (%s), size = %llu,"
                   " rc = %d", _tmpFileName, size, rc ) ;

      rc = _writer.seek( size, OSS_SEEK_SET ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to seek file size(%s), offset = %llu, "
                   "rc = %d", _tmpFileName, size, rc ) ;

      _size = size ;
   done:
      return rc ;
   error:
      if ( _writer.isOpened() )
      {
         _writer.close() ;
      }

      goto done ;
   }

   INT32 rplFileWriter::init( const CHAR *fileName )
   {
      INT32 rc = SDB_OK ;
      UINT32 mode = OSS_REPLACE | OSS_READWRITE ;

      if ( NULL == fileName )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Input fileName is NULL, rc = %d", rc) ;
      }

      if ( ossStrlen(fileName) + ossStrlen(SQL_FILE_TMP_SUFFIX)
           > OSS_MAX_PATHSIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "FileName(%s) is too long, rc = %d",
                      fileName, rc ) ;
      }

      ossMemset( _fileName, 0, sizeof(_fileName) ) ;
      ossStrncpy( _fileName, fileName, OSS_MAX_PATHSIZE ) ;

      ossMemset( _tmpFileName, 0, sizeof(_tmpFileName) ) ;
      ossSnprintf( _tmpFileName, OSS_MAX_PATHSIZE, "%s%s", fileName,
                   SQL_FILE_TMP_SUFFIX ) ;

      rc = _writer.open( _tmpFileName, mode, OSS_DEFAULTFILE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create file(%s), rc = %d",
                   _tmpFileName, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplFileWriter::writeRecord( const CHAR * record )
   {
      INT32 rc = SDB_OK ;
      INT64 len = ossStrlen( record ) ;

      if ( _size > 0 )
      {
         rc = _writer.writeN( SQL_FILE_NEWLINE, ossStrlen( SQL_FILE_NEWLINE ) ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to writeN record(%s), rc = %d",
                      SQL_FILE_NEWLINE, rc ) ;

         _size += ossStrlen( SQL_FILE_NEWLINE ) ;
      }

      rc = _writer.writeN( record, len ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to writeN record(%s), rc = %d",
                   record, rc ) ;

      _size += len ;

   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR *rplFileWriter::getTmpFileName()
   {
      return _tmpFileName ;
   }

   const CHAR *rplFileWriter::getFileName()
   {
      return _fileName ;
   }

   UINT64 rplFileWriter::getWriteSize()
   {
      return _size ;
   }

   INT32 rplFileWriter::flush()
   {
      INT32 rc = SDB_OK ;

      rc = _writer.sync() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync file(%s), rc = %d",
                   _tmpFileName, rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplFileWriter::flushAndClose()
   {
      INT32 rc = SDB_OK ;

      rc = _writer.sync() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to sync file(%s), rc = %d",
                   _tmpFileName, rc ) ;

      rc = _writer.close() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to close file(%s), rc = %d",
                   _tmpFileName, rc ) ;

      rc = ossRenamePath( _tmpFileName, _fileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to rename file(%s) to %s, rc = %d",
                   _tmpFileName, _fileName , rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   rplRecordWriter::rplRecordWriter( Monitor *monitor, const CHAR *outputDir,
                                     const CHAR *prefix, const CHAR *suffix )
   {
      _outputDir = outputDir ;
      if ( prefix[0] != '\0' )
      {
         _prefixWithConnector = string(prefix) + "_" ;
      }

      if ( suffix[0] != '\0' )
      {
         _suffixWithConnector = string("_") + suffix ;
      }

      _monitor = monitor ;
   }

   rplRecordWriter::~rplRecordWriter()
   {
      CL2WriterMap::iterator iter = _clWriters.begin() ;
      while ( iter != _clWriters.end() )
      {
         SAFE_OSS_DELETE( iter-> second ) ;
         iter++ ;
      }

      _clWriters.clear() ;
      _monitor = NULL ;
   }

   INT32 rplRecordWriter::init()
   {
      INT32 rc = SDB_OK ;
      BSONElement eleFileList ;
      BSONObj extraInfo = _monitor->getExtraInfo() ;

      if ( extraInfo.isEmpty() )
      {
         goto done ;
      }

      eleFileList = extraInfo.getField( RPL_STATUS_FILE_LIST ) ;
      if ( Array != eleFileList.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse fileList(%s), rc = %d",
                      extraInfo.toString().c_str(), rc ) ;
      }

      {
         BSONObjIterator fileIter( eleFileList.embeddedObject() ) ;
         while ( fileIter.more() )
         {
            BSONObj fileInfo ;
            BSONElement ele ;
            string tableName ;
            string fileName ;
            INT64 fileSize = 0 ;

            BSONElement fileEle = fileIter.next() ;
            if ( Object != fileEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse fileInfo(%s), rc = %d",
                            fileEle.toString().c_str(), rc ) ;
            }

            fileInfo = fileEle.embeddedObject() ;

            ele = fileInfo.getField( RPL_STATUS_FILE_NAME ) ;
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse fileInfo(%s), "
                            "rc = %d", fileInfo.toString().c_str(), rc ) ;
            }
            fileName = ele.str() ;

            ele = fileInfo.getField( RPL_STATUS_FILE_TABLENAME ) ;
            if ( String != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse fileInfo(%s), "
                            "rc = %d", fileInfo.toString().c_str(), rc ) ;
            }
            tableName = ele.str() ;

            ele = fileInfo.getField( RPL_STATUS_FILE_SIZE ) ;
            if ( !ele.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse fileInfo(%s), "
                            "rc = %d", fileInfo.toString().c_str(), rc ) ;
            }
            fileSize = ele.numberLong() ;

            rc = _restoreFile( tableName.c_str(), fileName.c_str(), fileSize ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to restore file(%s), rc = %d",
                         fileName.c_str(), rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplRecordWriter::_restoreFile(const CHAR *tableName,
                                       const CHAR *fileName, INT64 size )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isNeedDelete = FALSE ;
      rplFileWriter *fileWriter = NULL ;

      fileWriter = SDB_OSS_NEW rplFileWriter() ;
      if ( NULL == fileWriter )
      {
         rc = SDB_OOM ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate rplFileWriter,"
                      " rc = %d", rc ) ;
      }
      isNeedDelete = TRUE ;

      rc = fileWriter->restore( fileName, size ) ;
      if ( SDB_FNE == rc )
      {
         rc = SDB_OK ;
         PD_LOG( PDWARNING, "File(%s) is not exist", fileName ) ;
         SAFE_OSS_DELETE( fileWriter ) ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to restore writer(%s), rc = %d",
                   fileName, rc ) ;

      _clWriters[ tableName ] = fileWriter ;
      isNeedDelete = FALSE ;

   done:
      return rc ;
   error:
      if ( isNeedDelete )
      {
         SAFE_OSS_DELETE( fileWriter ) ;
      }
      goto done ;
   }

   INT32 rplRecordWriter::writeRecord( const CHAR *dbName,
                                       const CHAR *tableName,
                                       UINT64 lsn,
                                       const CHAR *record )
   {
      INT32 rc = SDB_OK ;
      rplFileWriter *fileWriter = NULL ;
      BOOLEAN isNeedDelete = FALSE ;
      string fullDBName = dbName ;
      fullDBName += "." ;
      fullDBName += tableName ;

      CL2WriterMap::iterator iter = _clWriters.find( fullDBName ) ;
      if ( iter == _clWriters.end() )
      {
         CHAR fullFileName[ OSS_MAX_PATHSIZE + 1 ] = "\0" ;
         CHAR fileName[ OSS_MAX_PATHSIZE + 1 ] = "\0" ;
         string dateStr ;
         UINT64 serial = _monitor->getSerial() ;
         serial++ ;
         if ( serial >= 9999999999 )
         {
            serial = 0 ;
         }
         _monitor->setSerial( serial ) ;
         getCurrentDate( dateStr ) ;

         ossSnprintf( fileName, OSS_MAX_PATHSIZE,
                      "%s%s_%s_%010lld_%lld_%s%s.csv",
                      _prefixWithConnector.c_str(), dbName, tableName, serial,
                      lsn, dateStr.c_str(), _suffixWithConnector.c_str() ) ;

         isNeedDelete = TRUE ;
         fileWriter = SDB_OSS_NEW rplFileWriter() ;
         if ( NULL == fileWriter )
         {
            rc = SDB_OOM ;
            PD_RC_CHECK( rc, PDERROR, "Failed to allocate rplFileWriter,"
                         " rc = %d", rc ) ;
         }

         rc = utilBuildFullPath( _outputDir.c_str(), fileName, OSS_MAX_PATHSIZE,
                                 fullFileName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build path(%s,%s), rc = %d",
                      _outputDir.c_str(), fileName, rc ) ;

         rc = fileWriter->init( fullFileName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to init writer(%s), rc = %d",
                      fullFileName, rc ) ;

         _clWriters[ fullDBName ] = fileWriter ;
         isNeedDelete = FALSE ;
      }
      else
      {
         fileWriter = iter->second ;
      }

      rc = fileWriter->writeRecord( record ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write record(%s), rc = %d",
                   record, rc ) ;

   done:
      return rc ;
   error:
      if ( isNeedDelete )
      {
         SAFE_OSS_DELETE( fileWriter ) ;
      }
      goto done ;
   }

   INT32 rplRecordWriter::flush()
   {
      INT32 rc = SDB_OK ;
      CL2WriterMap::iterator iter = _clWriters.begin() ;
      while ( iter != _clWriters.end() )
      {
         rplFileWriter *writer = iter->second ;
         rc = writer->flush() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to flush file(%s), "
                      "rc = %d", writer->getTmpFileName(), rc ) ;
         iter++ ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 rplRecordWriter::getStatus( BSONObj &status )
   {
      INT32 rc = SDB_OK ;
      try
      {
         BSONObjBuilder builder ;
         BSONArrayBuilder statusBuilder(
                             builder.subarrayStart( RPL_STATUS_FILE_LIST ) ) ;
         CL2WriterMap::iterator iter = _clWriters.begin() ;
         while ( iter != _clWriters.end() )
         {
            rplFileWriter *writer = iter->second ;
            BSONObjBuilder fileBuilder ;

            fileBuilder.append( RPL_STATUS_FILE_TABLENAME, iter->first ) ;
            fileBuilder.append( RPL_STATUS_FILE_NAME, writer->getFileName() ) ;
            fileBuilder.append( RPL_STATUS_FILE_SIZE,
                                (INT64)writer->getWriteSize() ) ;
            statusBuilder.append( fileBuilder.obj() ) ;

            iter++ ;
         }

         statusBuilder.done() ;

         status = builder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
      }

      return rc ;
   }

   INT32 rplRecordWriter::submit()
   {
      INT32 rc = SDB_OK ;
      CL2WriterMap::iterator iter = _clWriters.begin() ;
      while ( iter != _clWriters.end() )
      {
         rplFileWriter *writer = iter->second ;
         rc = writer->flushAndClose() ;
         PD_RC_CHECK( rc, PDERROR, "Failed to commit and close file(%s), "
                      "rc = %d", writer->getTmpFileName(), rc ) ;

         // remove from map anyway
         SAFE_OSS_DELETE( writer ) ;
         _clWriters.erase( iter++ ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}

