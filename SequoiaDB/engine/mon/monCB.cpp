/******************************************************************************


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

   Source File Name = monCB.cpp

   Descriptive Name = Monitor Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains structure for
   database, application and context snapshot.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "monCB.hpp"
#include "pmd.hpp"
#include "rtnCommand.hpp"

namespace engine
{

   /*
      Global functions
   */
   static monSvcTaskInfo** _monGetDefaultTaskInfo()
   {
      static monSvcTaskInfo *s_pDfaultTaskInfo = NULL ;
      return &s_pDfaultTaskInfo ;
   }

   void monSetDefaultTaskInfo( monSvcTaskInfo *pTaskInfo )
   {
      *_monGetDefaultTaskInfo() = pTaskInfo ;
   }

   /*
      _monCRUDCB implement
   */

   void _monCRUDCB::_increase( MON_OPERATION_TYPES op, UINT64 delta )
   {
      if ( delta <= 0 )
      {
         return ;
      }
      increaseOnce( op, delta ) ;
   }

   void _monCRUDCB::incMetrics( const _monAppCB &delta )
   {
      // FIXME: Temporary shielding the follow metrics to avoid
      // double submit, for they had submitted in real time
      /*
      // update operation count
      _increase( MON_DATA_READ, delta.totalDataRead ) ;
      _increase( MON_INDEX_READ, delta.totalIndexRead ) ;
      _increase( MON_DATA_WRITE, delta.totalDataWrite ) ;
      _increase( MON_INDEX_WRITE, delta.totalIndexWrite ) ;
      _increase( MON_UPDATE, delta.totalUpdate ) ;
      _increase( MON_DELETE, delta.totalDelete ) ;
      _increase( MON_INSERT, delta.totalInsert ) ;
      _increase( MON_SELECT, delta.totalSelect ) ;
      _increase( MON_READ, delta.totalRead ) ;
      */
      _increase( MON_LOB_GET, delta.totalLobGet ) ;
      _increase( MON_LOB_PUT, delta.totalLobPut ) ;
      _increase( MON_LOB_DELETE, delta.totalLobDelete ) ;
      _increase( MON_LOB_LIST, delta.totalLobList ) ;
      _increase( MON_LOB_READ, delta.totalLobRead ) ;
      _increase( MON_LOB_WRITE, delta.totalLobWrite ) ;
      _increase( MON_LOB_TRUNCATE, delta.totalLobTruncate ) ;
      _increase( MON_LOB_ADDRESSING, delta.totalLobAddressing ) ;
      // update byte count
      _increase( MON_LOB_READ_BYTES, delta.totalLobReadSize ) ;
      _increase( MON_LOB_WRITE_BYTES, delta.totalLobWriteSize ) ;
   }

   /*
      _monDBCB implement
   */
   _monDBCB::_monDBCB()
   :_curConns( 0 )
   {
      reset() ;
   }

   void _monDBCB::reset()
   {
      ossAtomicExchange64( &totalDataRead, 0 ) ;
      ossAtomicExchange64( &totalIndexRead, 0 ) ;
      ossAtomicExchange64( &totalDataWrite, 0 ) ;
      ossAtomicExchange64( &totalIndexWrite, 0 ) ;

      ossAtomicExchange64( &totalUpdate, 0 ) ;
      ossAtomicExchange64( &totalDelete, 0 ) ;
      ossAtomicExchange64( &totalInsert, 0 ) ;
      ossAtomicExchange64( &totalSelect, 0 ) ;
      ossAtomicExchange64( &totalRead, 0 ) ;
      ossAtomicExchange64( &totalGeneralQuery, 0 ) ;
      ossAtomicExchange64( &totalGeneralSlowQuery, 0 ) ;
      ossAtomicExchange64( &totalTransCommit, 0 ) ;
      ossAtomicExchange64( &totalTransRollback, 0 ) ;

      ossAtomicExchange64( &totalLobGet, 0 ) ;
      ossAtomicExchange64( &totalLobPut, 0 ) ;
      ossAtomicExchange64( &totalLobDelete, 0 ) ;
      ossAtomicExchange64( &totalLobList, 0 ) ;
      ossAtomicExchange64( &totalLobReadSize, 0 ) ;
      ossAtomicExchange64( &totalLobWriteSize, 0 ) ;
      ossAtomicExchange64( &totalLobRead, 0 ) ;
      ossAtomicExchange64( &totalLobWrite, 0 ) ;
      ossAtomicExchange64( &totalLobTruncate, 0 ) ;
      ossAtomicExchange64( &totalLobAddressing, 0 ) ;

      ossAtomicExchange64( &receiveNum, 0 ) ;

      ossAtomicExchange64( &replUpdate, 0 ) ;
      ossAtomicExchange64( &replInsert, 0 ) ;
      ossAtomicExchange64( &replDelete, 0 ) ;

      ossAtomicExchange64( &_svcNetIn, 0 ) ;
      ossAtomicExchange64( &_svcNetOut, 0 ) ;

      totalReadTime.clear() ;
      totalWriteTime.clear() ;

      ossGetCurrentTime( _resetTimestamp ) ;
   }

   _monDBCB& _monDBCB::operator= ( const _monDBCB &rhs )
   {
      totalDataRead             = rhs.totalDataRead ;
      totalIndexRead            = rhs.totalIndexRead ;
      totalDataWrite            = rhs.totalDataWrite ;
      totalIndexWrite           = rhs.totalIndexWrite ;

      totalUpdate               = rhs.totalUpdate ;
      totalDelete               = rhs.totalDelete ;
      totalInsert               = rhs.totalInsert ;
      totalSelect               = rhs.totalSelect ;
      totalRead                 = rhs.totalRead ;

      totalGeneralQuery         = rhs.totalGeneralQuery ;
      totalGeneralSlowQuery     = rhs.totalGeneralSlowQuery ;
      totalTransCommit          = rhs.totalTransCommit ;
      totalTransRollback        = rhs.totalTransRollback ;

      totalLobGet               = rhs.totalLobGet ;
      totalLobPut               = rhs.totalLobPut ;
      totalLobDelete            = rhs.totalLobDelete ;
      totalLobList              = rhs.totalLobList ;
      totalLobReadSize          = rhs.totalLobReadSize ;
      totalLobWriteSize         = rhs.totalLobWriteSize ;
      totalLobRead              = rhs.totalLobRead ;
      totalLobWrite             = rhs.totalLobWrite ;
      totalLobTruncate          = rhs.totalLobTruncate ;
      totalLobAddressing        = rhs.totalLobAddressing ;
      receiveNum                = rhs.receiveNum ;

      replUpdate                = rhs.replUpdate ;
      replDelete                = rhs.replDelete ;
      replInsert                = rhs.replInsert ;

      _svcNetIn                 = rhs._svcNetIn ;
      _svcNetOut                = rhs._svcNetOut ;

      totalReadTime             = rhs.totalReadTime ;
      totalWriteTime            = rhs.totalWriteTime ;
      _activateTimestamp        = rhs._activateTimestamp ;
      _resetTimestamp           = rhs._resetTimestamp ;

      return *this ;
   }

   void _monDBCB::recordActivateTimestamp()
   {
      ossGetCurrentTime( _activateTimestamp ) ;
      _resetTimestamp = _activateTimestamp ;
   }

   BOOLEAN _monDBCB::isConnLimited( UINT32 maxConn )
   {
      if ( maxConn > 0 && _curConns.fetch() > maxConn )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   void _monDBCB::_increase( MON_OPERATION_TYPES op, UINT64 delta )
   {
      if ( delta <= 0 )
      {
         return ;
      }
      if ( MON_COUNTER_OPERATION_NONE <= op && op <= MON_COUNTER_OPERATION_MAX )
      {
         monOperationCountInc( op, delta ) ;
      }
      else if ( MON_BYTE_COUNT_NONE <= op && op <= MON_BYTE_COUNT_MAX )
      {
         monByteCountInc( op, delta ) ;
      }
   }

   void _monDBCB::_increase( MON_OPERATION_TYPES op, ossTickDelta &delta )
   {
      if ( MON_TIME_OPERATION_NONE <= op && op <= MON_TIME_OPERATION_MAX )
      {
         monOperationTimeInc( op, delta ) ;
      }
   }

   void _monDBCB::incMetrics( const _monAppCB &delta )
   {
      // FIXME: Temporary shielding the follow metrics to avoid
      // double submit, for they had submitted in real time
      /*
      // update operation count
      _increase( MON_DATA_READ, delta.totalDataRead ) ;
      _increase( MON_INDEX_READ, delta.totalIndexRead ) ;
      _increase( MON_DATA_WRITE, delta.totalDataWrite ) ;
      _increase( MON_INDEX_WRITE, delta.totalIndexWrite ) ;
      _increase( MON_UPDATE, delta.totalUpdate ) ;
      _increase( MON_DELETE, delta.totalDelete ) ;
      _increase( MON_INSERT, delta.totalInsert ) ;
      _increase( MON_SELECT, delta.totalSelect ) ;
      _increase( MON_READ, delta.totalRead ) ;
      */
      _increase( MON_LOB_GET, delta.totalLobGet ) ;
      _increase( MON_LOB_PUT, delta.totalLobPut ) ;
      _increase( MON_LOB_DELETE, delta.totalLobDelete ) ;
      _increase( MON_LOB_LIST, delta.totalLobList ) ;
      _increase( MON_LOB_READ, delta.totalLobRead ) ;
      _increase( MON_LOB_WRITE, delta.totalLobWrite ) ;
      _increase( MON_LOB_TRUNCATE, delta.totalLobTruncate ) ;
      _increase( MON_LOB_ADDRESSING, delta.totalLobAddressing ) ;
      // update byte count
      _increase( MON_LOB_READ_BYTES, delta.totalLobReadSize ) ;
      _increase( MON_LOB_WRITE_BYTES, delta.totalLobWriteSize ) ;
   }

   /*
      _monAppCB implement
   */
   _monAppCB::_monAppCB()
   : _mbCRUDCB( NULL )
   {
      reset() ;

      _taskInfo = *_monGetDefaultTaskInfo() ;
      mondbcb = pmdGetKRCB()->getMonDBCB() ;
   }

   _monAppCB::_monAppCB( const _monAppCB &rhs )
   {
      reset() ;
      operator=( rhs ) ;
   }

   _monAppCB &_monAppCB::operator= ( const _monAppCB &rhs )
   {
      mondbcb                   = rhs.mondbcb ;
      _taskInfo                 = rhs._taskInfo ;

      totalDataRead             = rhs.totalDataRead ;
      totalIndexRead            = rhs.totalIndexRead ;
      totalDataWrite            = rhs.totalDataWrite ;
      totalIndexWrite           = rhs.totalIndexWrite ;

      totalUpdate               = rhs.totalUpdate ;
      totalDelete               = rhs.totalDelete ;
      totalInsert               = rhs.totalInsert ;
      totalSelect               = rhs.totalSelect ;
      totalRead                 = rhs.totalRead ;

      totalGeneralQuery         = rhs.totalGeneralQuery ;
      totalGeneralSlowQuery     = rhs.totalGeneralSlowQuery ;
      totalTransCommit          = rhs.totalTransCommit ;
      totalTransRollback        = rhs.totalTransRollback ;

      totalLobGet               = rhs.totalLobGet ;
      totalLobPut               = rhs.totalLobPut ;
      totalLobDelete            = rhs.totalLobDelete ;
      totalLobList              = rhs.totalLobList ;
      totalLobReadSize          = rhs.totalLobReadSize ;
      totalLobWriteSize         = rhs.totalLobWriteSize ;
      totalLobRead              = rhs.totalLobRead ;
      totalLobWrite             = rhs.totalLobWrite ;
      totalLobTruncate          = rhs.totalLobTruncate ;
      totalLobAddressing        = rhs.totalLobAddressing ;

      totalReadTime             = rhs.totalReadTime ;
      totalWriteTime            = rhs.totalWriteTime ;
      _connectTimestamp         = rhs._connectTimestamp ;
      _resetTimestamp           = rhs._resetTimestamp ;

      _lastOpType               = rhs._lastOpType ;
      _cmdType                  = rhs._cmdType ;
      _lastOpBeginTime          = rhs._lastOpBeginTime ;
      _lastOpEndTime            = rhs._lastOpEndTime ;
      _readTimeSpent            = rhs._readTimeSpent ;
      _writeTimeSpent           = rhs._writeTimeSpent ;

      // no matter rhs has message or not, we will format to last op detail
      if ( rhs._lastOpMsgSaved )
      {
         formatLastOpDetail( (const MsgHeader *)( rhs._lastOpDetail ),
                             _lastOpType ) ;
      }
      else
      {
         ossStrncpy( _lastOpDetail, rhs._lastOpDetail, MON_APP_LASTOP_DESC_LEN ) ;
         _lastOpDetail[ MON_APP_LASTOP_DESC_LEN ] = '\0' ;
      }
      _lastOpMsgSaved = FALSE ;

      return *this ;
   }

   _monAppCB &_monAppCB::operator+= ( const _monAppCB &rhs )
   {
      totalDataRead              += rhs.totalDataRead ;
      totalIndexRead             += rhs.totalIndexRead ;
      totalDataWrite             += rhs.totalDataWrite ;
      totalIndexWrite            += rhs.totalIndexWrite ;

      totalUpdate                += rhs.totalUpdate ;
      totalDelete                += rhs.totalDelete ;
      totalInsert                += rhs.totalInsert ;
      totalSelect                += rhs.totalSelect ;
      totalRead                  += rhs.totalRead ;

      totalGeneralQuery          += rhs.totalGeneralQuery ;
      totalGeneralSlowQuery      += rhs.totalGeneralSlowQuery ;
      totalTransCommit           += rhs.totalTransCommit ;
      totalTransRollback         += rhs.totalTransRollback ;

      totalLobGet                += rhs.totalLobGet ;
      totalLobPut                += rhs.totalLobPut ;
      totalLobDelete             += rhs.totalLobDelete ;
      totalLobList               += rhs.totalLobList ;
      totalLobReadSize           += rhs.totalLobReadSize ;
      totalLobWriteSize          += rhs.totalLobWriteSize ;
      totalLobRead               += rhs.totalLobRead ;
      totalLobWrite              += rhs.totalLobWrite ;
      totalLobTruncate           += rhs.totalLobTruncate ;
      totalLobAddressing         += rhs.totalLobAddressing ;
      totalReadTime              += rhs.totalReadTime ;
      totalWriteTime             += rhs.totalWriteTime ;

      _readTimeSpent             += rhs._readTimeSpent ;
      _writeTimeSpent            += rhs._writeTimeSpent ;

      return *this ;
   }

   const _monAppCB _monAppCB::operator- ( const _monAppCB &rhs )
   {
      _monAppCB delta ;

      delta.totalDataRead      = MON_APP_DELTA( rhs.totalDataRead, totalDataRead ) ;
      delta.totalIndexRead     = MON_APP_DELTA( rhs.totalIndexRead, totalIndexRead ) ;
      delta.totalDataWrite     = MON_APP_DELTA( rhs.totalDataWrite, totalDataWrite ) ;
      delta.totalIndexWrite    = MON_APP_DELTA( rhs.totalIndexWrite, totalIndexWrite ) ;
      delta.totalUpdate        = MON_APP_DELTA( rhs.totalUpdate, totalUpdate ) ;
      delta.totalDelete        = MON_APP_DELTA( rhs.totalDelete, totalDelete ) ;
      delta.totalInsert        = MON_APP_DELTA( rhs.totalInsert, totalInsert ) ;
      delta.totalSelect        = MON_APP_DELTA( rhs.totalSelect, totalSelect ) ;
      delta.totalRead          = MON_APP_DELTA( rhs.totalRead, totalRead ) ;

      delta.totalLobGet        = MON_APP_DELTA( rhs.totalLobGet, totalLobGet ) ;
      delta.totalLobPut        = MON_APP_DELTA( rhs.totalLobPut, totalLobPut ) ;
      delta.totalLobDelete     = MON_APP_DELTA( rhs.totalLobDelete, totalLobDelete ) ;
      delta.totalLobList       = MON_APP_DELTA( rhs.totalLobList, totalLobList ) ;
      delta.totalLobReadSize   = MON_APP_DELTA( rhs.totalLobReadSize, totalLobReadSize ) ;
      delta.totalLobWriteSize  = MON_APP_DELTA( rhs.totalLobWriteSize, totalLobWriteSize ) ;
      delta.totalLobRead       = MON_APP_DELTA( rhs.totalLobRead, totalLobRead ) ;
      delta.totalLobWrite      = MON_APP_DELTA( rhs.totalLobWrite, totalLobWrite ) ;
      delta.totalLobTruncate   = MON_APP_DELTA( rhs.totalLobTruncate, totalLobTruncate ) ;
      delta.totalLobAddressing = MON_APP_DELTA( rhs.totalLobAddressing, totalLobAddressing ) ;

      delta.totalReadTime      = MON_APP_TICK_DELTA( rhs.totalReadTime, totalReadTime ) ;
      delta.totalWriteTime     = MON_APP_TICK_DELTA( rhs.totalWriteTime, totalWriteTime ) ;
      delta._readTimeSpent     = MON_APP_TICK_DELTA( rhs._readTimeSpent, _readTimeSpent ) ;
      delta._writeTimeSpent    = MON_APP_TICK_DELTA( rhs._writeTimeSpent, _writeTimeSpent ) ;

      return delta ;
   }

   void _monAppCB::setSvcTaskInfo( monSvcTaskInfo *pSvcTaskInfo )
   {
      if ( pSvcTaskInfo )
      {
         _taskInfo = pSvcTaskInfo ;
      }
      else if ( _taskInfo != *_monGetDefaultTaskInfo() )
      {
         _taskInfo = *_monGetDefaultTaskInfo() ;
      }
   }

   monSvcTaskInfo* _monAppCB::getSvcTaskInfo()
   {
      return _taskInfo ;
   }

   void _monAppCB::reset()
   {
      totalDataRead = 0 ;
      totalIndexRead = 0 ;
      totalDataWrite = 0 ;
      totalIndexWrite = 0 ;

      totalUpdate = 0 ;
      totalDelete = 0 ;
      totalInsert = 0 ;
      totalSelect = 0 ;
      totalRead  = 0 ;

      totalGeneralQuery     = 0 ;
      totalGeneralSlowQuery = 0 ;
      totalTransCommit      = 0 ;
      totalTransRollback    = 0 ;
      totalLobGet           = 0 ;
      totalLobPut           = 0 ;
      totalLobDelete        = 0 ;
      totalLobList          = 0 ;
      totalLobReadSize      = 0 ;
      totalLobWriteSize     = 0 ;
      totalLobRead          = 0 ;
      totalLobWrite         = 0 ;
      totalLobTruncate      = 0 ;
      totalLobAddressing    = 0 ;

      totalReadTime.clear() ;
      totalWriteTime.clear() ;

      ossGetCurrentTime( _resetTimestamp ) ;

      _lastOpType = MSG_NULL ;
      _cmdType = CMD_UNKNOW ;
      _lastOpBeginTime.clear() ;
      _lastOpEndTime.clear() ;
      _readTimeSpent.clear() ;
      _writeTimeSpent.clear() ;
      // reset first 4 bytes to make sure both length of message and
      // first char of formatted detail are reset
      *( (INT32 *)_lastOpDetail ) = 0 ;
      _lastOpMsgSaved = FALSE ;
   }

   void _monAppCB::startOperator()
   {
      _lastOpBeginTime = pmdGetKRCB()->getCurTime() ;
      _lastOpEndTime.clear() ;
      _lastOpType = MSG_NULL ;
      _cmdType = CMD_UNKNOW ;
      // reset first 4 bytes to make sure both length of message and
      // first char of formatted detail are reset
      *( (INT32 *)_lastOpDetail ) = 0 ;
      _lastOpMsgSaved = FALSE ;
   }

   void _monAppCB::endOperator()
   {
      if ( (BOOLEAN)_lastOpBeginTime )
      {
         _lastOpEndTime = pmdGetKRCB()->getCurTime() ;
         ossTickDelta delta = _lastOpEndTime - _lastOpBeginTime ;
         opTimeSpentInc( delta ) ;
      }
   }

   void _monAppCB::setLastOpType( INT32 opType )
   {
      _lastOpType = opType ;
   }

   void _monAppCB::setLastCmdType( INT32 cmdType )
   {
      _cmdType = cmdType ;
   }

   void _monAppCB::setUnknownCmdType()
   {
      _cmdType = CMD_UNKNOW ;
   }

   void _monAppCB::opTimeSpentInc( ossTickDelta delta )
   {
      switch ( _lastOpType )
      {
         case MSG_BS_QUERY_REQ :
            {
               if ( _cmdType != CMD_UNKNOW )
               {
                  // it is command, do not inc readTimeSpent
                  break ;
               }
            }
         case MSG_BS_GETMORE_REQ :
         case MSG_BS_ADVANCE_REQ :
         /// LOB
         case MSG_BS_LOB_READ_REQ :
            {
               _readTimeSpent += delta ;
               break ;
            }
         case MSG_BS_INSERT_REQ :
         case MSG_BS_UPDATE_REQ :
         case MSG_BS_DELETE_REQ :
         /// LOB
         case MSG_BS_LOB_WRITE_REQ :
         case MSG_BS_LOB_REMOVE_REQ :
         case MSG_BS_LOB_UPDATE_REQ :
         case MSG_BS_LOB_TRUNCATE_REQ :
            {
               _writeTimeSpent += delta ;
               break ;
            }
         default :
            break ;
      }
   }

   const CHAR *_monAppCB::getLastOpDetail()
   {
      if ( _lastOpMsgSaved )
      {
         formatLastOpDetail( (const MsgHeader *)_lastOpDetail, _lastOpType ) ;
         _lastOpMsgSaved = FALSE ;
      }
      return _lastOpDetail ;
   }

   void _monAppCB::saveLastOpQuery( const MsgHeader *message,
                                    const rtnQueryOptions &options )
   {
      if ( NULL != message &&
           message->messageLength <= MON_APP_LASTOP_DESC_LEN )
      {
         ossMemcpy( _lastOpDetail, message, message->messageLength ) ;
         _lastOpMsgSaved = TRUE ;
      }
      else
      {
         formatLastOpDetail( options ) ;
         _lastOpMsgSaved = FALSE ;
      }
   }

   void _monAppCB::saveLastOpDetail( const CHAR *format, ... )
   {
      va_list argList ;
      UINT32 curLen = ossStrlen( _lastOpDetail ) ;
      if ( curLen >= sizeof( _lastOpDetail ) - 3 )
      {
         // buffer is full, couldn't save more info
         goto done ;
      }
      else if ( curLen > 0 )
      {
         _lastOpDetail[ curLen ] = ',' ;
         ++curLen ;
         _lastOpDetail[ curLen ] = ' ' ;
         ++curLen ;
         _lastOpDetail[ curLen ] = 0 ;
      }
      va_start( argList, format ) ;
      vsnprintf( _lastOpDetail + curLen,
                 sizeof( _lastOpDetail ) - curLen - 1,
                 format, argList ) ;
      va_end( argList ) ;

      _lastOpDetail[ sizeof( _lastOpDetail ) - 1 ] = '\0' ;
      _lastOpMsgSaved = FALSE ;

   done:
      return ;
   }

   void _monAppCB::replaceLastOpDetail( const CHAR *detail )
   {
      ossStrncpy( _lastOpDetail, detail, sizeof( _lastOpDetail ) - 1 ) ;
      _lastOpDetail[ sizeof( _lastOpDetail ) - 1 ] = '\0' ;
      _lastOpMsgSaved = FALSE ;
   }

   void _monAppCB::clearLastOpDetail()
   {
      _lastOpMsgSaved = FALSE ;
      // reset first 4 bytes to make sure both length of message and
      // first char of formatted detail are reset
      *( (INT32 *)_lastOpDetail ) = 0 ;
   }

   void _monAppCB::formatLastOpDetail( const rtnQueryOptions &options )
   {
      // reset flag first
      _lastOpMsgSaved = FALSE ;
      // reset first 4 bytes to make sure both length of message and
      // first char of formatted detail are reset
      *( (INT32 *)_lastOpDetail ) = 0 ;

      try
      {
         switch ( _lastOpType )
         {
            case MSG_BS_QUERY_REQ :
            {
               saveLastOpDetail( "Collection:%s, Matcher:%s, Selector:%s, "
                                 "OrderBy:%s, Hint:%s, Skip:%lld, Limit:%lld, "
                                 "Flag:0x%08x(%u)",
                                 options.getCLFullName(),
                                 options.getQuery().toPoolString().c_str(),
                                 options.getSelector().toPoolString().c_str(),
                                 options.getOrderBy().toPoolString().c_str(),
                                 options.getHint().toPoolString().c_str(),
                                 options.getSkip(),
                                 options.getLimit(),
                                 options.getFlag(),
                                 options.getFlag() ) ;
               break ;
            }
            case MSG_BS_INSERT_REQ :
            {
               saveLastOpDetail( "Collection:%s, Insertors:%s, ObjNum:%d, "
                                 "Flag:0x%08x(%u)",
                                 options.getCLFullName(),
                                 options.getInsertor().toPoolString().c_str(),
                                 options.getInsertNum(),
                                 options.getFlag(),
                                 options.getFlag() ) ;
               break ;
            }
            case MSG_BS_UPDATE_REQ :
            {
               saveLastOpDetail( "Collection:%s, Matcher:%s, Updator:%s, Hint:%s, "
                                 "Flag:0x%08x(%u)",
                                 options.getCLFullName(),
                                 options.getQuery().toPoolString().c_str(),
                                 options.getUpdator().toPoolString().c_str(),
                                 options.getHint().toPoolString().c_str(),
                                 options.getFlag(),
                                 options.getFlag() ) ;
               break ;
            }
            case MSG_BS_DELETE_REQ :
            {
               saveLastOpDetail( "Collection:%s, Deletor:%s, Hint:%s, "
                                 "Flag:0x%08x(%u)",
                                 options.getCLFullName(),
                                 options.getQuery().toPoolString().c_str(),
                                 options.getHint().toPoolString().c_str(),
                                 options.getFlag(),
                                 options.getFlag() ) ;
               break ;
            }
            default :
            {
               break ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDDEBUG, "Failed to save last op detail, "
                 "occur exception %s", e.what() ) ;
         // cut detail
         // reset first 4 bytes to make sure both length of message and
         // first char of formatted detail are reset
         *( (INT32 *)_lastOpDetail ) = 0 ;
      }
   }

   void _monAppCB::formatLastOpDetail( const MsgHeader *msg,
                                       INT32 expectingOpType )
   {
      SDB_ASSERT( NULL != msg, "message is invalid" ) ;

      BOOLEAN isValid = FALSE ;
      rtnQueryOptions options ;

      if ( msg->opCode == expectingOpType &&
           msg->messageLength > (INT32)( sizeof( MsgHeader ) ) &&
           msg->messageLength <= MON_APP_LASTOP_DESC_LEN )
      {
         CHAR temp[ MON_APP_LASTOP_DESC_LEN + 1 ] = { 0 } ;
         ossMemcpy( temp, (const CHAR *)msg, MON_APP_LASTOP_DESC_LEN ) ;
         switch ( expectingOpType )
         {
            case MSG_BS_QUERY_REQ :
            {
               if ( SDB_OK == options.fromQueryMsg( temp ) )
               {
                  isValid = TRUE ;
               }
               break ;
            }
            case MSG_BS_INSERT_REQ :
            {
               if ( SDB_OK == options.fromInsertMsg( temp ) )
               {
                  isValid = TRUE ;
               }
               break ;
            }
            case MSG_BS_DELETE_REQ :
            {
               if ( SDB_OK == options.fromDeleteMsg( temp ) )
               {
                  isValid = TRUE ;
               }
               break ;
            }
            case MSG_BS_UPDATE_REQ :
            {
               if ( SDB_OK == options.fromUpdateMsg( temp ) )
               {
                  isValid = TRUE ;
               }
               break ;
            }
            default:
            {
               // not support
               break ;
            }
         }

      }
      if ( isValid )
      {
         formatLastOpDetail( options ) ;
      }
      else
      {
         // cut the message
         *( (INT32 *)_lastOpDetail ) = 0 ;
      }
      _lastOpMsgSaved = FALSE ;
   }

   /*
      _monContextCB implement
    */
   _monContextCB::_monContextCB ()
   : _contextID( -1 ),
     _dataRead( 0 ),
     _indexRead( 0 ),
     _dataWrite( 0 ),
     _indexWrite( 0 ),
     _lobRead( 0 ),
     _lobWrite( 0 ),
     _lobTruncate( 0 ),
     _lobAddressing( 0 ),
     _returnBatches( 0 ),
     _returnRecords( 0 ),
     _startTimestamp(),
     _waitTime(),
     _queryTime(),
     _executeTime()
   {
   }

   _monContextCB::_monContextCB ( const _monContextCB & monCtxCB )
   : _contextID( monCtxCB._contextID ),
     _dataRead( monCtxCB._dataRead ),
     _indexRead( monCtxCB._indexRead ),
     _dataWrite( monCtxCB._dataWrite ),
     _indexWrite( monCtxCB._indexWrite ),
     _lobRead( monCtxCB._lobRead ),
     _lobWrite( monCtxCB._lobWrite ),
     _lobTruncate( monCtxCB._lobTruncate ),
     _lobAddressing( monCtxCB._lobAddressing ),
     _returnBatches( monCtxCB._returnBatches ),
     _returnRecords( monCtxCB._returnRecords ),
     _startTimestamp( monCtxCB._startTimestamp ),
     _waitTime( monCtxCB._waitTime ),
     _queryTime( monCtxCB._queryTime ),
     _executeTime( monCtxCB._executeTime )
   {
   }

   _monContextCB::~_monContextCB ()
   {
   }

   void _monContextCB::reset ()
   {
      _contextID     = -1 ;
      _dataRead      = 0 ;
      _indexRead     = 0 ;
      _dataWrite     = 0 ;
      _indexWrite    = 0 ;
      _lobRead       = 0 ;
      _lobWrite      = 0 ;
      _lobTruncate   = 0 ;
      _lobAddressing = 0 ;
      _returnBatches = 0 ;
      _returnRecords = 0 ;
      _startTimestamp.clear() ;
      _waitTime.clear() ;
      _queryTime.clear() ;
      _executeTime.clear() ;
    }

   _monContextCB & _monContextCB::operator = ( const _monContextCB & monCtxCB )
   {
      _contextID        = monCtxCB._contextID ;
      _dataRead         = monCtxCB._dataRead ;
      _indexRead        = monCtxCB._indexRead ;
      _dataWrite        = monCtxCB._dataWrite ;
      _indexWrite       = monCtxCB._indexWrite ;
      _lobRead          = monCtxCB._lobRead ;
      _lobWrite         = monCtxCB._lobWrite ;
      _lobTruncate      = monCtxCB._lobTruncate ;
      _lobAddressing    = monCtxCB._lobAddressing ;
      _returnBatches    = monCtxCB._returnBatches ;
      _returnRecords    = monCtxCB._returnRecords ;
      _startTimestamp   = monCtxCB._startTimestamp ;
      _waitTime         = monCtxCB._waitTime ;
      _queryTime        = monCtxCB._queryTime ;
      _executeTime      = monCtxCB._executeTime ;

      return ( *this ) ;
   }

   void _monContextCB::_increase( MON_OPERATION_TYPES op, UINT64 delta )
   {
      if ( delta <= 0 )
      {
         return ;
      }
      switch( op )
      {
         case MON_DATA_READ :
            monDataReadInc( delta ) ;
            break ;
         case MON_INDEX_READ :
            monIndexReadInc( delta ) ;
            break ;
         case MON_DATA_WRITE :
            monDataWriteInc( delta ) ;
            break ;
         case MON_INDEX_WRITE :
            monIndexWriteInc( delta ) ;
            break ;
         case MON_LOB_READ :
            monLobReadInc( delta ) ;
            break ;
         case MON_LOB_WRITE :
            monLobWriteInc( delta ) ;
            break ;
         case MON_LOB_TRUNCATE :
            monLobTruncateInc( delta ) ;
            break ;
         case MON_LOB_ADDRESSING :
            monLobAddressingInc( delta ) ;
            break ;
         default:
            break ;
      }
   }

   void _monContextCB::incMetrics( const _monAppCB &delta )
   {
      // FIXME: Temporary shielding the follow metrics to avoid
      // double submit, for they had submitted in real time
      /*
      // update operation count
      _increase( MON_DATA_READ, delta.totalDataRead ) ;
      _increase( MON_INDEX_READ, delta.totalIndexRead ) ;
      */
      _increase( MON_LOB_READ, delta.totalLobRead ) ;
      _increase( MON_LOB_WRITE, delta.totalLobWrite ) ;
      _increase( MON_LOB_TRUNCATE, delta.totalLobTruncate ) ;
      _increase( MON_LOB_ADDRESSING, delta.totalLobAddressing ) ;
   }

   /*
      _monSvcTaskInfo implement
   */
   _monSvcTaskInfo::_monSvcTaskInfo()
   {
      reset() ;

      _startTimestamp = _resetTimestamp ;
      _taskID = 0 ;
      ossMemset( _taskName, 0, sizeof( _taskName ) ) ;
   }

   void _monSvcTaskInfo::setTaskInfo( UINT64 taskID, const CHAR *taskName )
   {
      _taskID = taskID ;
      ossStrncpy( _taskName, taskName, MON_SVC_TASK_NAME_LEN ) ;
   }

   void _monSvcTaskInfo::reset()
   {
      ossAtomicExchange64( &_totalTime, 0 ) ;
      ossAtomicExchange64( &_totalDataRead, 0 ) ;
      ossAtomicExchange64( &_totalIndexRead, 0 ) ;
      ossAtomicExchange64( &_totalDataWrite, 0 ) ;
      ossAtomicExchange64( &_totalIndexWrite, 0 ) ;
      ossAtomicExchange64( &_totalUpdate, 0 ) ;
      ossAtomicExchange64( &_totalDelete, 0 ) ;
      ossAtomicExchange64( &_totalInsert, 0 ) ;
      ossAtomicExchange64( &_totalSelect, 0 ) ;
      ossAtomicExchange64( &_totalRead, 0 ) ;
      ossAtomicExchange64( &_totalWrite, 0 ) ;
      ossAtomicExchange64( &_totalContexts, 0 ) ;

      ossAtomicExchange64( &_totalLobGet, 0 ) ;
      ossAtomicExchange64( &_totalLobPut, 0 ) ;
      ossAtomicExchange64( &_totalLobDelete, 0 ) ;
      ossAtomicExchange64( &_totalLobList, 0 ) ;
      ossAtomicExchange64( &_totalLobReadSize, 0 ) ;
      ossAtomicExchange64( &_totalLobWriteSize, 0 ) ;
      ossAtomicExchange64( &_totalLobRead, 0 ) ;
      ossAtomicExchange64( &_totalLobWrite, 0 ) ;
      ossAtomicExchange64( &_totalLobTruncate, 0 ) ;
      ossAtomicExchange64( &_totalLobAddressing, 0 ) ;

      ossGetCurrentTime( _resetTimestamp ) ;
   }

   void _monSvcTaskInfo::_increase( MON_OPERATION_TYPES op, UINT64 delta )
   {
      if ( delta <= 0 )
      {
         return ;
      }
      if ( MON_COUNTER_OPERATION_NONE <= op && op <= MON_COUNTER_OPERATION_MAX )
      {
         monOperationCountInc( op, delta ) ;
      }
      else if ( MON_TIME_OPERATION_NONE <= op && op <= MON_TIME_OPERATION_MAX )
      {
         monOperationTimeInc( op, delta ) ;
      }
      else if ( MON_BYTE_COUNT_NONE <= op && op <= MON_BYTE_COUNT_MAX )
      {
         monByteCountInc( op, delta ) ;
      }
   }

   void _monSvcTaskInfo::incMetrics( const _monAppCB &delta )
   {
      // FIXME: Temporary shielding the follow metrics to avoid
      // double submit, for they had submitted in real time
      /*
      // update operation count
      _increase( MON_DATA_READ, delta.totalDataRead ) ;
      _increase( MON_INDEX_READ, delta.totalIndexRead ) ;
      _increase( MON_DATA_WRITE, delta.totalDataWrite ) ;
      _increase( MON_INDEX_WRITE, delta.totalIndexWrite ) ;
      _increase( MON_UPDATE, delta.totalUpdate ) ;
      _increase( MON_DELETE, delta.totalDelete ) ;
      _increase( MON_INSERT, delta.totalInsert ) ;
      _increase( MON_SELECT, delta.totalSelect ) ;
      _increase( MON_READ, delta.totalRead ) ;
      */
      _increase( MON_LOB_GET, delta.totalLobGet ) ;
      _increase( MON_LOB_PUT, delta.totalLobPut ) ;
      _increase( MON_LOB_DELETE, delta.totalLobDelete ) ;
      _increase( MON_LOB_LIST, delta.totalLobList ) ;
      _increase( MON_LOB_READ, delta.totalLobRead ) ;
      _increase( MON_LOB_WRITE, delta.totalLobWrite ) ;
      _increase( MON_LOB_TRUNCATE, delta.totalLobTruncate ) ;
      _increase( MON_LOB_ADDRESSING, delta.totalLobAddressing ) ;
      // update byte count
      _increase( MON_LOB_READ_BYTES, delta.totalLobReadSize ) ;
      _increase( MON_LOB_WRITE_BYTES, delta.totalLobWriteSize ) ;
   }


}

