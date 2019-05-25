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

   Source File Name = rtnCommandList.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/


#include "rtnCommandList.hpp"
#include "rtnFetchBase.hpp"
#include "monDump.hpp"

using namespace bson ;

namespace engine
{

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollections)
   /*
      _rtnListCollections implement
   */
   _rtnListCollections::_rtnListCollections()
   {
   }

   _rtnListCollections::~_rtnListCollections()
   {
   }

   const CHAR* _rtnListCollections::name()
   {
      return NAME_LIST_COLLECTIONS ;
   }

   RTN_COMMAND_TYPE _rtnListCollections::type()
   {
      return CMD_LIST_COLLECTIONS ;
   }

   INT32 _rtnListCollections::_getFetchType() const
   {
      return RTN_FETCH_COLLECTION ;
   }

   BOOLEAN _rtnListCollections::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnListCollections::_addInfoMask() const
   {
      return 0 ;
   }

   const CHAR* _rtnListCollections::getIntrCMDName()
   {
      return CMD_NAME_LIST_COLLECTION_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollectionsInner)
   /*
      _rtnListCollectionsInner implement
   */
   _rtnListCollectionsInner::_rtnListCollectionsInner()
   {
   }

   _rtnListCollectionsInner::~_rtnListCollectionsInner()
   {
   }

   const CHAR* _rtnListCollectionsInner::name()
   {
      return CMD_NAME_LIST_COLLECTION_INTR ;
   }

   RTN_COMMAND_TYPE _rtnListCollectionsInner::type()
   {
      return CMD_LIST_COLLECTIONS ;
   }

   INT32 _rtnListCollectionsInner::_getFetchType() const
   {
      return RTN_FETCH_COLLECTION ;
   }

   BOOLEAN _rtnListCollectionsInner::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnListCollectionsInner::_addInfoMask() const
   {
      return 0 ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollectionspaces)
   /*
      _rtnListCollectionspaces implement
   */
   _rtnListCollectionspaces::_rtnListCollectionspaces()
   {
   }
   _rtnListCollectionspaces::~_rtnListCollectionspaces()
   {
   }

   const CHAR* _rtnListCollectionspaces::name ()
   {
      return NAME_LIST_COLLECTIONSPACES ;
   }

   RTN_COMMAND_TYPE _rtnListCollectionspaces::type ()
   {
      return CMD_LIST_COLLECTIONSPACES ;
   }

   INT32 _rtnListCollectionspaces::_getFetchType() const
   {
      return RTN_FETCH_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnListCollectionspaces::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnListCollectionspaces::_addInfoMask() const
   {
      return 0 ;
   }

   const CHAR* _rtnListCollectionspaces::getIntrCMDName()
   {
      return CMD_NAME_LIST_SPACE_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListCollectionspacesInner)
   /*
      _rtnListCollectionspacesInner implement
   */
   _rtnListCollectionspacesInner::_rtnListCollectionspacesInner()
   {
   }

   _rtnListCollectionspacesInner::~_rtnListCollectionspacesInner()
   {
   }

   const CHAR* _rtnListCollectionspacesInner::name()
   {
      return CMD_NAME_LIST_SPACE_INTR ;
   }

   RTN_COMMAND_TYPE _rtnListCollectionspacesInner::type()
   {
      return CMD_LIST_COLLECTIONSPACES ;
   }

   INT32 _rtnListCollectionspacesInner::_getFetchType() const
   {
      return RTN_FETCH_COLLECTIONSPACE ;
   }

   BOOLEAN _rtnListCollectionspacesInner::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnListCollectionspacesInner::_addInfoMask() const
   {
      return 0 ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContexts)
   /*
      _rtnListContexts implement
   */
   _rtnListContexts::_rtnListContexts()
   {
   }
   _rtnListContexts::~_rtnListContexts()
   {
   }

   const CHAR* _rtnListContexts::name ()
   {
      return NAME_LIST_CONTEXTS ;
   }

   RTN_COMMAND_TYPE _rtnListContexts::type ()
   {
      return CMD_LIST_CONTEXTS ;
   }

   INT32 _rtnListContexts::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnListContexts::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListContexts::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnListContexts::getIntrCMDName()
   {
      return CMD_NAME_LIST_CONTEXT_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContextsInner)
   /*
      _rtnListContextsInner implement
   */
   _rtnListContextsInner::_rtnListContextsInner()
   {
   }
   _rtnListContextsInner::~_rtnListContextsInner()
   {
   }

   const CHAR *_rtnListContextsInner::name ()
   {
      return CMD_NAME_LIST_CONTEXT_INTR ;
   }

   RTN_COMMAND_TYPE _rtnListContextsInner::type ()
   {
      return CMD_LIST_CONTEXTS ;
   }

   INT32 _rtnListContextsInner::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnListContextsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListContextsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContextsCurrent)
   /*
      _rtnListContextsCurrent implement
   */
   _rtnListContextsCurrent::_rtnListContextsCurrent()
   {
   }

   _rtnListContextsCurrent::~_rtnListContextsCurrent()
   {
   }

   const CHAR* _rtnListContextsCurrent::name()
   {
      return NAME_LIST_CONTEXTS_CURRENT ;
   }

   RTN_COMMAND_TYPE _rtnListContextsCurrent::type()
   {
      return CMD_LIST_CONTEXTS_CURRENT ;
   }

   INT32 _rtnListContextsCurrent::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnListContextsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnListContextsCurrent::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnListContextsCurrent::getIntrCMDName()
   {
      return CMD_NAME_LIST_CONTEXTCUR_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListContextsCurrentInner)
   /*
      _rtnListContextsCurrentInner implement
   */
   _rtnListContextsCurrentInner::_rtnListContextsCurrentInner()
   {
   }

   _rtnListContextsCurrentInner::~_rtnListContextsCurrentInner()
   {
   }

   const CHAR *_rtnListContextsCurrentInner::name ()
   {
      return CMD_NAME_LIST_CONTEXTCUR_INTR ;
   }

   RTN_COMMAND_TYPE _rtnListContextsCurrentInner::type ()
   {
      return CMD_LIST_CONTEXTS_CURRENT ;
   }

   INT32 _rtnListContextsCurrentInner::_getFetchType() const
   {
      return RTN_FETCH_CONTEXT ;
   }

   BOOLEAN _rtnListContextsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnListContextsCurrentInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessions)
   /*
      _rtnListSessions implement
   */
   _rtnListSessions::_rtnListSessions()
   {
   }

   _rtnListSessions::~_rtnListSessions()
   {
   }

   const CHAR* _rtnListSessions::name ()
   {
      return NAME_LIST_SESSIONS ;
   }

   RTN_COMMAND_TYPE _rtnListSessions::type ()
   {
      return CMD_LIST_SESSIONS ;
   }

   INT32 _rtnListSessions::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnListSessions::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListSessions::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnListSessions::getIntrCMDName()
   {
      return CMD_NAME_LIST_SESSION_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessionsInner)
   /*
      _rtnListSessionsInner implement
   */
   _rtnListSessionsInner::_rtnListSessionsInner()
   {
   }
   _rtnListSessionsInner::~_rtnListSessionsInner()
   {
   }

   const CHAR *_rtnListSessionsInner::name ()
   {
      return CMD_NAME_LIST_SESSION_INTR ;
   }
   RTN_COMMAND_TYPE _rtnListSessionsInner::type ()
   {
      return CMD_LIST_SESSIONS ;
   }

   INT32 _rtnListSessionsInner::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnListSessionsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListSessionsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessionsCurrent)
   /*
      _rtnListSessionsCurrent implement
   */
   _rtnListSessionsCurrent::_rtnListSessionsCurrent()
   {
   }
   _rtnListSessionsCurrent::~_rtnListSessionsCurrent()
   {
   }

   const CHAR* _rtnListSessionsCurrent::name ()
   {
      return NAME_LIST_SESSIONS_CURRENT ;
   }

   RTN_COMMAND_TYPE _rtnListSessionsCurrent::type ()
   {
      return CMD_LIST_SESSIONS_CURRENT ;
   }

   INT32 _rtnListSessionsCurrent::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnListSessionsCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnListSessionsCurrent::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnListSessionsCurrent::getIntrCMDName()
   {
      return CMD_NAME_LIST_SESSIONCUR_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListSessionsCurrentInner)
   /*
      _rtnListSessionsCurrentInner implement
   */
   _rtnListSessionsCurrentInner::_rtnListSessionsCurrentInner()
   {
   }

   _rtnListSessionsCurrentInner::~_rtnListSessionsCurrentInner()
   {
   }

   const CHAR *_rtnListSessionsCurrentInner::name ()
   {
      return CMD_NAME_LIST_SESSIONCUR_INTR ;
   }
   RTN_COMMAND_TYPE _rtnListSessionsCurrentInner::type ()
   {
      return CMD_LIST_SESSIONS_CURRENT ;
   }

   INT32 _rtnListSessionsCurrentInner::_getFetchType() const
   {
      return RTN_FETCH_SESSION ;
   }

   BOOLEAN _rtnListSessionsCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnListSessionsCurrentInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListStorageUnits)
   /*
      _rtnListStorageUnits define
   */
   _rtnListStorageUnits::_rtnListStorageUnits()
   {
   }

   _rtnListStorageUnits::~_rtnListStorageUnits()
   {
   }

   const CHAR* _rtnListStorageUnits::name ()
   {
      return NAME_LIST_STORAGEUNITS ;
   }
   RTN_COMMAND_TYPE _rtnListStorageUnits::type ()
   {
      return CMD_LIST_STORAGEUNITS ;
   }

   INT32 _rtnListStorageUnits::_getFetchType() const
   {
      return RTN_FETCH_STORAGEUNIT ;
   }

   BOOLEAN _rtnListStorageUnits::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnListStorageUnits::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   const CHAR* _rtnListStorageUnits::getIntrCMDName()
   {
      return CMD_NAME_LIST_STORAGEUNIT_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListStorageUnitsInner)
   /*
      _rtnListStorageUnitsInner implement
   */
   _rtnListStorageUnitsInner::_rtnListStorageUnitsInner()
   {
   }

   _rtnListStorageUnitsInner::~_rtnListStorageUnitsInner()
   {
   }

   const CHAR *_rtnListStorageUnitsInner::name ()
   {
      return CMD_NAME_LIST_STORAGEUNIT_INTR ;
   }
   RTN_COMMAND_TYPE _rtnListStorageUnitsInner::type ()
   {
      return CMD_LIST_STORAGEUNITS ;
   }

   INT32 _rtnListStorageUnitsInner::_getFetchType() const
   {
      return RTN_FETCH_STORAGEUNIT ;
   }

   BOOLEAN _rtnListStorageUnitsInner::_isCurrent() const
   {
      if ( CMD_SPACE_SERVICE_LOCAL == getFromService() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _rtnListStorageUnitsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListBackups)
   /*
      _rtnListBackups implement
   */
   _rtnListBackups::_rtnListBackups()
   {
   }

   _rtnListBackups::~_rtnListBackups()
   {
   }

   const CHAR* _rtnListBackups::name ()
   {
      return NAME_LIST_BACKUPS ;
   }
   RTN_COMMAND_TYPE _rtnListBackups::type ()
   {
      return CMD_LIST_BACKUPS ;
   }

   INT32 _rtnListBackups::_getFetchType() const
   {
      return RTN_FETCH_BACKUP ;
   }

   BOOLEAN _rtnListBackups::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListBackups::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   BSONObj _rtnListBackups::_getOptObj() const
   {
      try
      {
         BSONObj hintObj( _hintBuff ) ;
         return hintObj ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
      return BSONObj() ;
   }

   const CHAR* _rtnListBackups::getIntrCMDName()
   {
      return CMD_NAME_LIST_BACKUP_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListBackupsInner)
   /*
      _rtnListBackupsInner implement
   */
   _rtnListBackupsInner::_rtnListBackupsInner()
   {
   }

   _rtnListBackupsInner::~_rtnListBackupsInner()
   {
   }

   const CHAR *_rtnListBackupsInner::name ()
   {
      return CMD_NAME_LIST_BACKUP_INTR ;
   }
   RTN_COMMAND_TYPE _rtnListBackupsInner::type ()
   {
      return CMD_LIST_BACKUPS ;
   }

   INT32 _rtnListBackupsInner::_getFetchType() const
   {
      return RTN_FETCH_BACKUP ;
   }

   BOOLEAN _rtnListBackupsInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListBackupsInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME ;
   }

   BSONObj _rtnListBackupsInner::_getOptObj() const
   {
      BSONObj obj ;
      try
      {
         BSONObjBuilder builder ;
         builder.appendBool( FIELD_NAME_DETAIL, TRUE ) ;
         obj = builder.obj() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      }
      return obj ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTrans)
   /*
      _rtnListTrans implement
   */
   _rtnListTrans::_rtnListTrans()
   {
   }

   _rtnListTrans::~_rtnListTrans()
   {
   }

   const CHAR* _rtnListTrans::name()
   {
      return CMD_NAME_LIST_TRANSACTIONS ;
   }

   RTN_COMMAND_TYPE _rtnListTrans::type()
   {
      return CMD_LIST_TRANS ;
   }

   INT32 _rtnListTrans::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnListTrans::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListTrans::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   const CHAR* _rtnListTrans::getIntrCMDName()
   {
      return CMD_NAME_LIST_TRANS_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTransInner)
   /*
      _rtnListTransInner implement
   */
   _rtnListTransInner::_rtnListTransInner()
   {
   }

   _rtnListTransInner::~_rtnListTransInner()
   {
   }

   const CHAR *_rtnListTransInner::name ()
   {
      return CMD_NAME_LIST_TRANS_INTR ;
   }
   RTN_COMMAND_TYPE _rtnListTransInner::type ()
   {
      return CMD_LIST_TRANS ;
   }

   INT32 _rtnListTransInner::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnListTransInner::_isCurrent() const
   {
      return FALSE ;
   }

   UINT32 _rtnListTransInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTransCurrent)
   /*
      _rtnListTransCurrent implement
   */
   _rtnListTransCurrent::_rtnListTransCurrent()
   {
   }

   _rtnListTransCurrent::~_rtnListTransCurrent()
   {
   }

   const CHAR* _rtnListTransCurrent::name()
   {
      return CMD_NAME_LIST_TRANSACTIONS_CUR ;
   }

   RTN_COMMAND_TYPE _rtnListTransCurrent::type()
   {
      return CMD_LIST_TRANS_CURRENT ;
   }

   INT32 _rtnListTransCurrent::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnListTransCurrent::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnListTransCurrent::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

   const CHAR* _rtnListTransCurrent::getIntrCMDName()
   {
      return CMD_NAME_LIST_TRANSCUR_INTR ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_rtnListTransCurrentInner)
   /*
      _rtnListTransCurrentInner implement
   */
   _rtnListTransCurrentInner::_rtnListTransCurrentInner()
   {
   }

   _rtnListTransCurrentInner::~_rtnListTransCurrentInner()
   {
   }

   const CHAR *_rtnListTransCurrentInner::name ()
   {
      return CMD_NAME_LIST_TRANSCUR_INTR ;
   }
   RTN_COMMAND_TYPE _rtnListTransCurrentInner::type ()
   {
      return CMD_LIST_TRANS_CURRENT ;
   }

   INT32 _rtnListTransCurrentInner::_getFetchType() const
   {
      return RTN_FETCH_TRANS ;
   }

   BOOLEAN _rtnListTransCurrentInner::_isCurrent() const
   {
      return TRUE ;
   }

   UINT32 _rtnListTransCurrentInner::_addInfoMask() const
   {
      return MON_MASK_NODE_NAME | MON_MASK_GROUP_NAME ;
   }

}


