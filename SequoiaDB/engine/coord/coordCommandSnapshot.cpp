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

   Source File Name = coordCommandSnapshot.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09-23-2016  XJH Init
   Last Changed =

*******************************************************************************/
#include "coordCommandSnapshot.hpp"
#include "coordSnapshotDef.hpp"
#include "catDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDSnapshotIntrBase implement
   */
   _coordCMDSnapshotIntrBase::_coordCMDSnapshotIntrBase()
   {
   }

   _coordCMDSnapshotIntrBase::~_coordCMDSnapshotIntrBase()
   {
   }

   /*
      _coordCMDSnapshotCurIntrBase implement
   */
   _coordCMDSnapshotCurIntrBase::_coordCMDSnapshotCurIntrBase()
   {
   }

   _coordCMDSnapshotCurIntrBase::~_coordCMDSnapshotCurIntrBase()
   {
   }

   /*
      _coordCmdSnapshotReset implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdSnapshotReset,
                                      CMD_NAME_SNAPSHOT_RESET,
                                      TRUE ) ;
   _coordCmdSnapshotReset::_coordCmdSnapshotReset()
   {
   }

   _coordCmdSnapshotReset::~_coordCmdSnapshotReset()
   {
   }

   void _coordCmdSnapshotReset::_preSet( pmdEDUCB *cb,
                                         coordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   /*
      _coordSnapshotTransCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTransCurIntr,
                                      CMD_NAME_SNAPSHOT_TRANSCUR_INTR,
                                      TRUE ) ;
   _coordSnapshotTransCurIntr::_coordSnapshotTransCurIntr()
   {
   }

   _coordSnapshotTransCurIntr::~_coordSnapshotTransCurIntr()
   {
   }

   void _coordSnapshotTransCurIntr::_preSet( pmdEDUCB *cb,
                                             coordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      ctrlParam._useSpecialNode = TRUE ;
      DpsTransNodeMap *pMap = cb->getTransNodeLst() ;
      if ( pMap )
      {
         DpsTransNodeMap::iterator it = pMap->begin() ;
         while( it != pMap->end() )
         {
            ctrlParam._specialNodes.insert( it->second.value ) ;
            ++it ;
         }
      }
   }

   /*
      _coordSnapshotTransIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTransIntr,
                                      CMD_NAME_SNAPSHOT_TRANS_INTR,
                                      TRUE ) ;
   _coordSnapshotTransIntr::_coordSnapshotTransIntr()
   {
   }

   _coordSnapshotTransIntr::~_coordSnapshotTransIntr()
   {
   }

   void _coordSnapshotTransIntr::_preSet( pmdEDUCB *cb,
                                          coordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 0 ;
      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;
   }

   /*
      _coordSnapshotTransCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTransCur,
                                      CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR,
                                      TRUE ) ;
   _coordSnapshotTransCur::_coordSnapshotTransCur()
   {
   }

   _coordSnapshotTransCur::~_coordSnapshotTransCur()
   {
   }

   const CHAR* _coordSnapshotTransCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSCUR_INTR ;
   }

   const CHAR* _coordSnapshotTransCur::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordSnapshotTrans implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSnapshotTrans,
                                      CMD_NAME_SNAPSHOT_TRANSACTIONS,
                                      TRUE ) ;
   _coordSnapshotTrans::_coordSnapshotTrans()
   {
   }

   _coordSnapshotTrans::~_coordSnapshotTrans()
   {
   }

   const CHAR* _coordSnapshotTrans::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANS_INTR ;
   }

   const CHAR* _coordSnapshotTrans::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotDataBase implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotDataBase,
                                      CMD_NAME_SNAPSHOT_DATABASE,
                                      TRUE ) ;
   _coordCMDSnapshotDataBase::_coordCMDSnapshotDataBase()
   {
   }

   _coordCMDSnapshotDataBase::~_coordCMDSnapshotDataBase()
   {
   }

   const CHAR* _coordCMDSnapshotDataBase::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE_INTR;
   }

   const CHAR* _coordCMDSnapshotDataBase::getInnerAggrContent()
   {
      return COORD_SNAPSHOTDB_INPUT ;
   }

   /*
      _coordCMDSnapshotDataBaseIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotDataBaseIntr,
                                      CMD_NAME_SNAPSHOT_DATABASE_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotDataBaseIntr::_coordCMDSnapshotDataBaseIntr()
   {
   }

   _coordCMDSnapshotDataBaseIntr::~_coordCMDSnapshotDataBaseIntr()
   {
   }

   /*
      _coordCMDSnapshotSystem implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSystem,
                                      CMD_NAME_SNAPSHOT_SYSTEM,
                                      TRUE ) ;
   _coordCMDSnapshotSystem::_coordCMDSnapshotSystem()
   {
   }

   _coordCMDSnapshotSystem::~_coordCMDSnapshotSystem()
   {
   }

   const CHAR* _coordCMDSnapshotSystem::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SYSTEM_INTR;
   }

   const CHAR* _coordCMDSnapshotSystem::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSYS_INPUT ;
   }

   /*
      _coordCMDSnapshotSystemIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSystemIntr,
                                      CMD_NAME_SNAPSHOT_SYSTEM_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSystemIntr::_coordCMDSnapshotSystemIntr()
   {
   }

   _coordCMDSnapshotSystemIntr::~_coordCMDSnapshotSystemIntr()
   {
   }

   /*
      _coordCMDSnapshotHealth implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotHealth,
                                      CMD_NAME_SNAPSHOT_HEALTH,
                                      TRUE ) ;
   _coordCMDSnapshotHealth::_coordCMDSnapshotHealth()
   {
   }

   _coordCMDSnapshotHealth::~_coordCMDSnapshotHealth()
   {
   }

   const CHAR* _coordCMDSnapshotHealth::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_HEALTH_INTR ;
   }

   const CHAR* _coordCMDSnapshotHealth::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotHealthIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotHealthIntr,
                                      CMD_NAME_SNAPSHOT_HEALTH_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotHealthIntr::_coordCMDSnapshotHealthIntr()
   {
   }

   _coordCMDSnapshotHealthIntr::~_coordCMDSnapshotHealthIntr()
   {
   }

   void _coordCMDSnapshotHealthIntr::_preSet( pmdEDUCB *cb,
                                              coordCtrlParam &ctrlParam )
   {
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   /*
      _coordCMDSnapshotCollections implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCollections,
                                      CMD_NAME_SNAPSHOT_COLLECTIONS,
                                      TRUE ) ;
   _coordCMDSnapshotCollections::_coordCMDSnapshotCollections()
   {
   }

   _coordCMDSnapshotCollections::~_coordCMDSnapshotCollections()
   {
   }

   const CHAR* _coordCMDSnapshotCollections::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTION_INTR;
   }

   const CHAR* _coordCMDSnapshotCollections::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCL_INPUT ;
   }

   /*
      _coordCMDSnapshotCLIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCLIntr,
                                      CMD_NAME_SNAPSHOT_COLLECTION_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotCLIntr::_coordCMDSnapshotCLIntr()
   {
   }

   _coordCMDSnapshotCLIntr::~_coordCMDSnapshotCLIntr()
   {
   }

   /*
      _coordCMDSnapshotSpaces implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSpaces,
                                      CMD_NAME_SNAPSHOT_COLLECTIONSPACES,
                                      TRUE ) ;
   _coordCMDSnapshotSpaces::_coordCMDSnapshotSpaces()
   {
   }

   _coordCMDSnapshotSpaces::~_coordCMDSnapshotSpaces()
   {
   }

   const CHAR* _coordCMDSnapshotSpaces::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SPACE_INTR;
   }

   const CHAR* _coordCMDSnapshotSpaces::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCS_INPUT ;
   }

   /*
      _coordCMDSnapshotCSIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCSIntr,
                                      CMD_NAME_SNAPSHOT_SPACE_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotCSIntr::_coordCMDSnapshotCSIntr()
   {
   }

   _coordCMDSnapshotCSIntr::~_coordCMDSnapshotCSIntr()
   {
   }

   /*
      _coordCMDSnapshotContexts implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContexts,
                                      CMD_NAME_SNAPSHOT_CONTEXTS,
                                      TRUE ) ;
   _coordCMDSnapshotContexts::_coordCMDSnapshotContexts()
   {
   }

   _coordCMDSnapshotContexts::~_coordCMDSnapshotContexts()
   {
   }

   const CHAR* _coordCMDSnapshotContexts::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXT_INTR;
   }

   const CHAR* _coordCMDSnapshotContexts::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCONTEXTS_INPUT ;
   }

   /*
      _coordCMDSnapshotContextIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContextIntr,
                                      CMD_NAME_SNAPSHOT_CONTEXT_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotContextIntr::_coordCMDSnapshotContextIntr()
   {
   }

   _coordCMDSnapshotContextIntr::~_coordCMDSnapshotContextIntr()
   {
   }

   /*
      _coordCMDSnapshotContextsCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContextsCur,
                                      CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT,
                                      TRUE ) ;
   _coordCMDSnapshotContextsCur::_coordCMDSnapshotContextsCur()
   {
   }

   _coordCMDSnapshotContextsCur::~_coordCMDSnapshotContextsCur()
   {
   }

   const CHAR* _coordCMDSnapshotContextsCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR;
   }

   const CHAR* _coordCMDSnapshotContextsCur::getInnerAggrContent()
   {
      return COORD_SNAPSHOTCONTEXTSCUR_INPUT ;
   }

   /*
      _coordCMDSnapshotContextCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotContextCurIntr,
                                      CMD_NAME_SNAPSHOT_CONTEXTCUR_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotContextCurIntr::_coordCMDSnapshotContextCurIntr()
   {
   }

   _coordCMDSnapshotContextCurIntr::~_coordCMDSnapshotContextCurIntr()
   {
   }

   /*
      _coordCMDSnapshotSessions implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessions,
                                      CMD_NAME_SNAPSHOT_SESSIONS,
                                      TRUE ) ;
   _coordCMDSnapshotSessions::_coordCMDSnapshotSessions()
   {
   }

   _coordCMDSnapshotSessions::~_coordCMDSnapshotSessions()
   {
   }

   const CHAR* _coordCMDSnapshotSessions::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSION_INTR;
   }

   const CHAR* _coordCMDSnapshotSessions::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSESS_INPUT ;
   }

   /*
      _coordCMDSnapshotSessionIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessionIntr,
                                      CMD_NAME_SNAPSHOT_SESSION_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSessionIntr::_coordCMDSnapshotSessionIntr()
   {
   }

   _coordCMDSnapshotSessionIntr::~_coordCMDSnapshotSessionIntr()
   {
   }

   /*
      _coordCMDSnapshotSessionsCur implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessionsCur,
                                      CMD_NAME_SNAPSHOT_SESSIONS_CURRENT,
                                      TRUE ) ;
   _coordCMDSnapshotSessionsCur::_coordCMDSnapshotSessionsCur()
   {
   }

   _coordCMDSnapshotSessionsCur::~_coordCMDSnapshotSessionsCur()
   {
   }

   const CHAR* _coordCMDSnapshotSessionsCur::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONCUR_INTR;
   }

   const CHAR* _coordCMDSnapshotSessionsCur::getInnerAggrContent()
   {
      return COORD_SNAPSHOTSESSCUR_INPUT ;
   }

   /*
      _coordCMDSnapshotSessionCurIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotSessionCurIntr,
                                      CMD_NAME_SNAPSHOT_SESSIONCUR_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotSessionCurIntr::_coordCMDSnapshotSessionCurIntr()
   {
   }

   _coordCMDSnapshotSessionCurIntr::~_coordCMDSnapshotSessionCurIntr()
   {
   }

   /*
      _coordCMDSnapshotCata implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCata,
                                      CMD_NAME_SNAPSHOT_CATA,
                                      TRUE ) ;
   _coordCMDSnapshotCata::_coordCMDSnapshotCata()
   {
   }

   _coordCMDSnapshotCata::~_coordCMDSnapshotCata()
   {
   }

   INT32 _coordCMDSnapshotCata::_preProcess( rtnQueryOptions &queryOpt,
                                             string &clName,
                                             BSONObj &outSelector )
   {
      clName = CAT_COLLECTION_INFO_COLLECTION ;
      return SDB_OK ;
   }

   /*
      _coordCMDSnapshotCataIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotCataIntr,
                                      CMD_NAME_SNAPSHOT_CATA_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotCataIntr::_coordCMDSnapshotCataIntr()
   {
   }

   _coordCMDSnapshotCataIntr::~_coordCMDSnapshotCataIntr()
   {
   }

   /*
      _coordCMDSnapshotAccessPlans implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotAccessPlans,
                                      CMD_NAME_SNAPSHOT_ACCESSPLANS,
                                      TRUE ) ;
   _coordCMDSnapshotAccessPlans::_coordCMDSnapshotAccessPlans ()
   {
   }

   _coordCMDSnapshotAccessPlans::~_coordCMDSnapshotAccessPlans ()
   {
   }

   const CHAR* _coordCMDSnapshotAccessPlans::getIntrCMDName ()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR ;
   }

   const CHAR* _coordCMDSnapshotAccessPlans::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotAccessPlansIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotAccessPlansIntr,
                                      CMD_NAME_SNAPSHOT_ACCESSPLANS_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotAccessPlansIntr::_coordCMDSnapshotAccessPlansIntr ()
   {
   }

   _coordCMDSnapshotAccessPlansIntr::~_coordCMDSnapshotAccessPlansIntr ()
   {
   }

   /*
      _coordCMDSnapshotConfigs implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotConfigs,
                                      CMD_NAME_SNAPSHOT_CONFIGS,
                                      TRUE ) ;
   _coordCMDSnapshotConfigs::_coordCMDSnapshotConfigs()
   {
   }

   _coordCMDSnapshotConfigs::~_coordCMDSnapshotConfigs()
   {
   }

   const CHAR* _coordCMDSnapshotConfigs::getIntrCMDName()
   {
      return CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONFIGS_INTR;
   }

   const CHAR* _coordCMDSnapshotConfigs::getInnerAggrContent()
   {
      return NULL ;
   }

   /*
      _coordCMDSnapshotConfigsIntr implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSnapshotConfigsIntr,
                                      CMD_NAME_SNAPSHOT_CONFIGS_INTR,
                                      TRUE ) ;
   _coordCMDSnapshotConfigsIntr::_coordCMDSnapshotConfigsIntr()
   {
   }

   _coordCMDSnapshotConfigsIntr::~_coordCMDSnapshotConfigsIntr()
   {
   }

}

