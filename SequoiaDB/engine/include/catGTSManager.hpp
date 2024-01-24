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

   Source File Name = catGTSManager.hpp

   Descriptive Name = GTS(Global Transaction Service) manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/13/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_MANAGER_HPP_
#define CAT_GTS_MANAGER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "netDef.hpp"
#include "msg.h"
#include "catGTSMsgHandler.hpp"
#include "catSequenceManager.hpp"
#include "catEventHandler.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class sdbCatalogueCB ;

   class _catGTSManager: public SDBObject,
                         public _catEventHandler
   {
   private:
      // disallow copy and assign
      _catGTSManager( const _catGTSManager& ) ;
      void operator=( const _catGTSManager& ) ;
   public:
      _catGTSManager() ;
      ~_catGTSManager() ;

      INT32 init() ;
      INT32 fini() ;

      void  attachCB( _pmdEDUCB* cb ) ;
      void  detachCB( _pmdEDUCB* cb ) ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 handleMsg( const NET_HANDLE& handle, const MsgHeader* msg ) ;

      virtual const CHAR *getHandlerName() { return "catGTSManager" ; }
      virtual INT32 onUpgrade( UINT32 version ) ;

   public:
      OSS_INLINE _catSequenceManager* getSequenceMgr()
      {
         return &_seqMgr ;
      }

   private:
      INT32 _ensureMetadata() ;
      INT32 _createSysIndex ( const CHAR* clFullName,
                              const CHAR* indexJson,
                              _pmdEDUCB* cb ) ;
      INT32 _createSysCollection ( const CHAR* clFullName,
                                   utilCLUniqueID clUID,
                                   _pmdEDUCB* cb ) ;

      // add collection unique ID to sequence
      // upgrade from 3.6 / 5.0.3
      INT32 _checkAndUpgradeSequenceCLUID() ;

   private:
      _SDB_DMSCB*          _dmsCB ;
      _pmdEDUCB*           _eduCB ;
      sdbCatalogueCB*      _catCB ;
      _catGTSMsgHandler    _msgHandler ;
      _catSequenceManager  _seqMgr ;
   } ;
   typedef _catGTSManager catGTSManager ;
}

#endif /* CAT_GTS_MANAGER_HPP_ */
