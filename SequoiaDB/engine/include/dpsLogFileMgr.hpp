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

   Source File Name = dpsLogFileMgr.hpp

   Descriptive Name = Data Protection Services Log File Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for log file manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGFILEMGR_HPP__
#define DPSLOGFILEMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogFile.hpp"
#include "dpsMetaFile.hpp"
#include "pmdDef.hpp"
#include <vector>
using namespace std;

namespace engine
{

   #define DPS_LOG_FILE_PREFIX "sequoiadbLog."

   class _dpsLogPage;
   class _dpsMessageBlock;
   class _dpsReplicaLogMgr ;

   /*
      _dpsLogFileMgr define
   */
   class _dpsLogFileMgr : public SDBObject
   {
   private:
      vector<_dpsLogFile *>   _files ;
      UINT32                  _work ;
      UINT32                  _logicalWork ;
      UINT32                  _begin ;
      BOOLEAN                 _rollFlag ;

      UINT32                  _logFileSz ;
      UINT32                  _logFileNum ;
      _dpsReplicaLogMgr       *_replMgr ;

   public:
      _dpsLogFileMgr( class _dpsReplicaLogMgr *replMgr );
      ~_dpsLogFileMgr();

      INT32 init( const CHAR *path, dpsMetaFileContent &content );
      void  fini() ;

      INT32 flush( _dpsMessageBlock *mb,
                   const DPS_LSN &beginLsn,
                   BOOLEAN shutdown = FALSE );

      INT32 load( const DPS_LSN &lsn, _dpsMessageBlock *mb,
                  BOOLEAN onlyHeader = FALSE,
                  UINT32 *pLength = NULL ) ;

      INT32 move( const DPS_LSN_OFFSET &offset, const DPS_LSN_VER &version ) ;

      void setLogFileSz ( UINT64 logFileSz )
      {
         _logFileSz = logFileSz ;
      }

      DPS_LSN getStartLSN ( BOOLEAN mustExist = TRUE ) ;

      UINT32 getLogFileSz ()
      {
         return _logFileSz ;
      }
      void setLogFileNum ( UINT32 logFileNum )
      {
         _logFileNum = logFileNum ;
      }
      UINT32 getLogFileNum ()
      {
         return _logFileNum ;
      }
      _dpsLogFile * getWorkLogFile()
      {
         return _files[_work] ;
      }

      _dpsLogFile* getLogFile( UINT32 fileId )
      {
         if ( fileId >= _logFileNum )
         {
            return NULL ;
         }

         return _files[ fileId ] ;
      }

      UINT32 getWorkPos() const
      {
         return _work ;
      }

      UINT32 getBeginPos() const
      {
         return _begin ;
      }

      UINT32 getLogicalWorkPos() const
      {
         return OSS_ONCE_UINT32_GET( _logicalWork ) ;
      }

      INT32 sync() ;

   protected:
      void     _analysis ( const dpsMetaFileContent &content,
                           BOOLEAN &needRetry ) ;
      void     _clear() ;

      UINT32   _incFileID ( UINT32 fileID ) ;
      UINT32   _decFileID ( UINT32 fileID ) ;
      void     _incLogicalFileID () ;

   };
   typedef class _dpsLogFileMgr dpsLogFileMgr ;

}

#endif // DPSLOGFILEMGR_HPP__

