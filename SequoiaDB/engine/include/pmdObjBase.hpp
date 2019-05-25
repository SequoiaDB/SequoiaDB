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

   Source File Name = pmdObjBase.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_OBJ_BASE_HPP_
#define PMD_OBJ_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pd.hpp"
#include "pmdDef.hpp"
#include "ossErr.h"
#include "msg.h"
#include "ossUtil.hpp"
#include "netDef.hpp"

namespace engine
{

   class _pmdEDUCB ;

   struct _msgMapEntry;
   enum MSG_SIG_FLAG { sig_event, sig_msg, sig_end };

   struct _msgMap
   {
      const _msgMap* (*pfnGetBaseMap)();
      const _msgMapEntry* entries;
   };

#define DECLARE_OBJ_MSG_MAP() \
   public: \
      virtual const CHAR *name () const ; \
   protected: \
      static const _msgMap* getThisMsgMap(); \
      virtual const _msgMap* getMsgMap() const; \


#define BEGIN_OBJ_MSG_MAP(theClass, baseClass) \
   const CHAR *theClass::name () const \
   { \
      return #theClass ; \
   } \
   const _msgMap* theClass::getMsgMap() const \
   { \
      return getThisMsgMap(); \
   } \
   const _msgMap* theClass::getThisMsgMap()  \
   { \
      typedef theClass thisClass; \
      typedef baseClass theBaseClass; \
      static const _msgMapEntry _msgEntries [] = \
      {

#define END_OBJ_MSG_MAP() \
         {0, 0, sig_end, "", NULL, NULL } \
      }; \
      static const _msgMap msgMap = \
      { &theBaseClass::getThisMsgMap, &_msgEntries[0] }; \
      return &msgMap; \
   }

#define ON_MSG_RANGE(typeBegin, typeEnd, func) \
   { (UINT32)typeBegin, (UINT32)typeEnd, sig_msg, #func, (MSG_FUNC)(&thisClass::func), NULL },
#define ON_MSG(type, func) ON_MSG_RANGE(type, type, func)

#define ON_EVENT_RANGE(typeBegin, typeEnd, func ) \
   { typeBegin, typeEnd, sig_event, #func, NULL, \
      (EVENT_FUNC)(&thisClass::func) },
#define ON_EVENT(type, func) ON_EVENT_RANGE(type, type, func)

   class _pmdObjBase : public SDBObject
   {
      public:
         _pmdObjBase() { _bProcess = FALSE ; }
         virtual ~_pmdObjBase() {}

         pmdEDUEvent*   getLastEvent() { return &_lastEvent ; }

         virtual void   attachCB( _pmdEDUCB *cb ) {}
         virtual void   detachCB( _pmdEDUCB *cb ) {}

         virtual const CHAR *name () const { return "_pmdObjBase"; }
         virtual INT32 getMaxProcMsgTime() const { return -1 ; }
         virtual INT32 getMaxProcEventTime() const { return -1 ; }

      public:
         OSS_INLINE BOOLEAN isProcess () const { return _bProcess ; }
         OSS_INLINE INT32   dispatch ( pmdEDUEvent *event,
                                       INT32 *pTime = NULL ) ;
         OSS_INLINE INT32   dispatchEvent ( pmdEDUEvent *event,
                                            INT32 *pTime = NULL ) ;
         OSS_INLINE INT32   dispatchMsg( NET_HANDLE handle, MsgHeader* msg,
                                         INT32 *pTime = NULL ) ;
         virtual void   onTimer ( UINT64 timerID, UINT32 interval ) { }

      protected:
         virtual OSS_INLINE INT32 _defaultMsgFunc ( NET_HANDLE handle,
                                                    MsgHeader* msg )
         {
            PD_LOG( PDWARNING, "[%s]Recieve unknow msg[type:[%d]%u, len:%u]",
               name(), IS_REPLY_TYPE( msg->opCode ) ? 1 : 0,
               GET_REQUEST_TYPE( msg->opCode ),
               msg->messageLength );
            return SDB_CLS_UNKNOW_MSG;
         }
         virtual OSS_INLINE INT32 _defaultEventFunc ( pmdEDUEvent *event )
         {
            PD_LOG ( PDWARNING, "[%s]Recieve unknown event[type:%u]",
               name(), event->_eventType ) ;
            return SDB_SYS ;
         }

         void  _onTimer(UINT64 timerID, UINT32 interval, UINT64 occurTime)
         {
            ossTimestamp ts;
            ossGetCurrentTime(ts);
            if (ts.time - occurTime > 2)
            {
               PD_LOG( PDWARNING,  "[%s]Timer(ID:%u-%u,interval:%u) lantcy %u "
                       "seconds", name(), (UINT32)(timerID >> 32),
                       (UINT32)(timerID & 0xFFFFFFFF), interval,
                       (UINT32)(ts.time - occurTime) );
            }

            onTimer ( timerID, interval ) ;
         }

         OSS_INLINE static const _msgMap* getThisMsgMap();
         OSS_INLINE static const _msgMap* _getBaseMsgMap() { return NULL; }

         virtual const _msgMap* getMsgMap() const { return getThisMsgMap(); }

      private:
         BOOLEAN           _bProcess ;
         pmdEDUEvent       _lastEvent ;

   };


   typedef INT32 (_pmdObjBase::*MSG_FUNC)( NET_HANDLE handle, MsgHeader* msg );
   typedef INT32 (_pmdObjBase::*EVENT_FUNC)( pmdEDUEvent *event );

   struct _msgMapEntry
   {
      UINT32                  msgType ;
      UINT32                  msgEndType ;
      INT32                   msgSig ;
      const CHAR*             pFncName ;
      MSG_FUNC                pMsgFn ;
      EVENT_FUNC              pEventFn ;
   };

   OSS_INLINE INT32 _pmdObjBase::dispatch ( pmdEDUEvent * event, INT32 *pTime )
   {
      if ( event->_eventType == PMD_EDU_EVENT_MSG )
      {
         return dispatchMsg ( 0, (MsgHeader*)(event->_Data), pTime ) ;
      }
      else
      {
         return dispatchEvent( event, pTime ) ;
      }
   }

   OSS_INLINE INT32 _pmdObjBase::dispatchMsg( NET_HANDLE handle, MsgHeader* msg,
                                              INT32 *pTime )
   {
      _bProcess = TRUE ;
      INT32 rc = SDB_OK;
      time_t bTime = 0 ;
      INT32  diffTime = 0 ;

      _lastEvent._Data = msg ;
      _lastEvent._dataMemType = PMD_EDU_MEM_NONE ;
      _lastEvent._eventType = PMD_EDU_EVENT_MSG ;
      _lastEvent._userData = ( UINT64 )handle ;

      const _msgMap* msgMap = NULL;
      for (msgMap = getMsgMap(); msgMap != NULL; 
               msgMap = (*msgMap->pfnGetBaseMap)())
      {
         UINT32 index = 0;
         while ( msgMap->entries[index].msgSig != sig_end )
         {
            if ( msgMap->entries[index].msgSig == sig_msg
               && msgMap->entries[index].msgType <= (UINT32)(msg->opCode)
               && msgMap->entries[index].msgEndType >= (UINT32)(msg->opCode) )
            {
               bTime = time( NULL ) ;
               rc =  (this->*(msgMap->entries[index].pMsgFn))( handle, msg ) ;
               diffTime = (INT32)(time( NULL ) - bTime) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDDEBUG, "[%s]Process func[%s] failed[rc = %d]",
                          name(), msgMap->entries[index].pFncName, rc ) ;
               }
               goto done;
            }
            index++;
         }
      }

