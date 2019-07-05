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

   Source File Name = coordCommandWithLocation.cpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   user command processing on coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "coordCommandWithLocation.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "dms.hpp"
#include "msgMessage.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDExpConfig implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDExpConfig,
                                      CMD_NAME_EXPORT_CONFIG,
                                      TRUE ) ;
   _coordCMDExpConfig::_coordCMDExpConfig()
   {
   }

   _coordCMDExpConfig::~_coordCMDExpConfig()
   {
   }

   INT32 _coordCMDExpConfig::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void _coordCMDExpConfig::_preSet( pmdEDUCB *cb,
                                     coordCtrlParam &ctrlParam ) 
   {
      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
   }

   UINT32 _coordCMDExpConfig::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   INT32 _coordCMDExpConfig::_posExcute( MsgHeader *pMsg,
                                         pmdEDUCB *cb,
                                         ROUTE_RC_MAP &faileds )
   {
      UINT32 mask = 0 ;
      INT32 rc = SDB_OK ;
      CHAR *pMatcherBuff = NULL ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pMatcherBuff, NULL, NULL, NULL ) ;

      try
      {
         BSONObj matcher( pMatcherBuff ) ;
         BSONElement e = matcher.getField( FIELD_NAME_TYPE ) ;
         if ( e.eoo() )
         {
            mask = PMD_CFG_MASK_SKIP_UNFIELD ;
         }
         else if ( e.isNumber() )
         {
            INT32 type = e.numberInt() ;
            switch( type )
            {
               case 0 :
                  mask = 0 ;
                  break ;
               case 1 :
                  mask = PMD_CFG_MASK_SKIP_HIDEDFT ;
                  break ;
               case 2 :
                  mask = PMD_CFG_MASK_SKIP_HIDEDFT | PMD_CFG_MASK_SKIP_NORMALDFT ;
                  break ;
               default :
                  mask = PMD_CFG_MASK_SKIP_UNFIELD ;
                  break ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Field[%s] should be numberInt",
                    e.toString( TRUE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = pmdGetKRCB()->getOptionCB()->reflush2File( mask ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Flush local config to file failed, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCMDInvalidateCache implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDInvalidateCache,
                                      CMD_NAME_INVALIDATE_CACHE,
                                      TRUE ) ;
   _coordCMDInvalidateCache::_coordCMDInvalidateCache()
   {
   }

   _coordCMDInvalidateCache::~_coordCMDInvalidateCache()
   {
   }

   void _coordCMDInvalidateCache::_preSet( pmdEDUCB * cb,
                                           coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   INT32 _coordCMDInvalidateCache::_preExcute( MsgHeader *pMsg,
                                               pmdEDUCB *cb,
                                               coordCtrlParam &ctrlParam,
                                               SET_RC &ignoreRCList )
   {
      _pResource->invalidateCataInfo() ;
      _pResource->invalidateGroupInfo() ;
      _pResource->invalidateStrategy() ;
      return SDB_OK ;
   }

   UINT32 _coordCMDInvalidateCache::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   /*
      _coordCMDSyncDB implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDSyncDB,
                                      CMD_NAME_SYNC_DB,
                                      TRUE ) ;
   _coordCMDSyncDB::_coordCMDSyncDB()
   {
   }

   _coordCMDSyncDB::~_coordCMDSyncDB()
   {
   }

   void _coordCMDSyncDB::_preSet( pmdEDUCB *cb,
                                  coordCtrlParam &ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   UINT32 _coordCMDSyncDB::_getControlMask() const
   {
      return COORD_CTRL_MASK_NODE_SELECT|COORD_CTRL_MASK_ROLE ;
   }

   INT32 _coordCMDSyncDB::_preExcute( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      coordCtrlParam &ctrlParam,
                                      SET_RC &ignoreRCList )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CHAR *pNewMsg = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         const CHAR *csName = NULL ;
         BSONObj obj( pQuery ) ;
         BSONElement e = obj.getField( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( String == e.type() )
         {
            csName = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTIONSPACE, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( csName )
         {
            INT32 newMsgSize = 0 ;
            CoordGroupList grpLst ;
            rtnQueryOptions queryOpt ;

            queryOpt.setCLFullName( "CAT" ) ;
            queryOpt.setQuery( BSON( CAT_COLLECTION_SPACE_NAME << csName ) ) ;
            rc = queryOpt.toQueryMsg( &pNewMsg, newMsgSize, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc query msg failed, rc: %d", rc ) ;
               goto error ;
            }
            ((MsgHeader*)pNewMsg)->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
            rc = executeOnCataGroup( (MsgHeader*)pNewMsg, cb, &grpLst ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Query collectionspace[%s] info from catalog "
                       "failed, rc: %d", csName, rc ) ;
               goto error ;
            }
            ctrlParam._useSpecialGrp = TRUE ;
            ctrlParam._specialGrps = grpLst ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( pNewMsg )
      {
         msgReleaseBuffer( pNewMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdLoadCS implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCmdLoadCS,
                                      CMD_NAME_LOAD_COLLECTIONSPACE,
                                      FALSE ) ;
   _coordCmdLoadCS::_coordCmdLoadCS()
   {
   }

   _coordCmdLoadCS::~_coordCmdLoadCS()
   {
   }

   void _coordCmdLoadCS::_preSet( pmdEDUCB * cb,
                                  coordCtrlParam &ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
   }

   UINT32 _coordCmdLoadCS::_getControlMask() const
   {
      return COORD_CTRL_MASK_NODE_SELECT ;
   }

   INT32 _coordCmdLoadCS::_preExcute( MsgHeader * pMsg,
                                      pmdEDUCB * cb,
                                      coordCtrlParam &ctrlParam,
                                      SET_RC &ignoreRCList )
   {
      INT32 rc = SDB_OK ;
      CHAR *pQuery = NULL ;
      CHAR *pNewMsg = NULL ;
      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }
      try
      {
         const CHAR *csName = NULL ;
         BSONObj obj( pQuery ) ;
         BSONElement e = obj.getField( FIELD_NAME_NAME ) ;
         if ( String == e.type() )
         {
            csName = e.valuestr() ;
         }
         else
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         {
            INT32 newMsgSize = 0 ;
            CoordGroupList grpLst ;
            rtnQueryOptions queryOpt ;

            queryOpt.setCLFullName( "CAT" ) ;
            queryOpt.setQuery( BSON( CAT_COLLECTION_SPACE_NAME << csName ) ) ;
            rc = queryOpt.toQueryMsg( &pNewMsg, newMsgSize, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Alloc query msg failed, rc: %d", rc ) ;
               goto error ;
            }
            ((MsgHeader*)pNewMsg)->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
            rc = executeOnCataGroup( (MsgHeader*)pNewMsg, cb, &grpLst ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Query collectionspace[%s] info from catalog "
                       "failed, rc: %d", csName, rc ) ;
               goto error ;
            }
            ctrlParam._useSpecialGrp = TRUE ;
            ctrlParam._specialGrps = grpLst ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( pNewMsg )
      {
         msgReleaseBuffer( pNewMsg, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _coordCmdUnloadCS implement
   */
   _coordCommandAssit _coordCmdUnloadCSAssit ( CMD_NAME_UNLOAD_COLLECTIONSPACE,
      FALSE, (COORD_NEW_OPERATOR)_coordCmdUnloadCS::newThis ) ;

   /*
      _coordForceSession implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordForceSession,
                                      CMD_NAME_FORCE_SESSION,
                                      TRUE ) ;
   _coordForceSession::_coordForceSession()
   {
   }

   _coordForceSession::~_coordForceSession()
   {
   }

   INT32 _coordForceSession::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void _coordForceSession::_preSet( pmdEDUCB * cb,
                                     coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   UINT32 _coordForceSession::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   /*
      _coordSetPDLevel implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordSetPDLevel,
                                      CMD_NAME_SET_PDLEVEL,
                                      TRUE ) ;
   _coordSetPDLevel::_coordSetPDLevel()
   {
   }

   _coordSetPDLevel::~_coordSetPDLevel()
   {
   }

   INT32 _coordSetPDLevel::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void _coordSetPDLevel::_preSet( pmdEDUCB * cb,
                                   coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = FALSE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
   }

   UINT32 _coordSetPDLevel::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   /*
      _coordReloadConf implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordReloadConf,
                                      CMD_NAME_RELOAD_CONFIG,
                                      TRUE ) ;
   _coordReloadConf::_coordReloadConf()
   {
   }

   _coordReloadConf::~_coordReloadConf()
   {
   }

   INT32 _coordReloadConf::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void _coordReloadConf::_preSet( pmdEDUCB * cb,
                                   coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   UINT32 _coordReloadConf::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   /*
      _coordUpdateConf implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordUpdateConf,
                                      CMD_NAME_UPDATE_CONFIG,
                                      TRUE ) ;
   _coordUpdateConf::_coordUpdateConf()
   {
   }

   _coordUpdateConf::~_coordUpdateConf()
   {
   }

   INT32 _coordUpdateConf::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void _coordUpdateConf::_preSet( pmdEDUCB * cb,
                                   coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   UINT32 _coordUpdateConf::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   /*
      _coordDeleteConf implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordDeleteConf,
                                      CMD_NAME_DELETE_CONFIG,
                                      TRUE ) ;
   _coordDeleteConf::_coordDeleteConf()
   {
   }

   _coordDeleteConf::~_coordDeleteConf()
   {
   }

   INT32 _coordDeleteConf::_onLocalMode( INT32 flag )
   {
      return SDB_COORD_UNKNOWN_OP_REQ ;
   }

   void _coordDeleteConf::_preSet( pmdEDUCB * cb,
                                   coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam._role[ SDB_ROLE_CATALOG ] = 1 ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   UINT32 _coordDeleteConf::_getControlMask() const
   {
      return COORD_CTRL_MASK_ALL ;
   }

   /*
      _coordCMDAnalyze implement
    */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDAnalyze,
                                      CMD_NAME_ANALYZE,
                                      TRUE ) ;

   _coordCMDAnalyze::_coordCMDAnalyze  ()
   {
   }

   _coordCMDAnalyze::~_coordCMDAnalyze()
   {
   }

   void _coordCMDAnalyze::_preSet( pmdEDUCB *cb,
                                   coordCtrlParam &ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;

      ctrlParam._filterID = FILTER_ID_MATCHER ;

      ctrlParam._emptyFilterSel = NODE_SEL_PRIMARY ;

      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_DATA ] = 1 ;
   }

   UINT32 _coordCMDAnalyze::_getControlMask() const
   {
      return COORD_CTRL_MASK_NODE_SELECT ;
   }

   INT32 _coordCMDAnalyze::_preExcute ( MsgHeader *pMsg,
                                        pmdEDUCB *cb,
                                        coordCtrlParam &ctrlParam,
                                        SET_RC &ignoreRCList )
   {
      INT32 rc = SDB_OK ;

      CHAR *pQuery = NULL ;
      const CHAR *csname = NULL ;
      const CHAR *clname = NULL ;
      const CHAR *ixname = NULL ;
      INT32 mode = SDB_ANALYZE_MODE_SAMPLE ;
      BOOLEAN sampleByNum = FALSE, sampleByPercent = FALSE ;

      rc = msgExtractQuery( (CHAR*)pMsg, NULL, NULL, NULL, NULL,
                            &pQuery, NULL, NULL, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Extract message failed, rc: %d", rc ) ;
         goto error ;
      }

      try
      {
         BSONObj obj( pQuery ) ;
         BSONElement e ;

         e = obj.getField( FIELD_NAME_COLLECTIONSPACE ) ;
         if ( String == e.type() )
         {
            csname = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTION, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = obj.getField( FIELD_NAME_COLLECTION ) ;
         if ( String == e.type() )
         {
            clname = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_COLLECTIONSPACE, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = obj.getField( FIELD_NAME_INDEX ) ;
         if ( String == e.type() )
         {
            ixname = e.valuestr() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_INDEX, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = obj.getField( FIELD_NAME_ANALYZE_MODE ) ;
         if ( NumberInt == e.type() )
         {
            mode = e.numberInt() ;
            if ( SDB_ANALYZE_MODE_SAMPLE == mode ||
                 SDB_ANALYZE_MODE_FULL == mode ||
                 SDB_ANALYZE_MODE_GENDFT == mode ||
                 SDB_ANALYZE_MODE_RELOAD == mode ||
                 SDB_ANALYZE_MODE_CLEAR == mode )
            {
            }
            else
            {
               PD_LOG( PDERROR, "Value of field[%s] is invalid",
                       FIELD_NAME_ANALYZE_MODE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_ANALYZE_MODE, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         e = obj.getField( FIELD_NAME_ANALYZE_NUM ) ;
         if ( NumberInt == e.type() )
         {
            UINT32 sampleNum = (UINT32)e.numberInt() ;
            if ( sampleNum > SDB_ANALYZE_SAMPLE_MAX ||
                 sampleNum < SDB_ANALYZE_SAMPLE_MIN )
            {
               PD_LOG( PDERROR, "Field[%s] %u is out of range [ %d - %d ]",
                       FIELD_NAME_ANALYZE_NUM, sampleNum,
                       SDB_ANALYZE_SAMPLE_MIN, SDB_ANALYZE_SAMPLE_MAX ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            sampleByNum = TRUE ;
         }

         e = obj.getField( FIELD_NAME_ANALYZE_PERCENT ) ;
         if ( NumberInt == e.type() )
         {
            double samplePercent = e.numberInt() ;
            if ( samplePercent > 100.0 ||
                 samplePercent <= 0.0 )
            {
               PD_LOG( PDERROR, "Field[%s] %.2f is out of range ( %.2f - %.2f ]",
                       FIELD_NAME_ANALYZE_PERCENT, samplePercent, 0.0, 100.0 ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            sampleByPercent = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( NULL != csname )
      {
         if ( NULL != clname )
         {
            PD_LOG( PDERROR, "Field[%s] and Field[%s] conflict",
                    FIELD_NAME_COLLECTIONSPACE, FIELD_NAME_COLLECTION ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( NULL != ixname )
         {
            PD_LOG( PDERROR, "Field[%s] and Field[%s] conflict",
                    FIELD_NAME_COLLECTIONSPACE, FIELD_NAME_INDEX ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( NULL != ixname && NULL == clname )
      {
         PD_LOG( PDERROR, "Field[%s] requires Field[%s]",
                 FIELD_NAME_INDEX, FIELD_NAME_COLLECTION ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_ANALYZE_MODE_GENDFT == mode )
      {
         PD_CHECK( NULL != clname, SDB_INVALIDARG, error, PDERROR,
                   "Only support generating default statistics on specified "
                   "collection or index" ) ;
      }

      if ( sampleByNum && sampleByPercent )
      {
         PD_LOG( PDERROR, "Field[%s] and Field[%s] conflict",
                 FIELD_NAME_ANALYZE_NUM, FIELD_NAME_ANALYZE_PERCENT ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( NULL != csname )
      {
         rc = _getCSGrps( csname, cb, ctrlParam ) ;
         PD_RC_CHECK( rc, PDERROR, "Get groups of collectionspace[%s], "
                      "rc: %d", csname, rc ) ;
         ignoreRCList.insert( SDB_DMS_CS_NOTEXIST ) ;
      }
      else if ( NULL != clname )
      {
         rc = _getCLGrps( pMsg, clname, cb, ctrlParam ) ;
         PD_RC_CHECK( rc, PDERROR, "Get groups of collection[%s], rc: %d",
                      clname, rc ) ;
      }

      if ( SDB_ANALYZE_MODE_RELOAD == mode ||
           SDB_ANALYZE_MODE_CLEAR == mode )
      {
         ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDAnalyze::_getCSGrps ( const CHAR *csname, pmdEDUCB *cb,
                                        coordCtrlParam &ctrlParam )
   {
      INT32 rc = SDB_OK ;

      CHAR *pNewMsg = NULL ;
      INT32 newMsgSize = 0 ;

      CoordGroupList grpLst ;
      rtnQueryOptions queryOpt ;

      PD_CHECK( !dmsIsSysCSName( csname ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection space [%s]", csname ) ;

      queryOpt.setCLFullName( "CAT" ) ;
      queryOpt.setQuery( BSON( CAT_COLLECTION_SPACE_NAME << csname ) ) ;
      rc = queryOpt.toQueryMsg( &pNewMsg, newMsgSize, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Alloc query msg failed, rc: %d", rc ) ;

      ((MsgHeader*)pNewMsg)->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
      rc = executeOnCataGroup( (MsgHeader*)pNewMsg, cb, &grpLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Query collectionspace[%s] info from catalog "
                   "failed, rc: %d", csname, rc ) ;

      ctrlParam._useSpecialGrp = TRUE ;
      ctrlParam._specialGrps = grpLst ;

   done :
      if ( pNewMsg )
      {
         msgReleaseBuffer( pNewMsg, cb ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _coordCMDAnalyze::_getCLGrps ( MsgHeader *pMsg,
                                        const CHAR *clname,
                                        pmdEDUCB *cb,
                                        coordCtrlParam &ctrlParam )
   {
      INT32 rc = SDB_OK ;

      coordCataSel cataSel ;
      CoordGroupList grpLst, exceptLst ;
      MsgOpQuery *pRequest = (MsgOpQuery *)pMsg ;

      CHAR csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
      CHAR clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

      rc = rtnResolveCollectionName( clname, ossStrlen( clname ),
                                     csName, DMS_COLLECTION_SPACE_NAME_SZ,
                                     clShortName, DMS_COLLECTION_NAME_SZ ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to resolve collection name [%s], "
                    "rc: %d", clname, rc ) ;

      PD_CHECK( !dmsIsSysCSName( csName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection space [%s]", csName ) ;

      PD_CHECK( !dmsIsSysCLName( clShortName ), SDB_INVALIDARG, error, PDERROR,
                "Could not analyze SYS collection [%s]", clShortName ) ;

      rc = cataSel.bind( _pResource, clname, cb, TRUE, TRUE ) ;
      PD_RC_CHECK( rc, PDERROR, "Update collection[%s]'s catalog info failed, "
                   "rc: %d", clname, rc ) ;


      rc = cataSel.getGroupLst( cb, exceptLst, grpLst ) ;
      PD_RC_CHECK( rc, PDERROR, "Get collection[%s]'s group list failed, "
                   "rc: %d", clname, rc ) ;

      pRequest->version = cataSel.getCataPtr()->getVersion() ;

      ctrlParam._useSpecialGrp = TRUE ;
      ctrlParam._specialGrps = grpLst ;

   done :
      return rc ;
   error :
      goto done ;
   }

}
