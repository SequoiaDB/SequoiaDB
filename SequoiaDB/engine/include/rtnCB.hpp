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

   Source File Name = rtnCB.hpp

   Descriptive Name = RunTime Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNCB_HPP_
#define RTNCB_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "rtnContext.hpp"
#include "rtnLobAccessManager.hpp"
#include "ossLatch.hpp"
#include "ossAtomic.hpp"
#include "pd.hpp"
#include "monEDU.hpp"
#include "pmdEDU.hpp"
#include "sdbInterface.hpp"
#include "utilConcurrentMap.hpp"
#include "optAPM.hpp"
#include <map>
#include "ossMemPool.hpp"
#include "rtnLocalTaskMgr.hpp"
#include "rtnRemoteMessenger.hpp"
#include "rtnIxmKeySorter.hpp"
#include "rtnScannerChecker.hpp"
#include "dmsTaskStatus.hpp"
#include "rtnUserCache.hpp"

#define RTN_INIT_TEXT_INDEX_VERSION    -1

namespace engine
{
   /*
      _SDB_RTNCB define
   */
   class _SDB_RTNCB : public _IControlBlock, public _IContextMgr, public _IEventHander
   {
   private :
      typedef utilConcurrentMap<INT64, rtnContextPtr> RTN_CTX_MAP ;
      typedef ossPoolMultiMap< EDUID, INT64 > _RTN_EDU_CTX_MAP ;

      ossAtomicSigned64    _contextIdGenerator ;
      RTN_CTX_MAP          _contextMap ;
      INT32                _maxContextNum ;
      INT32                _maxSessionContextNum ;
      INT32                _contextTimeout ;

      optAccessPlanManager _accessPlanManager ;

      _rtnLobAccessManager _lobAccessManager ;

      dmsTaskStatusMgr  _taskStatusMgr ;

      // The following members are used for communication with search engine
      // adapter when do text searching. Search engine adapter use the shard
      // plane to get data from data node, and data node use this new plane to
      // send text search request to adapter.
      rtnRemoteMessenger   *_remoteMessenger ;
      ossSpinSLatch        _mutex ;        // Lock for protection of accessing
                                          // index information.
      ossAtomicSigned64    _textIdxVersion ;

      rtnLocalTaskMgr      *_pLTMgr ;

      ossPoolSet<ossPoolString> _unloadCSSet ;
      ossSpinSLatch             _csLatch ;

      rtnIxmKeySorterCreator    _sorterCreator ;
      rtnScannerCheckerCreator  _checkerCreator ;

      rtnUserCache _userCache;

   public:
      virtual void contextDelete( INT64 contextID, IExecutor *pExe ) ;
      virtual void* queryInterface( SDB_INTERFACE_TYPE type ) ;

      virtual void   onPrimaryChange( BOOLEAN primary,
                                      SDB_EVENT_OCCUR_TYPE occurType ) ;