      bTime = time( NULL ) ;
      rc = _defaultMsgFunc( handle, msg ) ;
      diffTime = (INT32)(time( NULL ) - bTime) ;

   done:
      _bProcess = FALSE ;
      _lastEvent.reset() ;
      if ( pTime )
      {
         *pTime = diffTime ;
      }
      else if ( diffTime > 10 )
      {
         PD_LOG( PDDEBUG, "[%s] Process msg[opCode:[%d]%d, requestID: %lld, "
                 "TID: %d, Len: %d] over %d seconds", name(),
                 IS_REPLY_TYPE(msg->opCode), GET_REQUEST_TYPE(msg->opCode),
                 msg->requestID, msg->TID, msg->messageLength, diffTime ) ;
      }
      return rc;
   }

   OSS_INLINE INT32 _pmdObjBase::dispatchEvent( pmdEDUEvent * event,
                                                INT32 *pTime )
   {
      _bProcess = TRUE ;
      INT32 rc = SDB_OK;
      time_t bTime = 0 ;
      INT32  diffTime = 0 ;

      _lastEvent = *event ;

      const _msgMap* msgMap = NULL;
      for (msgMap = getMsgMap(); msgMap != NULL; 
               msgMap = (*msgMap->pfnGetBaseMap)())
      {
         UINT32 index = 0;
         while ( msgMap->entries[index].msgSig != sig_end )
         {
            if ( msgMap->entries[index].msgSig == sig_event
               && msgMap->entries[index].msgType <= (UINT32)(event->_eventType)
               && msgMap->entries[index].msgEndType >=
                  (UINT32)(event->_eventType) )
            {
               bTime = time( NULL ) ;
               rc =  (this->*(msgMap->entries[index].pEventFn))( event );
               diffTime = (INT32)(time( NULL ) - bTime) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDDEBUG, "[%s]Process func[%s] failed[rc = %d]",
                     name(), msgMap->entries[index].pFncName, rc );
               }
               goto done;
            }
            index++;
         }
      }

      if ( event->_eventType== PMD_EDU_EVENT_TIMEOUT )
      {
         PMD_EVENT_MESSAGES *timeMsg = (PMD_EVENT_MESSAGES*)( event->_Data );

         bTime = time( NULL ) ;
         _onTimer( timeMsg->timeoutMsg.timerID, timeMsg->timeoutMsg.interval,
                   timeMsg->timeoutMsg.occurTime ) ;
         diffTime = (INT32)(time( NULL ) - bTime) ;
      }
      else //Default Func
      {
         bTime = time( NULL ) ;
         rc = _defaultEventFunc( event );
         diffTime = (INT32)(time( NULL ) - bTime) ;
      }

   done:
      _bProcess = FALSE ;
      _lastEvent.reset() ;
      if ( pTime )
      {
         *pTime = diffTime ;
      }
      else if ( diffTime > 5 )
      {
         PD_LOG( PDINFO, "[%s] Process event[type:%d] over %d seconds", name(),
                 event->_eventType, diffTime ) ;
      }

      return rc;
   }

   OSS_INLINE  const _msgMap* _pmdObjBase::getThisMsgMap()
   {
      static const _msgMapEntry msgEntries[] = {
         {0, 0, sig_end, "", NULL, NULL } };
      static const _msgMap msgMap = { &_pmdObjBase::_getBaseMsgMap,
                                      &msgEntries[0] };
      return &msgMap;
   }

};

#endif //PMD_OBJ_BASE_HPP_


