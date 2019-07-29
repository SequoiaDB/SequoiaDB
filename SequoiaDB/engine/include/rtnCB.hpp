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
#include "dmsCB.hpp"
#include "pmdEDU.hpp"
#include "sdbInterface.hpp"
#include "utilConcurrentMap.hpp"
#include "optAPM.hpp"
#include <map>
#include <set>
#include "rtnRemoteMessenger.hpp"

#define RTN_INIT_TEXT_INDEX_VERSION    -1

namespace engine
{
   /*
      _SDB_RTNCB define
   */
   class _SDB_RTNCB : public _IControlBlock, public _IContextMgr
   {
   private :
      typedef utilConcurrentMap<INT64, rtnContext*> RTN_CTX_MAP ;

      ossAtomicSigned64    _contextIdGenerator ;
      RTN_CTX_MAP          _contextMap ;

      optAccessPlanManager _accessPlanManager ;

      _rtnLobAccessManager _lobAccessManager ;

      rtnRemoteMessenger   *_remoteMessenger ;
      ossSpinSLatch        _mutex ;        // Lock for protection of accessing
      ossAtomicSigned64    _textIdxVersion ;

   public:
      virtual void contextDelete( INT64 contextID, IExecutor *pExe ) ;
      virtual void* queryInterface( SDB_INTERFACE_TYPE type ) ;

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

      SINT32 contextNew ( RTN_CONTEXT_TYPE type, rtnContext **context,
                          SINT64 &contextID, _pmdEDUCB * pEDUCB ) ;

      rtnContext *contextFind ( SINT64 contextID, _pmdEDUCB *cb = NULL ) ;

      INT32 prepareRemoteMessenger() ;

      OSS_INLINE INT32 contextNum ()
      {
         return _contextMap.size() ;
      }

      OSS_INLINE void contextDump ( std::map<UINT64, std::set<SINT64> > &contextList,
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
            monContextCB *monCB = NULL ;
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
            item._info = (*it).second->toString() ;

            contextList[ eduID ].insert( item ) ;
         }
         FOR_EACH_CMAP_ELEMENT_END
      }

      OSS_INLINE void monContextSnap( UINT64 eduID,
                                      std::set<monContextFull> &contextList )
      {
         FOR_EACH_CMAP_ELEMENT_S( RTN_CTX_MAP, _contextMap )
         {
            INT64 contextID = (*it).second->contextID() ;
            monContextCB* monCB = (*it).second->getMonCB() ;

            monContextFull item( contextID, *monCB ) ;
            item._typeDesp = (*it).second->name() ;
            item._info = (*it).second->toString() ;

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

   } ;
   typedef class _SDB_RTNCB SDB_RTNCB ;

   /*
      get global rtn cb
   */
   SDB_RTNCB* sdbGetRTNCB () ;

}

#endif //RTNCB_HPP_

