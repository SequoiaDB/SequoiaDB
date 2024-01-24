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

   Source File Name = dpsLogFile.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGFILE_H_
#define DPSLOGFILE_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include "ossUtil.hpp"
#include "dpsLogDef.hpp"
#include <string>
#include "ossEvent.hpp"

using namespace std ;

namespace engine
{
#define DPS_LOG_HEADER_EYECATCHER      "SDBLOGHD"
#define DPS_LOG_HEADER_EYECATCHER_LEN  8

#define DPS_INVALID_LOG_FILE_ID     0xFFFFFFFF

#define DPS_LSN_2_FILEID(offset,fileSize)    (UINT32)(offset/fileSize)

#define DPS_FILEID_COMPARE(lID,rID) \
   ( lID == rID ? 0 : \
         ( DPS_INVALID_LOG_FILE_ID == lID ? -1 : \
              ( DPS_INVALID_LOG_FILE_ID == rID ? 1 : \
                   ( lID < rID ? -1 : 1 )  ) ) )

#define DPS_LOG_FILE_VERSION1       (1)

   /*
      _dpsLogHeader define
   */
   class _dpsLogHeader : public SDBObject
   {
   public :
      CHAR     _eyeCatcher [ DPS_LOG_HEADER_EYECATCHER_LEN ] ;
      DPS_LSN  _firstLSN ;
      UINT32   _logID ;
      UINT32   _version ;
      UINT64   _fileSize ;
      UINT32   _fileNum ;
      CHAR     _padding [ DPS_LOG_HEAD_LEN - 48 ] ;

      _dpsLogHeader ()
      {
         ossMemcpy( _eyeCatcher, DPS_LOG_HEADER_EYECATCHER, 
                    DPS_LOG_HEADER_EYECATCHER_LEN) ;
         _logID      = DPS_INVALID_LOG_FILE_ID ;
         _version    = DPS_LOG_FILE_VERSION1 ;
         _fileSize   = 0 ;
         _fileNum    = 0 ;
         ossMemset(_padding, 0, sizeof(_padding) ) ;

         SDB_ASSERT( sizeof(_dpsLogHeader) == DPS_LOG_HEAD_LEN,
                     "Log file header size must be 64K" ) ;
      }
   } ;
   typedef class _dpsLogHeader dpsLogHeader ;

   /*
      _dpsLogFile define
   */
   class _dpsLogFile : public SDBObject
   {
   private:
      _OSS_FILE      *_file ;
      // 32 bit size, so we can support up to 4GB file size
      UINT32         _fileSize ;
      UINT32         _fileNum ;
      UINT32         _idleSize ;
      dpsLogHeader   _logHeader ;
      ossAutoEvent   _writeEvent ;
      BOOLEAN        _inRestore ;
      BOOLEAN        _dirty ;
      string         _path ;

   public:
      _dpsLogFile();

      ~_dpsLogFile();

   public:
      OSS_INLINE UINT32 size()
      {
         return _fileSize;
      }

      OSS_INLINE dpsLogHeader &header()
      {
         return _logHeader ;
      }

      OSS_INLINE void idleSize( UINT32 size )
      {
         _idleSize = size ;
      }

      OSS_INLINE const string& path() const
      {
         return _path ;
      }

      string toString() const ;

   public:
      // initialize file
      INT32 init ( const CHAR *path,
                   UINT32 fileSize,
                   UINT32 fileNum,
                   INT32 length = -1,
                   BOOLEAN *pNeedRetry = NULL ) ;
      // write into file
      INT32 write ( const CHAR *content, UINT32 len ) ;
      // read from file
      INT32 read ( const DPS_LSN_OFFSET &lOffset, UINT32 len, CHAR *buf ) ;
      // close the file
      INT32 close();
      // get how much space we still able to write
      UINT32 getIdleSize() { return _idleSize ; }
      UINT32 getLength () { return _fileSize - _idleSize ; }
      UINT32 getValidLength() const ;
      // reset metadata
      INT32 reset ( UINT32 logID, const DPS_LSN_OFFSET &offset,
                    const DPS_LSN_VER &version ) ;
      // get first lsn
      DPS_LSN getFirstLSN ( BOOLEAN mustExist = TRUE ) ;

      BOOLEAN isZeroStart()
      {
         if ( _logHeader._logID != DPS_INVALID_LOG_FILE_ID &&
              !_logHeader._firstLSN.invalid() &&
              _logHeader._firstLSN.offset % _fileSize == 0 )
         {
            return TRUE ;
         }
         return FALSE ;
      }

      INT32 invalidateData() ;

      BOOLEAN isDirty() const
      {
         return _dirty ;
      }

      INT32 sync() ;
   private:
      void _initHead( UINT32 logID )
      {
         ossMemcpy ( &_logHeader._eyeCatcher, DPS_LOG_HEADER_EYECATCHER,
                     DPS_LOG_HEADER_EYECATCHER_LEN ) ;
         _logHeader._logID = logID ;
         _logHeader._fileNum = _fileNum ;
         _logHeader._fileSize = _fileSize ;
         _logHeader._version  = DPS_LOG_FILE_VERSION1 ;
      }
      // flush log file header
      INT32 _flushHeader() ;
      // restore header
      INT32 _readHeader () ;
      // restore from file
      INT32 _restore( BOOLEAN crashStart, INT32 length ) ;
   };
   typedef class _dpsLogFile dpsLogFile;
}

#endif

