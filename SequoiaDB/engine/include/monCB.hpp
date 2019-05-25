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

   Source File Name = monCB.hpp

   Descriptive Name = Monitor Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   application and context snapshot.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MONCB_HPP_
#define MONCB_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossAtomic.hpp"
#include "msg.h"

namespace engine
{
   #define MON_START_OP( _pMonAppCB_ )                      \
   {                                                        \
      if ( NULL != _pMonAppCB_ )                            \
      {                                                     \
         _pMonAppCB_->startOperator() ;                     \
      }                                                     \
   }

   #define MON_END_OP( _pMonAppCB_ )                        \
   {                                                        \
      if ( NULL != _pMonAppCB_ )                            \
      {                                                     \
         _pMonAppCB_->endOperator() ;                       \
      }                                                     \
   }

   #define MON_SET_OP_TYPE( _pMonAppCB_, opType )           \
   {                                                        \
      if ( NULL != _pMonAppCB_ )                            \
      {                                                     \
         _pMonAppCB_->setLastOpType( opType ) ;             \
      }                                                     \
   }

   #define MON_SAVE_OP_DETAIL( _pMonAppCB_, opType, format, ... )    \
   {                                                                 \
      if ( NULL != _pMonAppCB_ )                                     \
      {                                                              \
         try {                                                       \
            _pMonAppCB_->setLastOpType( opType ) ;                   \
            _pMonAppCB_->setLastCmdType( CMD_UNKNOW ) ;              \
            _pMonAppCB_->saveLastOpDetail( format,                   \
                                           ##__VA_ARGS__ ) ;         \
         } catch( ... ) {}                                           \
      }                                                              \
   }

   #define MON_SAVE_CMD_DETAIL( _pMonAppCB_, cmdType, format, ... )  \
   {                                                                 \
      if ( NULL != _pMonAppCB_ )                                     \
      {                                                              \
         try {                                                       \
            _pMonAppCB_->setLastOpType( MSG_BS_QUERY_REQ ) ;         \
            _pMonAppCB_->setLastCmdType( cmdType ) ;                 \
            _pMonAppCB_->saveLastOpDetail( format,                   \
                                           ##__VA_ARGS__ ) ;         \
         } catch(...) {}                                             \
      }                                                              \
   }

   /*
      Common Define
   */
   #define MON_APP_LASTOP_DESC_LEN                 ( 1024 )

   /*
      _monConfigCB define
   */
   struct _monConfigCB : public SDBObject
   {
      BOOLEAN timestampON ;
   } ;
   typedef _monConfigCB monConfigCB ;

   /*
      MON_OPERATION_TYPES define
   */
   enum MON_OPERATION_TYPES
   {
      MON_COUNTER_OPERATION_NONE = 0,
      MON_DATA_READ,
      MON_INDEX_READ,
      MON_LOB_READ,
      MON_TEMP_READ,
      MON_DATA_WRITE,
      MON_INDEX_WRITE,
      MON_LOB_WRITE,
      MON_TEMP_WRITE,
      MON_UPDATE,
      MON_DELETE,
      MON_INSERT,
      MON_UPDATE_REPL,
      MON_DELETE_REPL,
      MON_INSERT_REPL,
      MON_SELECT,
      MON_READ,
      MON_COUNTER_OPERATION_MAX = MON_READ,

      MON_TIME_OPERATION_NONE = 0,
      MON_TOTAL_WAIT_TIME,
      MON_TOTAL_READ_TIME,
      MON_TOTAL_WRITE_TIME,
      MON_TIME_OPERATION_MAX = MON_TOTAL_WRITE_TIME
   } ;

   /*
      _monDBCB define
   */
   class _monDBCB : public SDBObject
   {
   public :
      volatile UINT64 totalDataRead ;
      volatile UINT64 totalIndexRead ;
      volatile UINT64 totalLobRead ;
      volatile UINT64 totalDataWrite ;
      volatile UINT64 totalIndexWrite ;
      volatile UINT64 totalLobWrite ;

      volatile UINT64 totalUpdate ;
      volatile UINT64 totalDelete ;
      volatile UINT64 totalInsert ;
      volatile UINT64 totalSelect ;  // total records into result set
      volatile UINT64 totalRead ;    // total records readed from disk

      volatile UINT64 receiveNum ;

      volatile UINT64 replUpdate ;   // IUD caused by replica copy
      volatile UINT64 replDelete ;
      volatile UINT64 replInsert ;

      volatile UINT64 _svcNetIn ;
      volatile UINT64 _svcNetOut ;

      ossTickDelta    totalReadTime ;
      ossTickDelta    totalWriteTime ;
      ossTimestamp    _activateTimestamp ;
      ossTimestamp    _resetTimestamp ;

      /*
         Must be Atomic for _curConns
      */
      ossAtomic32     _curConns ;

   /*
      Functions
   */
   public:
      UINT32   getCurConns() { return _curConns.fetch() ; }
      void     connInc() { _curConns.inc() ; }
      void     connDec() { _curConns.dec() ; }
      BOOLEAN  isConnLimited( UINT32 maxConn ) ;

      void monOperationTimeInc( MON_OPERATION_TYPES op, ossTickDelta &delta )
      {
         switch ( op )
         {
            case MON_TOTAL_READ_TIME :
               totalReadTime += delta ;
               break ;

            case MON_TOTAL_WRITE_TIME :
               totalWriteTime += delta ;
               break ;

            default :
               break ;
         }
      }

      void monOperationCountInc( MON_OPERATION_TYPES op, UINT64 delta = 1 )
      {
         switch ( op )
         {
            case MON_DATA_READ :
               totalDataRead += delta ;
               break ;

            case MON_INDEX_READ :
               totalIndexRead += delta ;
               break ;

            case MON_LOB_READ :
               totalLobRead += delta ;
               break ;

            case MON_DATA_WRITE :
               totalDataWrite += delta ;
               break ;

            case MON_INDEX_WRITE :
               totalIndexWrite += delta ;
               break ;

            case MON_LOB_WRITE :
               totalLobWrite += delta ;
               break ;

            case MON_UPDATE :
               totalUpdate += delta ;
               break ;

            case MON_DELETE :
               totalDelete += delta ;
               break ;

            case MON_INSERT :
               totalInsert += delta ;
               break ;

            case MON_UPDATE_REPL :
               replUpdate += delta ;
               break ;

            case MON_DELETE_REPL :
               replDelete += delta ;
               break ;

            case MON_INSERT_REPL :
               replInsert += delta ;
               break ;

            case MON_SELECT :
               totalSelect += delta ;
               break ;

            case MON_READ :
               totalRead += delta ;
               break ;

            default:
               break ;
         }
      }

      UINT64 getReceiveNum() { return receiveNum ; }
      void   addReceiveNum() { ++receiveNum ; }

      void   svcNetInAdd( INT32 sendSize ) { _svcNetIn += sendSize ; }
      void   svcNetOutAdd( INT32 recvSize ) { _svcNetOut += recvSize ; }
      UINT64 svcNetIn() { return _svcNetIn ; }
      UINT64 svcNetOut() { return _svcNetOut ; }

      _monDBCB() ;
      _monDBCB& operator= ( const _monDBCB &rhs ) ;
      void   reset() ;
      void   recordActivateTimestamp() ;

   } ;
   typedef _monDBCB monDBCB ;

   /*
      _monAppCB define
   */
   class _monAppCB : public SDBObject
   {
   public :
      monDBCB *mondbcb ;
      UINT64 totalDataRead ;
      UINT64 totalIndexRead ;
      UINT64 totalLobRead ;
      UINT64 totalDataWrite ;
      UINT64 totalIndexWrite ;
      UINT64 totalLobWrite ;

      UINT64 totalUpdate ;
      UINT64 totalDelete ;
      UINT64 totalInsert ;
      UINT64 totalSelect ;  // total records into result set
      UINT64 totalRead ;    // total records readed from disk

      ossTickDelta   totalReadTime ;
      ossTickDelta   totalWriteTime ;
      ossTimestamp   _connectTimestamp ;
      ossTimestamp   _resetTimestamp ;

      INT32          _lastOpType ;
      INT32          _cmdType ;
      ossTick        _lastOpBeginTime ;
      ossTick        _lastOpEndTime ;
      ossTickDelta   _readTimeSpent ;
      ossTickDelta   _writeTimeSpent ;
      CHAR           _lastOpDetail[ MON_APP_LASTOP_DESC_LEN + 1 ] ;

      void monOperationTimeInc( MON_OPERATION_TYPES op, ossTickDelta &delta )
      {
         switch ( op )
         {
            case MON_TOTAL_READ_TIME :
               totalReadTime += delta ;
               break ;

            case MON_TOTAL_WRITE_TIME :
               totalWriteTime += delta ;
               break ;

            default :
               break ;
         }
         mondbcb->monOperationTimeInc( op, delta ) ;
      }

      void monOperationCountInc( MON_OPERATION_TYPES op, UINT64 delta = 1 )
      {
         switch ( op )
         {
            case MON_DATA_READ :
               totalDataRead += delta ;
               break ;

            case MON_INDEX_READ :
               totalIndexRead += delta ;
               break ;

            case MON_LOB_READ :
               totalLobRead += delta ;
               break ;

            case MON_DATA_WRITE :
               totalDataWrite += delta ;
               break ;

            case MON_INDEX_WRITE :
               totalIndexWrite += delta ;
               break ;

            case MON_LOB_WRITE :
               totalLobWrite += delta ;
               break ;

            case MON_UPDATE :
               totalUpdate += delta ;
               break ;

            case MON_DELETE :
               totalDelete += delta ;
               break ;

            case MON_INSERT :
               totalInsert += delta ;
               break ;

            case MON_SELECT :
               totalSelect += delta ;
               break ;

            case MON_READ :
               totalRead += delta ;
               break ;

            default:
               break ;
         }
         mondbcb->monOperationCountInc( op, delta ) ;
      }

      _monAppCB() ;
      _monAppCB &operator= ( const _monAppCB &rhs ) ;
      _monAppCB &operator+= ( const _monAppCB &rhs ) ;

      void reset() ;

      void recordConnectTimestamp()
      {
         ossGetCurrentTime( _connectTimestamp ) ;
      }

      void startOperator() ;
      void endOperator() ;
      void setLastOpType( INT32 opType ) ;
      void setLastCmdType( INT32 cmdType ) ;
      void opTimeSpentInc( ossTickDelta delta );
      void saveLastOpDetail( const CHAR *format, ... ) ;

   } ;
   typedef _monAppCB  monAppCB ;

   /*
      _monContextCB define
    */
   class _monContextCB : public SDBObject
   {
      public :
         _monContextCB () ;
         _monContextCB ( const _monContextCB & monCtxCB ) ;
         ~_monContextCB () ;

         void reset () ;

         _monContextCB & operator = ( const _monContextCB & monCtxCB ) ;

         OSS_INLINE INT64 getContextID () const
         {
            return _contextID ;
         }

         OSS_INLINE void setContextID ( INT64 contextID )
         {
            _contextID = contextID ;
         }

         OSS_INLINE UINT64 getDataRead () const
         {
            return _dataRead ;
         }

         OSS_INLINE void setDataRead ( UINT64 dataRead )
         {
            _dataRead = dataRead ;
         }

         OSS_INLINE UINT64 getIndexRead () const
         {
            return _indexRead ;
         }

         OSS_INLINE void setIndexRead ( UINT64 indexRead )
         {
            _indexRead = indexRead ;
         }

         OSS_INLINE UINT64 getLobRead() const
         {
            return _lobRead ;
         }

         OSS_INLINE void setLobRead( UINT64 lobRead )
         {
            _lobRead = lobRead ;
         }

         OSS_INLINE UINT64 getLobWrite() const
         {
            return _lobWrite ;
         }

         OSS_INLINE void setLobWrite( UINT64 lobWrite )
         {
            _lobWrite = lobWrite ;
         }

         OSS_INLINE UINT32 getReturnBatches () const
         {
            return _returnBatches ;
         }

         OSS_INLINE void setReturnBatches ( UINT32 returnBatches )
         {
            _returnBatches = returnBatches ;
         }

         OSS_INLINE UINT64 getReturnRecords () const
         {
            return _returnRecords ;
         }

         OSS_INLINE void setReturnRecords ( UINT64 returnRecords )
         {
            _returnRecords = returnRecords ;
         }

         OSS_INLINE const ossTimestamp & getStartTimestamp () const
         {
            return _startTimestamp ;
         }

         OSS_INLINE void setStartTimestamp ( const ossTimestamp & startTimestamp )
         {
            _startTimestamp = startTimestamp ;
         }

         OSS_INLINE const ossTickDelta & getWaitTime () const
         {
            return _waitTime ;
         }

         OSS_INLINE void setWaitTime ( const ossTickDelta & waitTime )
         {
            _waitTime = waitTime ;
         }

         OSS_INLINE const ossTickDelta & getQueryTime () const
         {
            return _queryTime ;
         }

         OSS_INLINE void setQueryTime ( const ossTickDelta & queryTime )
         {
            _queryTime = queryTime ;
         }

         OSS_INLINE const ossTickDelta & getExecuteTime () const
         {
            return _executeTime ;
         }

         OSS_INLINE void setExecuteTime ( const ossTickDelta & executeTime )
         {
            _executeTime = executeTime ;
         }

         OSS_INLINE void recordStartTimestamp ()
         {
            ossGetCurrentTime( _startTimestamp ) ;
         }

         OSS_INLINE void monReturnInc ( UINT32 batchDelta,
                                        UINT64 recordDelta )
         {
            _returnBatches += batchDelta ;
            _returnRecords += recordDelta ;
         }

         OSS_INLINE void monOperationCountInc ( MON_OPERATION_TYPES op,
                                                UINT64 delta = 1 )
         {
            switch ( op )
            {
               case MON_DATA_READ :
                  monDataReadInc( delta ) ;
                  break ;
               case MON_INDEX_READ :
                  monIndexReadInc( delta ) ;
                  break ;
               case MON_LOB_READ :
                  monLobReadInc( delta ) ;
                  break ;
               case MON_LOB_WRITE :
                  monLobWriteInc( delta ) ;
                  break ;
               default:
                  break ;
            }
         }

         OSS_INLINE void monDataReadInc ( UINT64 delta )
         {
            _dataRead += delta ;
         }

         OSS_INLINE void monIndexReadInc ( UINT64 delta )
         {
            _indexRead += delta ;
         }

         OSS_INLINE void monLobReadInc( UINT64 delta )
         {
            _lobRead += delta ;
         }

         OSS_INLINE void monLobWriteInc( UINT64 delta )
         {
            _lobWrite += delta ;
         }

         OSS_INLINE void monOperationTimeInc ( MON_OPERATION_TYPES op,
                                               ossTickDelta &delta )
         {
            switch ( op )
            {
               case MON_TOTAL_WAIT_TIME :
                  monWaitTimeInc( delta ) ;
                  break ;
               case MON_TOTAL_READ_TIME :
                  monQueryTimeInc( delta ) ;
                  break ;
               case MON_TOTAL_WRITE_TIME :
                  monExecuteTimeInc( delta ) ;
                  break ;
               default :
                  break ;
            }
         }

         OSS_INLINE void monOperationTimeInc ( MON_OPERATION_TYPES op,
                                               const ossTick &startTime,
                                               const ossTick &endTime )
         {
            ossTickDelta delta = endTime - startTime ;
            monOperationTimeInc( op, delta ) ;
         }

         OSS_INLINE void monWaitTimeInc ( ossTickDelta &delta )
         {
            _waitTime += delta ;
         }

         OSS_INLINE void monWaitTimeInc ( const ossTick &startTime,
                                          const ossTick &endTime )
         {
            ossTickDelta delta = endTime - startTime ;
            monWaitTimeInc( delta ) ;
         }

         OSS_INLINE void monQueryTimeInc ( ossTickDelta &delta )
         {
            _queryTime += delta ;
         }

         OSS_INLINE void monQueryTimeInc ( const ossTick &startTime,
                                           const ossTick &endTime )
         {
            ossTickDelta delta = endTime - startTime ;
            monQueryTimeInc( delta ) ;
         }

         OSS_INLINE void monExecuteTimeInc ( ossTickDelta &delta )
         {
            _executeTime += delta ;
         }

         OSS_INLINE void monExecuteTimeInc ( const ossTick &startTime,
                                             const ossTick &endTime )
         {
            ossTickDelta delta = endTime - startTime ;
            monExecuteTimeInc( delta ) ;
         }

      public :
         INT64          _contextID ;
         UINT64         _dataRead ;
         UINT64         _indexRead ;
         UINT64         _lobRead ;
         UINT64         _lobWrite ;
         UINT32         _returnBatches ;
         UINT64         _returnRecords ;
         ossTimestamp   _startTimestamp ;
         ossTickDelta   _waitTime ;
         ossTickDelta   _queryTime ;
         ossTickDelta   _executeTime ;
   } ;

   typedef _monContextCB  monContextCB ;

}

#endif // MONCB_HPP_