   public :
      _SDB_RTNCB() ;
      virtual ~_SDB_RTNCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_RTN ; }
      virtual const CHAR* cbName() const { return "RTNCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange () ;

      INT32 contextNew( RTN_CONTEXT_TYPE type,
                        rtnContextPtr &context,
                        INT64 &contextID,
                        _pmdEDUCB * pEDUCB ) ;

      INT32 contextFind( INT64 contextID,
                         rtnContextPtr &context,
                         _pmdEDUCB *cb = NULL ) ;
      INT32 contextFind( INT64 contextID,
                         RTN_CONTEXT_TYPE type,
                         rtnContextPtr &context,
                         _pmdEDUCB *cb = NULL,
                         BOOLEAN closeOnUnexpectType = TRUE ) ;
      BOOLEAN contextExist( INT64 contextID ) ;

      INT32 prepareRemoteMessenger() ;

      // try notify context owners to kill contexts of given collection space
      UINT32 preDelContext( const CHAR *csName, UINT32 suLogicalID ) ;
      // try notify context owners to kill expired contexts
      UINT32 preDelExpiredContext() ;

      INT32 dumpWritingContext( RTN_CTX_PROCESS_LIST &contextProcessList,
                                EDUID filterEDUID = PMD_INVALID_EDUID,
                                UINT64 blockID = 0 ) ;

      OSS_INLINE INT32 contextNum ()
      {
         return _contextMap.size() ;
      }

      OSS_INLINE void contextDump ( std::map<UINT64, ossPoolSet<SINT64> > &contextList,
                                    EDUID filterEDUID = PMD_INVALID_EDUID )
      {
         FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
         {
            INT64 contextID = -1  ;
            EDUID eduID = (*it).second->eduID() ;

            if ( PMD_INVALID_EDUID != filterEDUID &&
                 eduID != filterEDUID )
            {
               continue ;
            }

            contextID = (*it).second->contextID() ;
            contextList[ eduID ].insert( contextID ) ;
         }
         FOR_EACH_CMAP_ELEMENT_END
      }

      OSS_INLINE void monContextSnap ( std::map<UINT64,std::set<monContextFull> > &contextList,
                                       EDUID filterEDUID = PMD_INVALID_EDUID )
      {
         FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
         {
            INT64 contextID = -1  ;
            const monContextCB *monCB = NULL ;

            SDB_ASSERT( NULL != (*it).second.get(), "context is invalid" ) ;

            EDUID eduID = (*it).second->eduID() ;

            if ( PMD_INVALID_EDUID != filterEDUID &&
                 eduID != filterEDUID )
            {
               continue ;
            }

            contextID = (*it).second->contextID() ;
            monCB = (*it).second->getMonCB() ;

            monContextFull item( contextID, *monCB ) ;
            item._typeDesp = (*it).second->name() ;
            item._info = (*it).second.get()->toString() ;
            item._queryID = (*it).second->getGlobalID().getQueryID() ;

            contextList[ eduID ].insert( item ) ;
         }
         FOR_EACH_CMAP_ELEMENT_END
      }

      OSS_INLINE void monContextSnap( UINT64 eduID,
                                      std::set<monContextFull> &contextList )
      {
         FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
         {
            SDB_ASSERT( NULL != (*it).second.get(), "context is invalid" ) ;

            INT64 contextID = (*it).second->contextID() ;
            const monContextCB* monCB = (*it).second->getMonCB() ;

            monContextFull item( contextID, *monCB ) ;
            item._typeDesp = (*it).second->name() ;
            item._info = (*it).second.get()->toString() ;

            contextList.insert( item ) ;
         }
         FOR_EACH_CMAP_ELEMENT_END
      }

      OSS_INLINE BOOLEAN isEnabledMixCmp () const
      {
         return this->_accessPlanManager.mthEnabledMixCmp() ;
      }

      OSS_INLINE rtnRemoteMessenger* getRemoteMessenger()
      {
         return _remoteMessenger ;
      }

      OSS_INLINE INT64 getTextIdxVersion()
      {
         return _textIdxVersion.peek() ;
      }
      OSS_INLINE void incTextIdxVersion()
      {
         _textIdxVersion.inc() ;
      }
      OSS_INLINE BOOLEAN updateTextIdxVersion( INT64 oldVersion,
                                               INT64 newVersion )
      {
         return _textIdxVersion.compareAndSwap( oldVersion, newVersion ) ;
      }

      OSS_INLINE optAccessPlanManager *getAPM ()
      {
         return &_accessPlanManager ;
      }

      OSS_INLINE _rtnLobAccessManager* getLobAccessManager()
      {
         return &_lobAccessManager ;
      }

      OSS_INLINE rtnLocalTaskMgr* getLTMgr()
      {
         return _pLTMgr ;
      }

      OSS_INLINE dmsTaskStatusMgr* getTaskStatusMgr()
      {
         return &_taskStatusMgr ;
      }

      OSS_INLINE rtnUserCache* getUserCacheMgr()
      {
         return &_userCache;
      }

      INT32   addUnloadCS( const CHAR* csName ) ;
      void    delUnloadCS( const CHAR* csName ) ;
      BOOLEAN hasUnloadCS( const CHAR* csName ) ;

   private:
      void  _notifyKillContexts( const _RTN_EDU_CTX_MAP &contexts ) ;
      void  _setGlobalID( _pmdEDUCB *cb, rtnContextPtr &pContext ) ;
   } ;
   typedef class _SDB_RTNCB SDB_RTNCB ;

   /*
      get global rtn cb
   */
   SDB_RTNCB* sdbGetRTNCB () ;

}

#endif //RTNCB_HPP_

