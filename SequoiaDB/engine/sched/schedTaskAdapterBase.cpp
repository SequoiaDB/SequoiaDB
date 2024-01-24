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

   Source File Name = schedTaskAdapterBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "schedTaskAdapterBase.hpp"
#include "utilMemListPool.hpp"
#include "pmdEnv.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   #define SCHED_PREPARE_THRESHOLD           ( 6 )

   /*
      _schedTaskAdapterBase implement
   */
   _schedTaskAdapterBase::_schedTaskAdapterBase()
   :_doNotify( 0 ), _hardNum( 0 ), _eventNum( 0 ), _cacheNum( 0 )
   {
      _pTaskInfo = NULL ;
   }

   _schedTaskAdapterBase::~_schedTaskAdapterBase()
   {
   }

   INT32 _schedTaskAdapterBase::init( schedTaskInfo *pInfo,
                                      SCHED_TASK_QUE_TYPE queType )
   {
      _pTaskInfo = pInfo ;
      return _onInit( queType ) ;
   }

   void _schedTaskAdapterBase::fini()
   {
      _onFini() ;
   }

   BOOLEAN _schedTaskAdapterBase::pop( INT64 millisec,
                                       MsgHeader **pHeader,
                                       NET_HANDLE &handle,
                                       pmdEDUMemTypes &memType )
   {
      BOOLEAN bPop = FALSE ;
      pmdEDUEvent event ;

      bPop = _queue.pop( event, millisec ) ;
      if ( bPop )
      {
         _eventNum.dec() ;
         _hardNum.inc() ;

         *pHeader = (MsgHeader*)event._Data ;
         handle = (NET_HANDLE)event._userData ;
         memType = event._dataMemType ;
      }

      if ( _doNotify.compare( 0 ) &&
           _queue.size() < SCHED_PREPARE_THRESHOLD )
      {
         _doNotify.swap( 1 ) ;
         _notifyEvent.signal() ;
      }

      return bPop ;
   }

   INT32 _schedTaskAdapterBase::push( const NET_HANDLE &handle,
                                      const _MsgHeader *header,
                                      const schedInfo *pInfo )
   {
      INT32 rc = SDB_OK ;
      CHAR *pBuff = NULL ;
      INT64 priority = 0 ;
      pmdEDUEvent event ;

      UINT64 lastCacheNum = 0 ;
      UINT64 lastEventNum = 0 ;

      pBuff = ( CHAR* )SDB_THREAD_ALLOC( header->messageLength ) ;
      if ( !pBuff )
      {
         PD_LOG( PDERROR, "Alloc memory failed, size: %d",
                 header->messageLength ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      ossMemcpy( pBuff, ( const CHAR* )header, header->messageLength ) ;

      event._Data = ( void* )pBuff ;
      event._dataMemType = PMD_EDU_MEM_THREAD ;
      event._eventType = PMD_EDU_EVENT_MSG ;
      event._userData = ( UINT64 )handle ;

      priority = (INT64)pmdGetDBTick() + pInfo->getNice() * 1000 ;

      rc = _onPush( event, priority, pInfo ) ;
      if ( rc )
      {
         goto error ;
      }
      pBuff = NULL ;

      lastCacheNum = _cacheNum.inc() ;
      lastEventNum = _eventNum.inc() ;

      /// notify prepare task
      if ( lastEventNum < lastCacheNum + SCHED_PREPARE_THRESHOLD &&
           _doNotify.compare( 0 ) )
      {
         _doNotify.swap( 1 ) ;
         _notifyEvent.signal() ;
      }

   done:
      return rc ;
   error:
      if ( pBuff )
      {
         SDB_OSS_FREE( pBuff ) ;
      }
      goto done ;
   }

   UINT32 _schedTaskAdapterBase::prepare( INT64 millisec )
   {
      UINT32 preparedNum = 0 ;
      UINT32 expectNum = 0 ;

      /// wait
      _notifyEvent.wait( millisec ) ;

      if ( SDB_OK == _pTaskInfo->getAndWaitRemaingTask( millisec,
                                                        expectNum ) )
      {
         UINT32 cacheNum = _cacheNum.fetch() ;
         if ( expectNum > cacheNum )
         {
            expectNum = cacheNum ;
         }
         preparedNum = _onPrepared( expectNum ) ;
      }

      _doNotify.swap( 0 ) ;

      return preparedNum ;
   }

   void _schedTaskAdapterBase::dump( BSONObjBuilder &builder )
   {
      builder.append( FIELD_NAME_SCHDLR_TYPE, ( INT32 )_getType() ) ;
      builder.append( FIELD_NAME_SCHDLR_TYPE_DESP,
                      schedType2String( _getType() ) ) ;
      builder.append( FIELD_NAME_RUN, (INT32)_pTaskInfo->getRunTaskNum() ) ;
      builder.append( FIELD_NAME_WAIT, (INT32)_eventNum.fetch() ) ;
      builder.append( FIELD_NAME_SCHDLR_MGR_EVT_NUM,
                      (INT32)_cacheNum.fetch() ) ;
      builder.append( FIELD_NAME_SCHDLR_TIMES, (INT64)_hardNum.fetch() ) ;
   }

   void _schedTaskAdapterBase::resetDump()
   {
      _hardNum.swap( 0 ) ;
   }

   void _schedTaskAdapterBase::_push2Que( const pmdEDUEvent &event )
   {
      _queue.push( event, 0 ) ;
      _cacheNum.dec() ;
   }

   BOOLEAN _schedTaskAdapterBase::_isControlMsg( const pmdEDUEvent &event )
   {
      const MsgHeader *pHeader = NULL ;

      pHeader = ( const MsgHeader* )event._Data ;

      if ( MSG_BS_DISCONNECT == pHeader->opCode ||
           MSG_BS_INTERRUPTE == pHeader->opCode ||
           MSG_BS_INTERRUPTE_SELF == pHeader->opCode )
      {
         return TRUE ;
      }
      return FALSE ;
   }

}

