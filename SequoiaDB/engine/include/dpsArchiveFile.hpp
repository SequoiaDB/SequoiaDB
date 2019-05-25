/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dpsArchiveFile.hpp

   Descriptive Name = Data Protection Services Log Archive File

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/3/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSARCHIVEFILE_HPP_
#define DPSARCHIVEFILE_HPP_

#include "dpsLogDef.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossIO.hpp"
#include "utilFileStream.hpp"
#include "utilZlibStream.hpp"
#include "pd.hpp"
#include <string>

using namespace std ;

namespace engine
{
   #define DPS_ARCHIVE_HEADER_EYECATCHER_LEN    8
   #define DPS_ARCHIVE_HEADER_LEN               (4 * 1024)
   #define DPS_ARCHIVE_HEADER_OFFSET            (60 * 1024)
   #define DPS_ARCHIVE_HEADER_EYECATCHER        "SDBARCHD"
   #define DPS_ARCHIVE_HEADER_VERSION           1
   #define DPS_ARCHIVE_FILE_PREFIX              "archivelog."

   #define DPS_ARCHIVE_COMPRESSED               0x00000001
   #define DPS_ARCHIVE_MOVED                    0x00000002
   #define DPS_ARCHIVE_PARTIAL                  0x00000004

   class _dpsLogHeader ;

   class dpsArchiveHeader: public SDBObject
   {
   public:
      CHAR    eyeCatcher[ DPS_ARCHIVE_HEADER_EYECATCHER_LEN ] ;
      UINT32  version ;
      UINT32  flag ;
      DPS_LSN startLSN ;
      DPS_LSN endLSN ;
      CHAR    padding[ DPS_ARCHIVE_HEADER_LEN - 48 ] ;

      dpsArchiveHeader()
      {
         init() ;
      }

      void init()
      {
         SDB_ASSERT( sizeof(dpsArchiveHeader) == DPS_ARCHIVE_HEADER_LEN,
                     "Archive file header size must be 4KB" ) ;
         SDB_ASSERT( sizeof(dpsArchiveHeader) + DPS_ARCHIVE_HEADER_OFFSET
                     <= DPS_LOG_HEAD_LEN,
                     "Archive file header must inside log file header" ) ;
         ossMemset( &padding, 0, DPS_ARCHIVE_HEADER_LEN - 48 ) ;
         ossMemcpy( eyeCatcher, DPS_ARCHIVE_HEADER_EYECATCHER,
                    DPS_ARCHIVE_HEADER_EYECATCHER_LEN ) ;
         version = DPS_ARCHIVE_HEADER_VERSION ;
         flag = 0 ;
         startLSN.reset() ;
         endLSN.reset() ;
      }

      BOOLEAN isValid() const
      {
         if ( 0 != ossStrncmp( eyeCatcher, DPS_ARCHIVE_HEADER_EYECATCHER,
                               DPS_ARCHIVE_HEADER_EYECATCHER_LEN ) )
         {
            return FALSE ;
         }

         if ( DPS_ARCHIVE_HEADER_VERSION != version )
         {
            return FALSE ;
         }

         return TRUE ;
      }

      OSS_INLINE void setFlag( UINT32 bit )
      {
         flag |= bit ;
      }

      OSS_INLINE void unsetFlag( UINT32 bit ) 
      {
         flag &= ~bit ;
      }

      OSS_INLINE BOOLEAN hasFlag( UINT32 bit )
      {
         return ( flag & bit ) ? TRUE : FALSE ;
      }
   } ;

   class dpsArchiveFile: public SDBObject
   {
   public:
      dpsArchiveFile() ;
      ~dpsArchiveFile() ;

   public:
      INT32 init( const string& path, BOOLEAN readOnly ) ;
      void  close() ;
      INT32 flushHeader() ;
      INT32 write( const CHAR* data, INT64 len ) ;
      INT32 write( INT64 offset, const CHAR* data, INT64 len ) ;
      INT32 write( ossFile& fromFile, BOOLEAN compress ) ;
      INT32 extend( INT64 fileSize ) ;

      _dpsLogHeader* getLogHeader()
      {
         SDB_ASSERT( _inited, "can't get log header before inited" ) ;
         SDB_ASSERT( NULL != _logHeader, "_logHeader can't be NULL" ) ;
         return _logHeader ;
      }

      dpsArchiveHeader* getArchiveHeader()
      {
         SDB_ASSERT( _inited, "can't get archive header before inited" ) ;
         SDB_ASSERT( NULL != _archiveHeader, "_archiveHeader can't be NULL" ) ;
         return _archiveHeader ;
      }

      OSS_INLINE const string& path() const { return _path; }

   private:
      INT32 _initHeader() ;
      INT32 _flushHeader() ;
      INT32 _readHeader() ;

   private:
      ossFile             _file ;
      _dpsLogHeader*      _logHeader ;
      dpsArchiveHeader*   _archiveHeader ;
      string              _path ;
      BOOLEAN             _readOnly ;
      BOOLEAN             _inited ;
   } ;
}

#endif /* DPSARCHIVEFILE_HPP_ */
