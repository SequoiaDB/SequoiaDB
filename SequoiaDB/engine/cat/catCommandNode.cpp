/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = catCommandNode.hpp

   Descriptive Name = Catalogue commands for node

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for catalog
   commands.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/18/2022  LCX Initial Draft

   Last Changed =

*******************************************************************************/

#include "catalogueCB.hpp"
#include "catCommandNode.hpp"
#include "catLevelLock.hpp"
#include "catCommon.hpp"
#include "catTrace.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"

namespace engine
{
   /*
      _catCMDAlterNode implement
   */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDAlterNode ) ;

   _catCMDAlterNode::_catCMDAlterNode()
   : _groupID( CAT_INVALID_GROUPID ),
     _nodeID( CAT_INVALID_NODEID ),
     _pCatNodeMgr( sdbGetCatalogueCB()->getCatNodeMgr() )
   {
   }

   _catCMDAlterNode::~_catCMDAlterNode()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERNODE_INITNODEINFO, "_catCMDAlterNode::_initNodeInfo" )
   INT32 _catCMDAlterNode::_initNodeInfo( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERNODE_INITNODEINFO ) ;

      BSONObj groupObj, nodeList ;
      BSONElement svcEle ;
      string hostName ;
      string svcName ;

      // Get SYSCAT.SYSNODES
      rc = catGetGroupObj( _nodeID, groupObj, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get group info, rc: %d", rc ) ;

      // Init _groupName
      rc = rtnGetSTDStringElement( groupObj, FIELD_NAME_GROUPNAME, _groupName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                   FIELD_NAME_GROUPNAME, rc ) ;

      // Get groupArray
      rc = rtnGetArrayElement( groupObj, CAT_GROUP_NAME, nodeList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                   CAT_GROUP_NAME, rc ) ;

      try
      {
         // Get nodeObj
         BSONObjIterator nodeItr( nodeList ) ;
         while ( nodeItr.more() )
         {
            BSONElement nodeEle = nodeItr.next() ;
            BSONObj nodeObj = nodeEle.embeddedObject() ;

            BSONElement tmpNodeIDEle = nodeObj.getField( CAT_NODEID_NAME ) ;
            if ( tmpNodeIDEle.eoo() || !tmpNodeIDEle.isNumber() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDWARNING, "Failed to get the field(%s)", CAT_NODEID_NAME );
               goto error ;
            }
            if ( _nodeID == tmpNodeIDEle.numberInt() )
            {
               _nodeObj = nodeObj.getOwned() ;
               break ;
            }
         }
         PD_CHECK( ! _nodeObj.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get node[ID:%d] object", _nodeID ) ;

         // Get hostName
         rc = rtnGetSTDStringElement( _nodeObj, FIELD_NAME_HOST, hostName ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get field [%s], rc: %d",
                      FIELD_NAME_HOST, rc ) ;

         // Get svcName
         svcEle = _nodeObj.getField( FIELD_NAME_SERVICE ) ;
         if ( svcEle.eoo() || Array != svcEle.type() )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Failed to get field [%s], field type: %d",
                    FIELD_NAME_SERVICE, svcEle.type() ) ;
            goto error ;
         }
         svcName = getServiceName( svcEle, MSG_ROUTE_LOCAL_SERVICE ) ;

         // Init _nodeName
         _nodeName = hostName + ":" + svcName ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d",
                 e.what(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERNODE_INITNODEINFO, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERNODE_INIT, "_catCMDAlterNode::init" )
   INT32 _catCMDAlterNode::init( const CHAR *pQuery,
                                 const CHAR *pSelector,
                                 const CHAR *pOrderBy,
                                 const CHAR *pHint,
                                 INT32 flags,
                                 INT64 numToSkip,
                                 INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERNODE_INIT ) ;

      try
      {
         BSONObj            queryObj( pQuery ) ;
         BSONObj            hintObj( pHint ) ;
         BSONElement        ele ;

         // Get _groupID
         ele = hintObj.getField( FIELD_NAME_GROUPID ) ;
         if ( ele.eoo() || !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from hint object: %s",
                    FIELD_NAME_GROUPID, hintObj.toPoolString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _groupID = ele.numberInt() ;

         // Get _nodeID
         ele = hintObj.getField( FIELD_NAME_NODEID ) ;
         if ( ele.eoo() || !ele.isNumber() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from hint object: %s",
                    FIELD_NAME_NODEID, hintObj.toPoolString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _nodeID = ele.numberInt() ;

         // Get _actionName
         ele = queryObj.getField( FIELD_NAME_ACTION ) ;
         if ( ele.eoo() || String != ele.type() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_ACTION, queryObj.toPoolString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _actionName = ele.valuestrsafe() ;

         // Get option
         ele = queryObj.getField( FIELD_NAME_OPTIONS ) ;
         if ( ele.eoo() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from query object: %s",
                    FIELD_NAME_OPTIONS, queryObj.toPoolString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _option = ele.embeddedObject() ;

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d",
                 e.what(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERNODE_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERNODE_DOIT, "_catCMDAlterNode::doit" )
   INT32 _catCMDAlterNode::doit( _pmdEDUCB *cb,
                                 rtnContextBuf &ctxBuf,
                                 INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERNODE_DOIT ) ;

      BSONObjIterator itr ;
      catCtxLockMgr lockMgr ;

      // Init nodeInfo, _groupName, _nodeName
      rc = _initNodeInfo( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init node info, rc: %d", rc ) ;

      // Lock node
      PD_CHECK( lockMgr.tryLockNode( _groupName, _nodeName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock node [%s] on group [%s]",
                _nodeName.c_str(), _groupName.c_str() ) ;

      try
      {
         // Match Action
         if ( 0 == ossStrcmp( SDB_ALTER_NODE_SET_LOCATION, _actionName.c_str() ) )
         {
            // Check the groupID, only the node in db group can set location
            if ( CATALOG_GROUPID != _groupID && COORD_GROUPID != _groupID &&
               ( DATA_GROUP_ID_BEGIN > _groupID || DATA_GROUP_ID_END < _groupID ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Group [%s] doesn't support to set location",
                           _groupName.c_str() ) ;
               goto error ;
            }

            // Get newLocation
            BSONElement newLocEle = _option.getField( CAT_LOCATION_NAME ) ;
            if ( newLocEle.eoo() || String != newLocEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDWARNING, "Failed to get the field(%s)",
                           CAT_LOCATION_NAME ) ;
               goto error ;
            }
            // ele.valuestrsize include the length of '\0'
            if ( MSG_LOCATION_NAMESZ < newLocEle.valuestrsize() - 1 )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
               goto error ;
            }
            const ossPoolString newLoc( newLocEle.valuestrsafe() ) ;

            BSONElement oldLocEle = _nodeObj.getField( CAT_LOCATION_NAME ) ;
            if ( ! oldLocEle.eoo() && String != oldLocEle.type() )
            {
               rc = SDB_CAT_CORRUPTION ;
               PD_LOG( PDWARNING, "Failed to get the field(%s)",
                       CAT_LOCATION_NAME ) ;
               goto error ;
            }
            const ossPoolString oldLoc( oldLocEle.valuestrsafe() ) ;

            // Compare oldLocation and newLocation
            if ( oldLoc == newLoc )
            {
               PD_LOG( PDDEBUG, "The old and new location are same, do nothing" ) ;
               goto done ;
            }

            if ( ! newLoc.empty() )
            {
               rc = _pCatNodeMgr->setLocation( _nodeID, _groupID, newLoc ) ;
            }
            else
            {
               rc = _pCatNodeMgr->removeLocation( _nodeID, _groupID ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to set location, rc: %d", rc ) ;
         }
         else if ( 0 == ossStrcmp( SDB_ALTER_NODE_SET_ATTR, _actionName.c_str() ) )
         {
            // Parse options
            itr = BSONObjIterator( _option ) ;
            while ( itr.more() )
            {
               BSONElement optionEle = itr.next() ;
               if ( 0 == ossStrcmp( CAT_LOCATION_NAME, optionEle.fieldName() ) )
               {
                  // Check the groupID, only the node in db group can set location
                  if ( CATALOG_GROUPID != _groupID &&
                       COORD_GROUPID != _groupID &&
                       ( DATA_GROUP_ID_BEGIN > _groupID ||
                         DATA_GROUP_ID_END < _groupID ) )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Group [%s] doesn't support to "
                                 "set location", _groupName.c_str() ) ;
                     goto error ;
                  }

                  if ( optionEle.eoo() || String != optionEle.type() )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDWARNING, "Failed to get the field(%s)",
                                 CAT_LOCATION_NAME ) ;
                     goto error ;
                  }
                  // ele.valuestrsize include the length of '\0'
                  if ( MSG_LOCATION_NAMESZ < optionEle.valuesize() - 1 )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Size of location name is greater than 256B" ) ;
                     goto error ;
                  }
                  const ossPoolString newLoc( optionEle.valuestrsafe() ) ;

                  BSONElement oldLocEle = _nodeObj.getField( FIELD_NAME_LOCATION ) ;
                  if ( ! oldLocEle.eoo() && String != optionEle.type() )
                  {
                     PD_LOG( PDWARNING, "Failed to get the field(%s)",
                             CAT_LOCATION_NAME ) ;
                     rc = SDB_CAT_CORRUPTION ;
                     goto error ;
                  }

                  // Compare oldLocation and newLocation
                  const ossPoolString oldLoc( oldLocEle.valuestrsafe() ) ;
                  if ( oldLoc == newLoc )
                  {
                     PD_LOG( PDDEBUG,
                             "The old and new location are same, do nothing" ) ;
                     goto done ;
                  }

                  if ( ! newLoc.empty() )
                  {
                     rc = _pCatNodeMgr->setLocation( _nodeID, _groupID, newLoc ) ;
                  }
                  else
                  {
                     rc = _pCatNodeMgr->removeLocation( _nodeID, _groupID ) ;
                  }
                  PD_RC_CHECK( rc, PDERROR, "Failed to set location, "
                               "rc: %d", rc ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Invalid alter node option[%s]",
                          optionEle.fieldName() ) ;
                  goto error ;
               }
            }
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to alter node, "
                    "received unknown action[%s]", _actionName.c_str() ) ;
         }
      }
      catch( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }


   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERNODE_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDAlterRG implement
   */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDAlterRG ) ;

   _catCMDAlterRG::_catCMDAlterRG()
   {
   }

   _catCMDAlterRG::~_catCMDAlterRG()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERG_INIT, "_catCMDAlterRG::init" )
   INT32 _catCMDAlterRG::init( const CHAR *pQuery,
                               const CHAR *pSelector,
                               const CHAR *pOrderBy,
                               const CHAR *pHint,
                               INT32 flags,
                               INT64 numToSkip,
                               INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERG_INIT ) ;

      try
      {
         _queryObj.init( pQuery ) ;
         _hintObj.init( pHint ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERG_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CATCMDALTERRG_DOIT, "_catCMDAlterRG::doit" )
   INT32 _catCMDAlterRG::doit( _pmdEDUCB *cb,
                               rtnContextBuf &ctxBuf,
                               INT64 &contextID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CATCMDALTERRG_DOIT ) ;

      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      catCtxAlterGrp::sharePtr context ;

      rc = rtnCB->contextNew( RTN_CONTEXT_CAT_ALTER_GROUP, context, contextID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create alter group context, rc: %d", rc ) ;

      rc = context->open( MSG_BS_QUERY_REQ, _queryObj, _hintObj, ctxBuf, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open alter group context, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__CATCMDALTERRG_DOIT, rc ) ;
      return rc ;

   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
         contextID = -1 ;
      }
      goto done ;
   }

}