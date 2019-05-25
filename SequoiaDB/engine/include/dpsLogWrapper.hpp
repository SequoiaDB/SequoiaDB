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

   Source File Name = dpsLogWrapper.hpp

   Descriptive Name = Data Protection Services Log Wrapper Header

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains declare for dpsLogWrapper.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGWRAPPER_HPP__
#define DPSLOGWRAPPER_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "sdbInterface.hpp"
#include "sdbIPersistence.hpp"
#include "dpsReplicaLogMgr.hpp"
#include "dpsArchiveMgr.hpp"
#include "../bson/bsonelement.h"
#include "../bson/bsonobj.h"
#include <vector>
using namespace bson;
using namespace std ;

namespace engine
{

   /*
      macro define
   */
   #define DPS_DFT_LOG_BUF_SZ          (1024)

   class _pmdEDUCB ;


   /*
      _dpsLogAccessor define
   */
   class _dpsLogAccessor
   {
      public:
         _dpsLogAccessor() {}
         virtual ~_dpsLogAccessor() {}

      public:
         virtual INT32     search( const DPS_LSN &minLsn,
                                   _dpsMessageBlock *mb,
                                   UINT8 type = DPS_SEARCH_ALL,
                                   INT32 maxNum = 1,        // -1 for unlimited
                                   INT32 maxTime = -1,      // -1 for unlimited
                                   INT32 maxSize = 5242880  // -1 for unlimited
                                   ) = 0 ;

         virtual INT32     searchHeader( const DPS_LSN &lsn,
                                         _dpsMessageBlock *mb,
                                         UINT8 type = DPS_SEARCH_ALL ) = 0 ;

         virtual DPS_LSN   getStartLsn ( BOOLEAN logBufOnly = FALSE ) = 0 ;

         virtual DPS_LSN   getCurrentLsn() = 0 ;
         virtual DPS_LSN   expectLsn() = 0 ;
         virtual DPS_LSN   commitLsn() = 0 ;

         virtual void      getLsnWindow( DPS_LSN &beginLsn,
                                         DPS_LSN &memBeginLsn,
                                         DPS_LSN &endLsn,
                                         DPS_LSN *pExpectLsn,
                                         DPS_LSN *committed ) = 0 ;

         virtual void      getLsnWindow( DPS_LSN &beginLsn,
                                         DPS_LSN &endLsn,
                                         DPS_LSN *pExpectLsn,
                                         DPS_LSN *committed ) = 0 ;

         virtual INT32     move( const DPS_LSN_OFFSET &offset,
                                 const DPS_LSN_VER &version ) = 0 ;

         virtual INT32     recordRow( const CHAR *row, UINT32 len ) = 0 ;

   } ;
   typedef _dpsLogAccessor ILogAccessor ;


