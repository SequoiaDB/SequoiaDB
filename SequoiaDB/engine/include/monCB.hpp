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
#include "rtnQueryOptions.hpp"

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
            _pMonAppCB_->setUnknownCmdType() ;                        \
            _pMonAppCB_->saveLastOpDetail( format,                   \
                                           ##__VA_ARGS__ ) ;         \
         } catch( ... ) {}                                           \
      }                                                              \
   }

   #define MON_CLEAR_OP_DETAIL( _pMonAppCB_ )                        \
   {                                                                 \
      if ( NULL != _pMonAppCB_ )                                     \
      {                                                              \
         try {                                                       \
            _pMonAppCB_->setLastOpType( 0 ) ;                        \
            _pMonAppCB_->setLastCmdType( CMD_UNKNOW ) ;              \
            _pMonAppCB_->clearLastOpDetail() ;                       \
         } catch ( ...) {}                                           \
      }                                                              \
   }

   #define MON_REPLACE_OP_DETAIL( _pMonAppCB_, opType, detail )      \
   {                                                                 \
      if ( NULL != _pMonAppCB_ )                                     \
      {                                                              \
         try {                                                       \
            _pMonAppCB_->setLastOpType( opType ) ;                   \
            _pMonAppCB_->setUnknownCmdType() ;                       \
            _pMonAppCB_->replaceLastOpDetail( detail ) ;             \
         } catch ( ... ) {}                                          \
      }                                                              \
   }

   #define MON_SAVE_OP_OPTION( _pMonAppCB_, msg, options )           \
   {                                                                 \
      if ( NULL != _pMonAppCB_ )                                     \
      {                                                              \
         try {                                                       \
            _pMonAppCB_->setLastOpType( msg->opCode ) ;              \
            _pMonAppCB_->setUnknownCmdType() ;                        \
            _pMonAppCB_->saveLastOpQuery( msg, options ) ;           \
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

   #define MON_APP_DELTA( begin, end ) \
      ( ( end ) < ( begin ) ? 0 : ( end ) - ( begin ) )

   #define MON_APP_TICK_DELTA( begin, end ) \
      ( ( end ) < ( begin ) ? ( ossTickDelta () ) : ( end ) - ( begin ) )

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
      MON_TEMP_READ,
      MON_DATA_WRITE,
      MON_INDEX_WRITE,
      MON_TEMP_WRITE,
      MON_UPDATE,
      MON_DELETE,
      MON_INSERT,
      MON_UPDATE_REPL,
      MON_DELETE_REPL,
      MON_INSERT_REPL,
      MON_SELECT,
      MON_READ,
      MON_TRANS_COMMIT,
      MON_TRANS_ROLLBACK,
      MON_GENERAL_QUERY,
      MON_GENERAL_SLOW_QUERY,
      MON_LOB_GET,
      MON_LOB_PUT,
      MON_LOB_DELETE,
      MON_LOB_LIST,
      MON_LOB_READ,
      MON_LOB_WRITE,
      MON_LOB_TRUNCATE,
      MON_LOB_ADDRESSING,
      MON_COUNTER_OPERATION_MAX = MON_LOB_ADDRESSING,

      MON_TIME_OPERATION_NONE = 1000,
      MON_TOTAL_WAIT_TIME,
      MON_TOTAL_READ_TIME,
      MON_TOTAL_WRITE_TIME,
      MON_TIME_OPERATION_MAX = MON_TOTAL_WRITE_TIME,

      MON_BYTE_COUNT_NONE = 2000,
      MON_LOB_READ_BYTES,
      MON_LOB_WRITE_BYTES,
      MON_BYTE_COUNT_MAX  = MON_LOB_WRITE_BYTES
   } ;

   enum MON_UPDATE_CLASS
   {
      MON_UPDATE_NONE       = 0x00000000,
      MON_UPDATE_CONTEXT    = 0x00000001,
      MON_UPDATE_SESSION    = 0x00000002,
      MON_UPDATE_CL         = 0x00000004,
      MON_UPDATE_CS         = 0x00000008,
      MON_UPDATE_SDB        = 0x00000010,
      MON_UPDATE_TASK       = 0x00000020
   } ;

   enum MON_LOB_OP_TYPES
   {
      MON_LOB_OP_NONE       = 0x00000000,
      MON_LOB_OP_PUT        = 0x00000001,
      MON_LOB_OP_GET        = 0x00000002,
      MON_LOB_OP_DELETE     = 0x00000004,
      MON_LOB_OP_LIST       = 0x00000008
   } ;

   #define MON_UPDATE_CLASS_DEFAULT ( MON_UPDATE_SESSION | MON_UPDATE_SDB | \
                                      MON_UPDATE_TASK | MON_UPDATE_CL )

   class _monAppCB ;

   /*
      _monRuntimeCB define
    */
   class _monCRUDCB : public SDBObject
   {
      public :
         _monCRUDCB ()
         : _totalDataRead( 0 ),
           _totalIndexRead( 0 ),
           _totalDataWrite( 0 ),
           _totalIndexWrite( 0 ),
           _totalUpdate( 0 ),
           _totalDelete( 0 ),
           _totalInsert( 0 ),
           _totalSelect( 0 ),
           _totalRead( 0 ),
           _totalWrite( 0 ),
           _totalTbScan( 0 ),
           _totalIxScan( 0 ),
           _totalLobGet( 0 ),
           _totalLobPut( 0 ),
           _totalLobDelete( 0 ),
           _totalLobList( 0 ),
           _totalLobReadSize( 0 ),
           _totalLobWriteSize( 0 ),
           _totalLobRead( 0 ),
           _totalLobWrite( 0 ),
           _totalLobTruncate( 0 ),
           _totalLobAddressing( 0 )
         {
            ossGetCurrentTime( _resetTimestamp ) ;
         }

         _monCRUDCB ( const _monCRUDCB & monCB )
         : _totalDataRead( monCB._totalDataRead ),
           _totalIndexRead( monCB._totalIndexRead ),
           _totalDataWrite( monCB._totalDataWrite ),
           _totalIndexWrite( monCB._totalIndexWrite ),
           _totalUpdate( monCB._totalUpdate ),
           _totalDelete( monCB._totalDelete ),
           _totalInsert( monCB._totalInsert ),
           _totalSelect( monCB._totalSelect ),
           _totalRead( monCB._totalRead ),
           _totalWrite( monCB._totalWrite ),
           _totalTbScan( monCB._totalTbScan ),
           _totalIxScan( monCB._totalIxScan ),
           _totalLobGet( monCB._totalLobGet ),
           _totalLobPut( monCB._totalLobPut ),
           _totalLobDelete( monCB._totalLobDelete ),
           _totalLobList( monCB._totalLobList ),
           _totalLobReadSize( monCB._totalLobReadSize ),
           _totalLobWriteSize( monCB._totalLobWriteSize ),
           _totalLobRead( monCB._totalLobRead ),
           _totalLobWrite( monCB._totalLobWrite ),
           _totalLobTruncate( monCB._totalLobTruncate ),
           _totalLobAddressing( monCB._totalLobAddressing ),
           _resetTimestamp( monCB._resetTimestamp )
         {
         }

         ~_monCRUDCB ()
         {
         }

      public :
         OSS_INLINE void reset ()
         {
            _totalDataRead = 0 ;
            _totalIndexRead = 0 ;
            _totalDataWrite = 0 ;
            _totalIndexWrite = 0 ;
            _totalUpdate = 0 ;
            _totalDelete = 0 ;
            _totalInsert = 0 ;
            _totalSelect = 0 ;
            _totalRead = 0 ;
            _totalWrite = 0 ;
            _totalTbScan = 0 ;
            _totalIxScan = 0 ;
            _totalLobGet = 0 ;
            _totalLobPut = 0 ;
            _totalLobDelete = 0 ;
            _totalLobList = 0 ;
            _totalLobReadSize = 0 ;
            _totalLobWriteSize = 0 ;
            _totalLobRead = 0 ;
            _totalLobWrite = 0 ;
            _totalLobTruncate = 0 ;
            _totalLobAddressing = 0 ;
            ossGetCurrentTime( _resetTimestamp ) ;
         }

         OSS_INLINE void resetOnce ()
         {
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalDataRead ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalIndexRead ), 0 ) ;

            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalDataWrite ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalIndexWrite ), 0 ) ;

            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalUpdate ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalDelete ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalInsert ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalSelect ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalRead ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalWrite ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalTbScan ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalIxScan ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobGet ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobPut ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobDelete ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobList ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobReadSize ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobWriteSize ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobRead ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobWrite ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobTruncate ), 0 ) ;
            ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobAddressing ), 0 ) ;
            ossGetCurrentTime( _resetTimestamp ) ;
         }

         OSS_INLINE void set ( const _monCRUDCB & monCB )
         {
            _totalDataRead = monCB._totalDataRead ;
            _totalIndexRead = monCB._totalIndexRead ;

            _totalDataWrite = monCB._totalDataWrite ;
            _totalIndexWrite = monCB._totalIndexWrite ;

            _totalUpdate = monCB._totalUpdate ;
            _totalDelete = monCB._totalDelete ;
            _totalInsert = monCB._totalInsert ;
            _totalSelect = monCB._totalSelect ;
            _totalRead = monCB._totalRead ;
            _totalWrite = monCB._totalWrite ;
            _totalTbScan = monCB._totalTbScan ;
            _totalIxScan = monCB._totalIxScan ;
            _totalLobGet = monCB._totalLobGet ;
            _totalLobPut = monCB._totalLobPut ;
            _totalLobDelete = monCB._totalLobDelete ;
            _totalLobList = monCB._totalLobList ;
            _totalLobReadSize = monCB._totalLobReadSize ;
            _totalLobWriteSize = monCB._totalLobWriteSize ;
            _totalLobRead = monCB._totalLobRead ;
            _totalLobWrite = monCB._totalLobWrite ;
            _totalLobTruncate = monCB._totalLobTruncate ;
            _totalLobAddressing = monCB._totalLobAddressing ;

            _resetTimestamp = monCB._resetTimestamp ;
         }

         OSS_INLINE void setFromOnce ( const _monCRUDCB & monCB )
         {
            _totalDataRead = OSS_ONCE_UINT64_GET( monCB._totalDataRead ) ;
            _totalIndexRead = OSS_ONCE_UINT64_GET( monCB._totalIndexRead ) ;
            _totalDataWrite = OSS_ONCE_UINT64_GET( monCB._totalDataWrite ) ;
            _totalIndexWrite = OSS_ONCE_UINT64_GET( monCB._totalIndexWrite ) ;
            _totalUpdate = OSS_ONCE_UINT64_GET( monCB._totalUpdate ) ;
            _totalDelete = OSS_ONCE_UINT64_GET( monCB._totalDelete ) ;
            _totalInsert = OSS_ONCE_UINT64_GET( monCB._totalInsert ) ;
            _totalSelect = OSS_ONCE_UINT64_GET( monCB._totalSelect ) ;
            _totalRead = OSS_ONCE_UINT64_GET( monCB._totalRead ) ;
            _totalWrite = OSS_ONCE_UINT64_GET( monCB._totalWrite ) ;
            _totalTbScan = OSS_ONCE_UINT64_GET( monCB._totalTbScan ) ;
            _totalIxScan = OSS_ONCE_UINT64_GET( monCB._totalIxScan ) ;
            _totalLobGet = OSS_ONCE_UINT64_GET( monCB._totalLobGet ) ;
            _totalLobPut = OSS_ONCE_UINT64_GET( monCB._totalLobPut ) ;
            _totalLobDelete = OSS_ONCE_UINT64_GET( monCB._totalLobDelete ) ;
            _totalLobList = OSS_ONCE_UINT64_GET( monCB._totalLobList ) ;
            _totalLobReadSize = OSS_ONCE_UINT64_GET( monCB._totalLobReadSize ) ;
            _totalLobWriteSize = OSS_ONCE_UINT64_GET( monCB._totalLobWriteSize ) ;
            _totalLobRead = OSS_ONCE_UINT64_GET( monCB._totalLobRead ) ;
            _totalLobWrite = OSS_ONCE_UINT64_GET( monCB._totalLobWrite ) ;
            _totalLobTruncate = OSS_ONCE_UINT64_GET( monCB._totalLobTruncate ) ;
            _totalLobAddressing = OSS_ONCE_UINT64_GET( monCB._totalLobAddressing ) ;

            _resetTimestamp = monCB._resetTimestamp ;
         }

         void increase ( MON_OPERATION_TYPES op, UINT64 delta = 1 )
         {
            switch ( op )
            {
               case MON_DATA_READ :
                  _totalDataRead += delta ;
                  break ;
               case MON_INDEX_READ :
                  _totalIndexRead += delta ;
                  break ;
               case MON_DATA_WRITE :
                  _totalDataWrite += delta ;
                  break ;
               case MON_INDEX_WRITE :
                  _totalIndexWrite += delta ;
                  break ;
               case MON_UPDATE :
                  _totalUpdate += delta ;
                  _totalWrite += delta ;
                  break ;
               case MON_DELETE :
                  _totalDelete += delta ;
                  _totalWrite += delta ;
                  break ;
               case MON_INSERT :
                  _totalInsert += delta ;
                  _totalWrite += delta ;
                  break ;
               case MON_SELECT :
                  _totalSelect += delta ;
                  break ;
               case MON_READ :
                  _totalRead += delta ;
                  break ;
               case MON_LOB_GET :
                  _totalLobGet += delta ;
                  break ;
               case MON_LOB_PUT :
                  _totalLobPut += delta ;
                  break ;
               case MON_LOB_DELETE :
                  _totalLobDelete += delta ;
                  break ;
               case MON_LOB_LIST:
                  _totalLobList += delta ;
                  break ;
               case MON_LOB_READ_BYTES :
                  _totalLobReadSize += delta ;
                  break ;
               case MON_LOB_WRITE_BYTES :
                  _totalLobWriteSize += delta ;
                  break ;
               case MON_LOB_READ :
                  _totalLobRead += delta ;
                  break ;
               case MON_LOB_WRITE :
                  _totalLobWrite += delta ;
                  break ;
               case MON_LOB_TRUNCATE:
                  _totalLobTruncate += delta ;
                  break ;
               case MON_LOB_ADDRESSING :
                  _totalLobAddressing += delta ;
                  break ;
               default :
                  break ;
            }
         }

         void increaseOnce ( MON_OPERATION_TYPES op, UINT64 delta = 1 )
         {
            switch ( op )
            {
               case MON_DATA_READ :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalDataRead ),
                                    delta ) ;
                  break ;
               case MON_INDEX_READ :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalIndexRead ),
                                    delta ) ;
                  break ;
               case MON_DATA_WRITE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalDataWrite ),
                                    delta ) ;
                  break ;
               case MON_INDEX_WRITE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalIndexWrite ),
                                    delta ) ;
                  break ;
               case MON_UPDATE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalUpdate ),
                                    delta ) ;
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalWrite ),
                                    delta ) ;
                  break ;
               case MON_DELETE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalDelete ),
                                    delta ) ;
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalWrite ),
                                    delta ) ;
                  break ;
               case MON_INSERT :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalInsert ),
                                    delta ) ;
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalWrite ),
                                    delta ) ;
                  break ;
               case MON_SELECT :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalSelect ),
                                    delta ) ;
                  break ;
               case MON_READ :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalRead ),
                                    delta ) ;
                  break ;
               case MON_LOB_GET :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobGet ),
                                    delta ) ;
                  break ;
               case MON_LOB_PUT :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobPut ),
                                    delta ) ;
                  break ;
               case MON_LOB_DELETE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobDelete ),
                                    delta ) ;
                  break ;
               case MON_LOB_LIST :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobList ),
                                    delta ) ;
                  break ;
               case MON_LOB_READ_BYTES :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobReadSize ),
                                    delta ) ;
                  break ;
               case MON_LOB_WRITE_BYTES :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobWriteSize ),
                                    delta ) ;
                  break ;
               case MON_LOB_READ :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobRead ),
                                    delta ) ;
                  break ;
               case MON_LOB_WRITE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobWrite ),
                                    delta ) ;
                  break ;
               case MON_LOB_TRUNCATE :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobTruncate ),
                                    delta ) ;
                  break ;
               case MON_LOB_ADDRESSING :
                  ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobAddressing ),
                                    delta ) ;
                  break ;

               default :
                  break ;
            }
         }

         OSS_INLINE void increaseTbScan ( UINT64 delta )
         {
            ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalTbScan ), delta ) ;
         }

         OSS_INLINE void increaseIxScan ( UINT64 delta )
         {
            ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalIxScan ), delta ) ;
         }

         void incMetrics( const _monAppCB &delta ) ;

      private:
         void _increase( MON_OPERATION_TYPES op, UINT64 delta ) ;

      public :
         UINT64 _totalDataRead ;
         UINT64 _totalIndexRead ;
         UINT64 _totalDataWrite ;
         UINT64 _totalIndexWrite ;

         UINT64 _totalUpdate ;
         UINT64 _totalDelete ;
         UINT64 _totalInsert ;
         UINT64 _totalSelect ;
         UINT64 _totalRead ;
         UINT64 _totalWrite ;

         UINT64 _totalTbScan ;
         UINT64 _totalIxScan ;

         UINT64 _totalLobGet ;
         UINT64 _totalLobPut ;
         UINT64 _totalLobDelete ;
         UINT64 _totalLobList ;
         UINT64 _totalLobReadSize ;
         UINT64 _totalLobWriteSize ;
         UINT64 _totalLobRead ;
         UINT64 _totalLobWrite ;
         UINT64 _totalLobTruncate ;
         UINT64 _totalLobAddressing ;

         ossTimestamp _resetTimestamp ;
   } ;

   typedef class _monCRUDCB monCRUDCB ;

   /*
      _monDBCB define
   */
   class _monDBCB : public SDBObject
   {
   public :
      volatile UINT64 totalDataRead ;
      volatile UINT64 totalIndexRead ;
      volatile UINT64 totalDataWrite ;
      volatile UINT64 totalIndexWrite ;

      volatile UINT64 totalUpdate ;
      volatile UINT64 totalDelete ;
      volatile UINT64 totalInsert ;
      volatile UINT64 totalSelect ;  // total records into result set
      volatile UINT64 totalRead ;    // total records readed from disk
      volatile UINT64 totalGeneralQuery ; // Generalized query operation,
                                          // including almost any operations
                                          // on the database, except some
                                          // internal operations.
      volatile UINT64 totalGeneralSlowQuery ;
      volatile UINT64 totalTransCommit ;
      volatile UINT64 totalTransRollback ;

      volatile UINT64 totalLobGet ;        // total number of lob read by client
      volatile UINT64 totalLobPut ;        // total number of lob writed by client
      volatile UINT64 totalLobDelete ;     // total number of lob removed/truncated by client
      volatile UINT64 totalLobList ;       // total number of list lob operation by client
      volatile UINT64 totalLobReadSize ;   // total lob bytes read by client
      volatile UINT64 totalLobWriteSize ;  // total lob bytes writed by client
      volatile UINT64 totalLobRead ;       // total number of lob page read in server
      volatile UINT64 totalLobWrite ;      // total number of lob page writed in server
      volatile UINT64 totalLobTruncate ;   // total number of lob page truncated in server
      volatile UINT64 totalLobAddressing ; // total number of addressing in lobm

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
               ossFetchAndAdd64( &totalDataRead, delta ) ;
               break ;

            case MON_INDEX_READ :
               ossFetchAndAdd64( &totalIndexRead, delta ) ;
               break ;

            case MON_DATA_WRITE :
               ossFetchAndAdd64( &totalDataWrite, delta ) ;
               break ;

            case MON_INDEX_WRITE :
               ossFetchAndAdd64( &totalIndexWrite, delta ) ;
               break ;

            case MON_UPDATE :
               ossFetchAndAdd64( &totalUpdate, delta ) ;
               break ;

            case MON_DELETE :
               ossFetchAndAdd64( &totalDelete, delta ) ;
               break ;

            case MON_INSERT :
               ossFetchAndAdd64( &totalInsert, delta ) ;
               break ;

            case MON_UPDATE_REPL :
               ossFetchAndAdd64( &replUpdate, delta ) ;
               break ;

            case MON_DELETE_REPL :
               ossFetchAndAdd64( &replDelete, delta ) ;
               break ;

            case MON_INSERT_REPL :
               ossFetchAndAdd64( &replInsert, delta ) ;
               break ;

            case MON_SELECT :
               ossFetchAndAdd64( &totalSelect, delta ) ;
               break ;

            case MON_READ :
               ossFetchAndAdd64( &totalRead, delta ) ;
               break ;

            case MON_GENERAL_QUERY:
               ossFetchAndAdd64( &totalGeneralQuery, delta ) ;
               break ;

            case MON_GENERAL_SLOW_QUERY:
               ossFetchAndAdd64( &totalGeneralSlowQuery, delta ) ;
               break ;

            case MON_TRANS_COMMIT:
               ossFetchAndAdd64( &totalTransCommit, delta ) ;
               break ;

            case MON_TRANS_ROLLBACK:
               ossFetchAndAdd64( &totalTransRollback, delta ) ;
               break ;

            case MON_LOB_GET :
               ossFetchAndAdd64( &totalLobGet, delta ) ;
               break ;

            case MON_LOB_PUT :
               ossFetchAndAdd64( &totalLobPut, delta ) ;
               break ;

            case MON_LOB_DELETE :
               ossFetchAndAdd64( &totalLobDelete, delta ) ;
               break ;

            case MON_LOB_LIST:
               ossFetchAndAdd64( &totalLobList, delta ) ;
               break ;

            case MON_LOB_READ :
               ossFetchAndAdd64( &totalLobRead, delta ) ;
               break ;

            case MON_LOB_WRITE :
               ossFetchAndAdd64( &totalLobWrite, delta ) ;
               break ;

            case MON_LOB_TRUNCATE :
               ossFetchAndAdd64( &totalLobTruncate, delta ) ;
               break ;

            case MON_LOB_ADDRESSING :
               ossFetchAndAdd64( &totalLobAddressing, delta ) ;
               break ;

            default:
               break ;
         }
      }

      void monByteCountInc( MON_OPERATION_TYPES op, UINT64 delta )
      {
         switch ( op )
         {
            case MON_LOB_READ_BYTES :
               ossFetchAndAdd64( &totalLobReadSize, delta ) ;
               break ;

            case MON_LOB_WRITE_BYTES :
               ossFetchAndAdd64( &totalLobWriteSize, delta ) ;
               break ;

            default:
               break ;
         }
      }

      UINT64 getReceiveNum() { return receiveNum ; }
      void   addReceiveNum()
      {
         ossFetchAndAdd64( &receiveNum, 1 ) ;
      }

      void   svcNetInAdd( INT32 sendSize )
      {
         ossFetchAndAdd64( &_svcNetIn, sendSize ) ;
      }
      void   svcNetOutAdd( INT32 recvSize )
      {
         ossFetchAndAdd64( &_svcNetOut, recvSize ) ;
      }
      UINT64 svcNetIn() { return _svcNetIn ; }
      UINT64 svcNetOut() { return _svcNetOut ; }

      _monDBCB() ;
      _monDBCB& operator= ( const _monDBCB &rhs ) ;
      void   reset() ;
      void   recordActivateTimestamp() ;
      void   incMetrics( const _monAppCB &delta ) ;

   private:
      void   _increase( MON_OPERATION_TYPES op, UINT64 delta ) ;
      void   _increase( MON_OPERATION_TYPES op, ossTickDelta &delta ) ;
   } ;
   typedef _monDBCB monDBCB ;

   #define MON_SVC_TASK_NAME_LEN             ( 127 )
   /*
      _monSvcTaskInfo define
   */
   class _monSvcTaskInfo : public SDBObject
   {
   private:
      UINT64                        _taskID ;
      CHAR                          _taskName[ MON_SVC_TASK_NAME_LEN + 1 ] ;

   public:
      volatile UINT64               _totalTime ;
      volatile UINT64               _totalContexts ;
      volatile UINT64               _totalDataRead ;
      volatile UINT64               _totalIndexRead ;
      volatile UINT64               _totalDataWrite ;
      volatile UINT64               _totalIndexWrite ;
      volatile UINT64               _totalUpdate ;
      volatile UINT64               _totalDelete ;
      volatile UINT64               _totalInsert ;
      volatile UINT64               _totalSelect ;
      volatile UINT64               _totalRead ;
      volatile UINT64               _totalWrite ;

      volatile UINT64               _totalLobGet ;
      volatile UINT64               _totalLobPut ;
      volatile UINT64               _totalLobDelete ;
      volatile UINT64               _totalLobList ;
      volatile UINT64               _totalLobReadSize ;
      volatile UINT64               _totalLobWriteSize ;
      volatile UINT64               _totalLobRead ;
      volatile UINT64               _totalLobWrite ;
      volatile UINT64               _totalLobTruncate ;
      volatile UINT64               _totalLobAddressing ;

      ossTimestamp                  _startTimestamp ;
      ossTimestamp                  _resetTimestamp ;

   public:
      _monSvcTaskInfo() ;

      void setTaskInfo( UINT64 taskID, const CHAR* taskName ) ;

      UINT64         getTaskID() const { return _taskID ; }
      const CHAR*    getTaskName() const { return _taskName ; }

      void monContextInc( INT64 delta = 1 )
      {
         _totalContexts += delta ;
      }

      void monOperationCountInc( MON_OPERATION_TYPES op,
                                 INT64 delta = 1 )
      {
         switch ( op )
         {
            case MON_DATA_READ :
               ossFetchAndAdd64( &_totalDataRead, delta ) ;
               break ;
            case MON_INDEX_READ :
               ossFetchAndAdd64( &_totalIndexRead, delta ) ;
               break ;
            case MON_DATA_WRITE :
               ossFetchAndAdd64( &_totalDataWrite, delta ) ;
               break ;
            case MON_INDEX_WRITE :
               ossFetchAndAdd64( &_totalIndexWrite, delta ) ;
               break ;
            case MON_UPDATE :
               ossFetchAndAdd64( &_totalUpdate, delta ) ;
               ossFetchAndAdd64( &_totalWrite, delta ) ;
               break ;
            case MON_DELETE :
               ossFetchAndAdd64( &_totalDelete, delta ) ;
               ossFetchAndAdd64( &_totalWrite, delta ) ;
               break ;
            case MON_INSERT :
               ossFetchAndAdd64( &_totalInsert, delta ) ;
               ossFetchAndAdd64( &_totalWrite, delta ) ;
               break ;
            case MON_SELECT :
               ossFetchAndAdd64( &_totalSelect, delta ) ;
               break ;
            case MON_READ :
               ossFetchAndAdd64( &_totalRead, delta ) ;
               break ;
            case MON_LOB_GET :
               ossFetchAndAdd64( &_totalLobGet, delta ) ;
               break ;
            case MON_LOB_PUT :
               ossFetchAndAdd64( &_totalLobPut, delta ) ;
               break ;
            case MON_LOB_DELETE :
               ossFetchAndAdd64( &_totalLobDelete, delta ) ;
               break ;
            case MON_LOB_LIST:
               ossFetchAndAdd64( &_totalLobList, delta ) ;
               break ;
            case MON_LOB_READ :
               ossFetchAndAdd64( &_totalLobRead, delta ) ;
               break ;
            case MON_LOB_WRITE :
               ossFetchAndAdd64( &_totalLobWrite, delta ) ;
               break ;
            case MON_LOB_TRUNCATE :
               ossFetchAndAdd64( &_totalLobTruncate, delta ) ;
               break ;
            case MON_LOB_ADDRESSING :
               ossFetchAndAdd64( &_totalLobAddressing, delta ) ;
               break ;
            default:
               break ;
         }
      }

      void monByteCountInc( MON_OPERATION_TYPES op, UINT64 delta )
      {
         switch ( op )
         {
            case MON_LOB_READ_BYTES :
               ossFetchAndAdd64( &_totalLobReadSize, delta ) ;
               break ;
            case MON_LOB_WRITE_BYTES :
               ossFetchAndAdd64( &_totalLobWriteSize, delta ) ;
               break ;
            default:
               break ;
         }
      }

      void monOperationTimeInc( MON_OPERATION_TYPES op,
                                INT64 delta = 1 )
      {
         switch ( op )
         {
            case MON_TOTAL_READ_TIME :
            case MON_TOTAL_WRITE_TIME :
               ossFetchAndAdd64( &_totalTime, delta ) ;
               break ;
            default:
               break ;
         }
      }

      void reset() ;
      void incMetrics( const _monAppCB &delta ) ;

   private:
      void _increase( MON_OPERATION_TYPES op, UINT64 delta ) ;

   } ;
   typedef _monSvcTaskInfo monSvcTaskInfo ;

   void  monSetDefaultTaskInfo( monSvcTaskInfo *pTaskInfo ) ;

   /*
      _monAppCB define
   */
   class _monAppCB : public SDBObject
   {
   private:
      monSvcTaskInfo    *_taskInfo ;
      monCRUDCB         *_mbCRUDCB ;

   public :
      monDBCB           *mondbcb ;

      UINT64 totalDataRead ;
      UINT64 totalIndexRead ;
      UINT64 totalDataWrite ;
      UINT64 totalIndexWrite ;

      UINT64 totalUpdate ;
      UINT64 totalDelete ;
      UINT64 totalInsert ;
      UINT64 totalSelect ;  // total records into result set
      UINT64 totalRead ;    // total records readed from disk

      UINT64 totalGeneralQuery ;
      UINT64 totalGeneralSlowQuery ;

      UINT64 totalTransCommit ;
      UINT64 totalTransRollback ;

      UINT64 totalLobGet ;        // total number of lob read by client
      UINT64 totalLobPut ;        // total number of lob writed by client
      UINT64 totalLobDelete ;     // total number of lob removed/truncated by client
      UINT64 totalLobList ;       // total number of list lob operation by client
      UINT64 totalLobReadSize ;   // total lob bytes read by client
      UINT64 totalLobWriteSize ;  // total lob bytes writed by client
      UINT64 totalLobRead ;       // total number of lob page read in server
      UINT64 totalLobWrite ;      // total number of lob page writed in server
      UINT64 totalLobTruncate ;   // total number of lob page truncated in server
      UINT64 totalLobAddressing ; // total number of lob addressing in server

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
      BOOLEAN        _lastOpMsgSaved ;

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

      void monOperationCountInc( MON_OPERATION_TYPES op,
                                 UINT64 delta = 1,
                                 INT32 updateClass = MON_UPDATE_CLASS_DEFAULT )
      {
         if ( updateClass & MON_UPDATE_SESSION )
         {
            switch ( op )
            {
               case MON_DATA_READ :
                  totalDataRead += delta ;
                  break ;

               case MON_INDEX_READ :
                  totalIndexRead += delta ;
                  break ;

               case MON_DATA_WRITE :
                  totalDataWrite += delta ;
                  break ;

               case MON_INDEX_WRITE :
                  totalIndexWrite += delta ;
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

               case MON_GENERAL_QUERY:
                  totalGeneralQuery += delta ;
                  break ;

               case MON_GENERAL_SLOW_QUERY:
                  totalGeneralSlowQuery += delta ;
                  break ;

               case MON_TRANS_COMMIT:
                  totalTransCommit += delta ;
                  break ;

               case MON_TRANS_ROLLBACK:
                  totalTransRollback += delta ;
                  break ;

               case MON_LOB_GET :
                  totalLobGet += delta ;
                  break ;

               case MON_LOB_PUT :
                  totalLobPut += delta ;
                  break ;

               case MON_LOB_DELETE :
                  totalLobDelete += delta ;
                  break ;

               case MON_LOB_LIST:
                  totalLobList += delta ;
                  break ;

               case MON_LOB_READ :
                  totalLobRead += delta ;
                  break ;

               case MON_LOB_WRITE :
                  totalLobWrite += delta ;
                  break ;

               case MON_LOB_TRUNCATE :
                  totalLobTruncate += delta ;
                  break ;

               case MON_LOB_ADDRESSING :
                  totalLobAddressing += delta ;
                  break ;

               default:
                  break ;
            }
         }
         if ( updateClass & MON_UPDATE_SDB )
         {
            mondbcb->monOperationCountInc( op, delta ) ;
         }
         if ( _taskInfo && ( updateClass & MON_UPDATE_TASK ) )
         {
            _taskInfo->monOperationCountInc( op, delta ) ;
         }
         if ( _mbCRUDCB && ( updateClass & MON_UPDATE_CL ) )
         {
            _mbCRUDCB->increaseOnce( op, delta ) ;
         }
      }

      void monByteCountInc( MON_OPERATION_TYPES op, UINT64 delta,
                            INT32 updateClass = MON_UPDATE_CLASS_DEFAULT )
      {
         if ( updateClass & MON_UPDATE_SESSION )
         {
            switch ( op )
            {
               case MON_LOB_READ_BYTES :
                  totalLobReadSize += delta ;
                  break ;

               case MON_LOB_WRITE_BYTES :
                  totalLobWriteSize += delta ;
                  break ;

               default:
                  break ;
            }
         }
         if ( updateClass & MON_UPDATE_SDB )
         {
            mondbcb->monByteCountInc( op, delta ) ;
         }
         if ( _taskInfo && ( updateClass & MON_UPDATE_TASK ) )
         {
            _taskInfo->monByteCountInc( op, delta ) ;
         }
         if ( _mbCRUDCB && ( updateClass & MON_UPDATE_CL ) )
         {
            _mbCRUDCB->increaseOnce( op, delta ) ;
         }
      }

      _monAppCB() ;
      _monAppCB( const _monAppCB &monApp ) ;
      _monAppCB &operator= ( const _monAppCB &rhs ) ;
      _monAppCB &operator+= ( const _monAppCB &rhs ) ;
      const _monAppCB operator- ( const _monAppCB &rhs ) ;

      void setSvcTaskInfo( monSvcTaskInfo *pSvcTaskInfo ) ;
      monSvcTaskInfo* getSvcTaskInfo() ;

      void reset() ;

      void recordConnectTimestamp()
      {
         ossGetCurrentTime( _connectTimestamp ) ;
      }

      void startOperator() ;
      void endOperator() ;
      void setLastOpType( INT32 opType ) ;
      void setLastCmdType( INT32 cmdType ) ;
      void setUnknownCmdType() ;
      void opTimeSpentInc( ossTickDelta delta );
      const CHAR * getLastOpDetail() ;
      void saveLastOpQuery( const MsgHeader *message,
                            const rtnQueryOptions &options ) ;
      void saveLastOpDetail( const CHAR *format, ... ) ;
      void replaceLastOpDetail( const CHAR *detail ) ;
      void clearLastOpDetail() ;
      void formatLastOpDetail( const rtnQueryOptions &options ) ;
      void formatLastOpDetail( const MsgHeader *message,
                               INT32 expectingOpType ) ;

      OSS_INLINE void setCRUDCB ( monCRUDCB * crudCB )
      {
         _mbCRUDCB = crudCB ;
      }

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

         OSS_INLINE UINT64 getIndexRead () const
         {
            return _indexRead ;
         }

         OSS_INLINE UINT64 getDataWrite () const
         {
            return _dataWrite ;
         }

         OSS_INLINE UINT64 getIndexWrite () const
         {
            return _indexWrite ;
         }

         OSS_INLINE UINT64 getLobRead() const
         {
            return _lobRead ;
         }

         OSS_INLINE UINT64 getLobWrite() const
         {
            return _lobWrite ;
         }

         OSS_INLINE UINT64 getLobTruncate() const
         {
            return _lobTruncate ;
         }

         OSS_INLINE UINT64 getLobAddressing() const
         {
            return _lobAddressing ;
         }

         OSS_INLINE UINT32 getReturnBatches () const
         {
            return _returnBatches ;
         }

         OSS_INLINE UINT64 getReturnRecords () const
         {
            return _returnRecords ;
         }

         OSS_INLINE const ossTimestamp & getStartTimestamp () const
         {
            return _startTimestamp ;
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

         OSS_INLINE void monDataReadInc ( UINT64 delta )
         {
            _dataRead += delta ;
         }

         OSS_INLINE void monIndexReadInc ( UINT64 delta )
         {
            _indexRead += delta ;
         }

         OSS_INLINE void monDataWriteInc ( UINT64 delta )
         {
            _dataWrite += delta ;
         }

         OSS_INLINE void monIndexWriteInc ( UINT64 delta )
         {
            _indexWrite += delta ;
         }

         OSS_INLINE void monLobReadInc( UINT64 delta )
         {
            _lobRead += delta ;
         }

         OSS_INLINE void monLobWriteInc( UINT64 delta )
         {
            _lobWrite += delta ;
         }

         OSS_INLINE void monLobTruncateInc( UINT64 delta )
         {
            _lobTruncate += delta ;
         }

         OSS_INLINE void monLobAddressingInc( UINT64 delta )
         {
            _lobAddressing += delta ;
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

         void incMetrics( const _monAppCB &delta ) ;

      private:
         void _increase( MON_OPERATION_TYPES op, UINT64 delta ) ;

      public :
         INT64          _contextID ;
         UINT64         _dataRead ;
         UINT64         _indexRead ;
         UINT64         _dataWrite ;
         UINT64         _indexWrite ;
         UINT64         _lobRead ;
         UINT64         _lobWrite ;
         UINT64         _lobTruncate ;
         UINT64         _lobAddressing ;
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