   /*
      _dpsLogWrapper define
   */
   class _dpsLogWrapper : public _IControlBlock, public ILogAccessor,
                          public IDataSyncBase
   {
   private:
      _dpsReplicaLogMgr          _buf ;
      BOOLEAN                    _initialized ;
      BOOLEAN                    _dpslocal ;
      vector< dpsEventHandler* > _vecEventHandler ;
      dpsArchiveMgr              _archiver ;

      UINT32                     _syncInterval ;
      UINT32                     _syncRecordNum ;

      UINT32                     _writeReordNum ;
      UINT64                     _lastWriteTick ;
      UINT64                     _lastSyncTime ;

   public:
      _dpsLogWrapper() ;
      virtual ~_dpsLogWrapper() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_DPS ; }
      virtual const CHAR* cbName() const { return "DPSCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

   public:
      virtual INT32     search( const DPS_LSN &minLsn,
                                _dpsMessageBlock *mb,
                                UINT8 type = DPS_SEARCH_ALL,
                                INT32 maxNum = 1,
                                INT32 maxTime = -1,
                                INT32 maxSize = 5242880 ) ;

      virtual INT32     searchHeader( const DPS_LSN &lsn,
                                      _dpsMessageBlock *mb,
                                      UINT8 type = DPS_SEARCH_ALL ) ;

      virtual DPS_LSN   getStartLsn ( BOOLEAN logBufOnly = FALSE ) ;

      virtual DPS_LSN   getCurrentLsn() ;
      virtual DPS_LSN   expectLsn() ;
      virtual DPS_LSN   commitLsn() ;

      virtual void      getLsnWindow( DPS_LSN &beginLsn,
                                      DPS_LSN &memBeginLsn,
                                      DPS_LSN &endLsn,
                                      DPS_LSN *pExpectLsn,
                                      DPS_LSN *committed ) ;

      virtual void      getLsnWindow( DPS_LSN &beginLsn,
                                      DPS_LSN &endLsn,
                                      DPS_LSN *pExpectLsn,
                                      DPS_LSN *committed ) ;

      virtual INT32     move( const DPS_LSN_OFFSET &offset,
                              const DPS_LSN_VER &version ) ;

      virtual INT32     recordRow( const CHAR *row, UINT32 len ) ;

   public:
         virtual BOOLEAN      isClosed() const ;
         virtual BOOLEAN      canSync( BOOLEAN &force ) const ;

         virtual INT32        sync( BOOLEAN force,
                                    BOOLEAN sync,
                                    IExecutor* cb ) ;

         virtual void         lock() ;
         virtual void         unlock() ;

   public:
      void regEventHandler( dpsEventHandler *pHandler ) ;
      void unregEventHandler( dpsEventHandler *pHandler ) ;

      OSS_INLINE _dpsReplicaLogMgr *getLogMgr ()
      {
         return &_buf ;
      }
      OSS_INLINE BOOLEAN isLogLocal() const
      {
         return _dpslocal ;
      }
      OSS_INLINE INT32 run( _pmdEDUCB *cb )
      {
         if ( !_initialized )
         {
            return SDB_OK ;
         }
         return _buf.run( cb );
      }
      OSS_INLINE INT32 tearDown()
      {
         if ( !_initialized )
         {
            return SDB_OK ;
         }
         return _buf.tearDown();
      }
      OSS_INLINE BOOLEAN doLog () const
      {
         return _initialized ;
      }
      OSS_INLINE INT32 archive()
      {
         if ( !_initialized )
         {
            return SDB_OK ;
         }
         return _archiver.run() ;
      }

      OSS_INLINE INT32 flushAll()
      {
         SDB_ASSERT ( _initialized, "shouldn't call flushAll without init" ) ;
         return _buf.flushAll() ;
      }

      OSS_INLINE void incVersion()
      {
         _buf.incVersion() ;
      }

      OSS_INLINE void cancelIncVersion()
      {
         _buf.cancelIncVersion() ;
      }

      OSS_INLINE INT32 checkSyncControl( UINT32 reqLen, _pmdEDUCB *cb )
      {
         return _buf.checkSyncControl( reqLen, cb ) ;
      }

      INT32 commit( BOOLEAN deeply, DPS_LSN *committedLsn ) ;

   public:
      INT32 prepare( dpsMergeInfo &info ) ;
      void  writeData ( dpsMergeInfo &info ) ;
      INT32 completeOpr( _pmdEDUCB *cb, INT32 w ) ;

      void setLogFileSz ( UINT32 logFileSz )
      {
         _buf.setLogFileSz ( logFileSz ) ;
      }
      UINT32 getLogFileSz ()
      {
         return _buf.getLogFileSz () ;
      }
      void setLogFileNum ( UINT32 logFileNum )
      {
         _buf.setLogFileNum ( logFileNum ) ;
      }
      UINT32 getLogFileNum ()
      {
         return _buf.getLogFileNum () ;
      }
      UINT32 calcFileID ( DPS_LSN_OFFSET offset )
      {
         return _buf.calcFileID( offset ) ;
      }

      BOOLEAN isInRestore() ;

   };
   typedef class _dpsLogWrapper SDB_DPSCB ;

   /*
      get dps cb
   */
   SDB_DPSCB* sdbGetDPSCB() ;

}

#endif // DPSLOGWRAPPER_HPP__
