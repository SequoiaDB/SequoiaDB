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

   Source File Name = catRecycleReturnInfo.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "catCommon.hpp"
#include "catRecycleReturnInfo.hpp"
#include "catCommand.hpp"
#include "catalogueCB.hpp"
#include "rtn.hpp"
#include "catGTSDef.hpp"
#include "clsCatalogAgent.hpp"
#include "../bson/bson.hpp"
#include "pdTrace.hpp"
#include "catTrace.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _catRecycleReturnInfo implement
    */
   _catRecycleReturnInfo::_catRecycleReturnInfo()
   : _isOnSite( FALSE )
   {
   }

   _catRecycleReturnInfo::~_catRecycleReturnInfo()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_CHKRTRNCL2CS, "_catRecycleReturnInfo::checkReturnCLToCS" )
   INT32 _catRecycleReturnInfo::checkReturnCLToCS( const CHAR *clName,
                                                   utilCLUniqueID clUniqueID,
                                                   BOOLEAN allowRename,
                                                   pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_CHKRTRNCL2CS ) ;

      utilCSUniqueID csUniqueID = utilGetCSUniqueID( clUniqueID ) ;

      if ( _checkCSSet.find( csUniqueID ) == _checkCSSet.end() )
      {
         CHAR originCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { '\0' } ;
         BSONObj boSpace ;
         const CHAR *curCSName = NULL ;

         // use unique ID of collection space, since the collection space
         // may be renamed
         rc = catGetAndLockCollectionSpace( csUniqueID, boSpace, curCSName,
                                            cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get collection space [%llu], "
                      "rc: %d", csUniqueID, rc ) ;

         // check origin name of collection space
         rc = rtnResolveCollectionSpaceName( clName,
                                             ossStrlen( clName ),
                                             originCSName,
                                             DMS_COLLECTION_SPACE_NAME_SZ ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection space name "
                      "from collection [%s], rc: %d", clName, rc ) ;

         // not the same, collection space has been renamed
         if ( 0 != ossStrcmp( originCSName, curCSName ) )
         {
            if ( allowRename )
            {
               PD_LOG( PDDEBUG, "Collection space [%s] has been "
                       "renamed to [%s]", originCSName, curCSName ) ;
            }
            else
            {
               PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                                 "collection space [%s] has been "
                                 "renamed to [%s]", originCSName, curCSName ) ;
            }

            rc = addRenameCS( originCSName, curCSName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save rename collection "
                         "space, rc: %d", rc ) ;
         }

         // save a cache, so no need to fetch for the next time
         try
         {
            _checkCSSet.insert( csUniqueID ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add return collection space info, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_CHKRTRNCL2CS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDCONFLICTCS, "_catRecycleReturnInfo::addConflictCS" )
   INT32 _catRecycleReturnInfo::addConflictCS( const CHAR *csName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDCONFLICTCS ) ;

      try
      {
         _conflictCSSet.insert( csName ) ;
         PD_LOG( PDWARNING, "Found conflict collection space [%s]", csName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add conflict collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDCONFLICTCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDCONFLICTNAMECL, "_catRecycleReturnInfo::addConflictNameCL" )
   INT32 _catRecycleReturnInfo::addConflictNameCL( utilCLUniqueID clUniqueID,
                                                   const utilReturnNameInfo &clNameInfo )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDCONFLICTNAMECL ) ;

      try
      {
         catCheckCLInfo clInfo( clUniqueID, clNameInfo ) ;
         _conflictNameCL.insert( make_pair( clInfo.getReturnName(), clInfo ) ) ;
         PD_LOG( PDWARNING, "Found conflict collection [return %s, origin %s]",
                 clInfo.getReturnName().c_str(),
                 clInfo.getOriginName().c_str() ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add conflict name collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDCONFLICTNAMECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDCONFLICTUIDCL, "_catRecycleReturnInfo::addConflictUIDCL" )
   INT32 _catRecycleReturnInfo::addConflictUIDCL( utilCLUniqueID clUniqueID,
                                                  const CHAR *originName,
                                                  const CHAR *conflictName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDCONFLICTUIDCL ) ;

      try
      {
         utilReturnNameInfo tmpNameInfo( originName ) ;
         tmpNameInfo.setReturnName( conflictName ) ;
         catCheckCLInfo clInfo( clUniqueID, tmpNameInfo ) ;
         _conflictUIDCL.insert( make_pair( clUniqueID, clInfo ) ) ;
         PD_LOG( PDWARNING, "Found conflict collection [%s], "
                 "unique ID [%llu]", originName, clUniqueID ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add conflict unique ID collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDCONFLICTUIDCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ISCONFLICTNAMECL, "_catRecycleReturnInfo::isConflictNameCL" )
   BOOLEAN _catRecycleReturnInfo::isConflictNameCL( const CHAR *clName )
   {
      BOOLEAN isConflict = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ISCONFLICTNAMECL ) ;

      try
      {
         if ( _conflictNameCL.find( clName ) != _conflictNameCL.end() )
         {
            isConflict = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to checkout conflict collection, "
                 "occur exception %s", e.what() ) ;

         // iterate one by one
         for ( _CAT_NAME_CL_MAP::iterator iter = _conflictNameCL.begin() ;
               iter != _conflictNameCL.end() ;
               ++ iter )
         {
            INT32 res = ossStrcmp( iter->first.c_str(), clName ) ;
            if ( 0 == res )
            {
               isConflict = TRUE ;
               break ;
            }
            else if ( res > 0 )
            {
               // end search
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_ISCONFLICTNAMECL ) ;

      return isConflict ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ISCONFLICTUIDCL, "_catRecycleReturnInfo::isConflictUIDCL" )
   BOOLEAN _catRecycleReturnInfo::isConflictUIDCL( utilCLUniqueID clUniqueID )
   {
      BOOLEAN isConflict = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ISCONFLICTUIDCL ) ;

      if ( _conflictUIDCL.find( clUniqueID ) != _conflictUIDCL.end() )
      {
         isConflict = TRUE ;
      }

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_ISCONFLICTUIDCL ) ;

      return isConflict ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_LOCKREPLACECS, "_catRecycleReturnInfo::lockReplaceCS" )
   INT32 _catRecycleReturnInfo::lockReplaceCS( catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_LOCKREPLACECS ) ;

      for ( UTIL_RETURN_NAME_SET_IT iter = _replaceCSSet.begin() ;
            iter != _replaceCSSet.end() ;
            ++ iter )
      {
         const CHAR *csName = iter->c_str() ;
         PD_CHECK( lockMgr.tryLockCollectionSpace( csName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock dropped collection space [%s]", csName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_LOCKREPLACECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_LOCKREPLACECL, "_catRecycleReturnInfo::lockReplaceCL" )
   INT32 _catRecycleReturnInfo::lockReplaceCL( catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_LOCKREPLACECL ) ;

      for ( UTIL_RETURN_NAME_SET_IT iter = _replaceCLSet.begin() ;
            iter != _replaceCLSet.end() ;
            ++ iter )
      {
         const CHAR *clName = iter->c_str() ;
         PD_CHECK( lockMgr.tryLockCollection( clName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock dropped collection [%s]", clName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_LOCKREPLACECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDRENAMECSBYRECYID, "_catRecycleReturnInfo::addRenameCSByRecyID" )
   INT32 _catRecycleReturnInfo::addRenameCSByRecyID( const CHAR *csName,
                                                     utilRecycleID recycleID,
                                                     catCtxLockMgr &lockMgr,
                                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDRENAMECSBYRECYID ) ;

      CHAR newCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 32 + 1 ] = { 0 } ;
      BSONObj boSpace ;
      BOOLEAN isExist = FALSE ;

      ossSnprintf( newCSName, DMS_COLLECTION_SPACE_NAME_SZ + 32,
                   "%s_%llu", csName, recycleID ) ;
      PD_LOG_MSG_CHECK( ossStrlen( newCSName ) <= DMS_COLLECTION_SPACE_NAME_SZ,
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to rename collection space [%s] to [%s], "
                        "new name is too long", csName, newCSName ) ;

      rc = catCheckSpaceExist( newCSName, isExist, boSpace, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check existence of collection "
                   "space [%s], rc: %d", newCSName, rc ) ;
      PD_LOG_MSG_CHECK( !isExist, SDB_DMS_CS_EXIST, error, PDERROR,
                        "Collection space [%s] already exists",
                        newCSName ) ;

      // lock collection space
      PD_CHECK( lockMgr.tryLockCollectionSpace( newCSName, EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock rename collection space [%s]", newCSName ) ;

      rc = addRenameCS( csName, newCSName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add rename collection space "
                   "[%s] to [%s], rc: %d", csName, newCSName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDRENAMECSBYRECYID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDRENAMECLBYRECYID, "_catRecycleReturnInfo::addRenameCLByRecyID" )
   INT32 _catRecycleReturnInfo::addRenameCLByRecyID( const catCheckCLInfo &checkInfo,
                                                     utilRecycleID recycleID,
                                                     catCtxLockMgr &lockMgr,
                                                     pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDRENAMECLBYRECYID ) ;

      const CHAR *origName = checkInfo.getOriginName().c_str() ;
      const CHAR *rtrnName = checkInfo.isRenamed() ? checkInfo.getReturnName().c_str() : origName ;
      CHAR newCLName[ DMS_COLLECTION_FULL_NAME_SZ + 32 + 1 ] = { 0 } ;
      BOOLEAN isExist = FALSE ;
      BSONObj boCollection ;

      // add recycle ID as suffix
      ossSnprintf( newCLName, DMS_COLLECTION_FULL_NAME_SZ + 32,
                   "%s_%llu", rtrnName, recycleID ) ;
      PD_LOG_MSG_CHECK( ossStrlen( newCLName ) <= DMS_COLLECTION_FULL_NAME_SZ,
                        SDB_INVALIDARG, error, PDERROR,
                        "Failed to rename collection [%s] to [%s], "
                        "new name is too long", origName, newCLName ) ;

      // check existence if needed
      rc = catCheckCollectionExist( newCLName, isExist, boCollection, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check existence of collection "
                   "[%s], rc: %d", newCLName, rc ) ;
      PD_LOG_MSG_CHECK( !isExist, SDB_DMS_EXIST, error, PDERROR,
                        "Collection [%s] already exists", newCLName ) ;

      // if cl uniqueID is conflict, it will be locked later
      if ( !isConflictUIDCL( checkInfo.getUniqueID() ) )
      {
         PD_CHECK( lockMgr.tryLockCollection( newCLName, EXCLUSIVE ),
                   SDB_LOCK_FAILED, error, PDERROR,
                   "Failed to lock rename collection [%s]", newCLName ) ;
      }

      rc = addRenameCL( origName, newCLName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add rename collection [%s] "
                   "to [%s], rc: %d", origName, newCLName, rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDRENAMECLBYRECYID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDCHGUIDCL, "_catRecycleReturnInfo::addChangeUIDCL" )
   INT32 _catRecycleReturnInfo::addChangeUIDCL( const ossPoolString &clName,
                                                utilCLUniqueID clUniqueID,
                                                catCtxLockMgr &lockMgr,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDCHGUIDCL ) ;

      utilCLUniqueID newCLUniqueID = UTIL_UNIQUEID_NULL ;
      utilReturnNameInfo info( clName.c_str() ) ;

      rc = catGetReturnCLUID( clUniqueID, newCLUniqueID, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to change collection unique "
                   "ID [%llu], rc: %d", clUniqueID, rc ) ;

      rc = getReturnCLName( info ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get return name of "
                   "collection [%s], rc: %d", clName.c_str(), rc ) ;
      PD_CHECK( lockMgr.tryLockCollection( info.getReturnName(), EXCLUSIVE ),
                SDB_LOCK_FAILED, error, PDERROR,
                "Failed to lock return collection [%s]", info.getReturnName() ) ;

      rc = _BASE::addChangeUIDCL( clUniqueID, newCLUniqueID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add change unique ID collection "
                   "[%llu] to [%llu], rc: %d", clUniqueID, newCLUniqueID,
                   rc ) ;

      // check conflict sequences
      for ( _CAT_CL_SEQ_MAP::iterator iter = _conflictSeq.find( clUniqueID ) ;
            iter != _conflictSeq.end() && iter->first == clUniqueID ;
            ++ iter )
      {
         catCheckSeqInfo &seqInfo = iter->second ;

         string returnSeqName ;
         utilSequenceID returnSeqID = UTIL_SEQUENCEID_NULL ;

         try
         {
            returnSeqName =
                  catGetSeqName4AutoIncFld( newCLUniqueID,
                                            seqInfo.getFieldName().c_str() ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to get sequence name, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         rc = addRenameSeq( seqInfo.getName().c_str(),
                            returnSeqName.c_str() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add rename sequence, rc: %d",
                      rc ) ;

         rc = catUpdateGlobalID( cb, w, returnSeqID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return sequence ID, rc: %d",
                      rc ) ;

         rc = addChangeUIDSeq( seqInfo.getUniqueID(), returnSeqID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add change unique ID sequence, "
                      "rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDCHGUIDCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDMISSSUBCL, "_catRecycleReturnInfo::addMissingSubCL" )
   INT32 _catRecycleReturnInfo::addMissingSubCL( const CHAR *subCLName,
                                                 const CHAR *mainCLName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDMISSSUBCL ) ;

      try
      {
         _missingSubCL.insert( make_pair( subCLName, mainCLName ) ) ;
         PD_LOG( PDWARNING, "Found missing sub-collection [%s] from "
                 "main-collection [%s]", subCLName, mainCLName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add missing sub-collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDMISSSUBCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_PROCESSMISSSUBCL, "_catRecycleReturnInfo::processMissingSubCL" )
   INT32 _catRecycleReturnInfo::processMissingSubCL( pmdEDUCB *cb,
                                                     _SDB_DMSCB *dmsCB,
                                                     _dpsLogWrapper *dpsCB,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_PROCESSMISSSUBCL ) ;

      for ( _CAT_SUB_MAIN_MAP::iterator iter = _missingSubCL.begin() ;
            iter != _missingSubCL.end() ;
            ++ iter )
      {
         BSONObj lowBound, upBound ;
         const CHAR *subCLName = iter->first.c_str() ;
         const CHAR *mainCLName = iter->second.c_str() ;
         const CHAR *rtrnMainCLName = NULL ;
         const CHAR *rtrnSubCLName = NULL ;

         utilReturnNameInfo mainCLNameInfo( mainCLName ) ;
         utilReturnNameInfo subCLNameInfo( subCLName ) ;

         rc = getReturnCLName( mainCLNameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return name of "
                      "collection [%s], rc: %d", mainCLName, rc ) ;
         rtrnMainCLName = mainCLNameInfo.getReturnName() ;

         rc = getReturnCLName( subCLNameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return name of "
                      "collection [%s], rc: %d", subCLName, rc ) ;
         rtrnSubCLName = subCLNameInfo.getReturnName() ;

         rc = catUnlinkMainCLStep( rtrnMainCLName, rtrnSubCLName, FALSE,
                                   lowBound, upBound, cb, dmsCB, dpsCB, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to unlink sub-connection "
                      "[return %s, origin %s] from main-collection "
                      "[return %s, origin %s], rc: %d",
                      rtrnSubCLName, subCLName, rtrnMainCLName, mainCLName,
                      rc ) ;

         PD_LOG( PDDEBUG, "unlink missing sub-collection "
                 "[return %s, origin %s] from main-collection "
                 "[return %s, origin %s]",
                 rtrnSubCLName, subCLName, rtrnMainCLName, mainCLName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_PROCESSMISSSUBCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDMISSMAINCL, "_catRecycleReturnInfo::addMissingMainCL" )
   INT32 _catRecycleReturnInfo::addMissingMainCL( const CHAR *mainCLName,
                                                  const CHAR *subCLName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDMISSMAINCL ) ;

      try
      {
         _missingMainCL.insert( make_pair( mainCLName, subCLName ) ) ;
         PD_LOG( PDWARNING, "Found missing main-collection [%s] from "
                          "sub-collection [%s]", mainCLName, subCLName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add missing main-collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDMISSMAINCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_PROCESSMISSMAINCL, "_catRecycleReturnInfo::processMissingMainCL" )
   INT32 _catRecycleReturnInfo::processMissingMainCL( pmdEDUCB *cb,
                                                      _SDB_DMSCB *dmsCB,
                                                      _dpsLogWrapper *dpsCB,
                                                      INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_PROCESSMISSMAINCL ) ;

      for ( _CAT_MAIN_SUB_MAP::iterator iter = _missingMainCL.begin() ;
            iter != _missingMainCL.end() ;
            ++ iter )
      {
         const CHAR *mainCLName = iter->first.c_str() ;
         const CHAR *subCLName = iter->second.c_str() ;

         utilReturnNameInfo nameInfo( subCLName ) ;

         rc = getReturnCLName( nameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return name of "
                      "collection [%s], rc: %d", subCLName, rc ) ;

         rc = catUnlinkSubCLStep( nameInfo.getReturnName(), cb, dmsCB, dpsCB,
                                  w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to unlink main-collection [%s] "
                      "from sub-collection [return %s, origin %s], rc: %d",
                      mainCLName, nameInfo.getReturnName(), subCLName, rc ) ;

         PD_LOG( PDDEBUG, "unlink missing main-collection [%s] from "
                 "sub-collection [return %s, origin %s]", mainCLName,
                 nameInfo.getReturnName(), subCLName ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_PROCESSMISSMAINCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_CHKCONFLICTS, "_catRecycleReturnInfo::checkConflicts" )
   INT32 _catRecycleReturnInfo::checkConflicts( const utilRecycleItem &item,
                                                BOOLEAN isEnforced,
                                                BOOLEAN isReturnToName,
                                                catCtxLockMgr &lockMgr,
                                                pmdEDUCB *cb,
                                                INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_CHKCONFLICTS ) ;

      rc = _checkConflictCS( item, isEnforced, lockMgr, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict collection spaces, "
                   "rc: %d", rc ) ;

      rc = _checkConflictNameCL( item, isEnforced, isReturnToName, lockMgr,
                                 cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict name collections, "
                   "rc: %d", rc ) ;

      rc = _checkConflictUIDCL( item, isEnforced, isReturnToName, lockMgr,
                                cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check conflict unique ID "
                   "collections, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_CHKCONFLICTS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDCONFLICTSEQ, "_catRecycleReturnInfo::addConflictSeq" )
   INT32 _catRecycleReturnInfo::addConflictSeq( utilCLUniqueID clUniqueID,
                                                utilSequenceID sequenceID,
                                                const CHAR *seqName,
                                                const CHAR *fieldName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDCONFLICTSEQ ) ;

      try
      {
         catCheckSeqInfo seqInfo( sequenceID, seqName, fieldName ) ;
         _conflictSeq.insert( make_pair( clUniqueID, seqInfo ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add conflict sequence, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDCONFLICTSEQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDRENAMESEQ, "_catRecycleReturnInfo::addRenameSeq" )
   INT32 _catRecycleReturnInfo::addRenameSeq( const CHAR *origSeqName,
                                              const CHAR *renameSeqName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDRENAMESEQ ) ;

      try
      {
         _renameSeq.insert( make_pair( origSeqName, renameSeqName ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add rename sequence, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDRENAMESEQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_GETRENAMESEQ, "_catRecycleReturnInfo::getRenameSeq" )
   BOOLEAN _catRecycleReturnInfo::getRenameSeq( const CHAR *origSeqName,
                                                const CHAR *&renameSeqName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_GETRENAMESEQ ) ;

      result = _getRename( _renameSeq, origSeqName, renameSeqName ) ;

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_GETRENAMESEQ ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDCHGUIDSEQ, "_catRecycleReturnInfo::addChangeUIDSeq" )
   INT32 _catRecycleReturnInfo::addChangeUIDSeq( utilSequenceID origSeqUID,
                                                 utilSequenceID rtrnSeqUID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDCHGUIDSEQ ) ;

      try
      {
         _changeUIDSeq.insert( make_pair( origSeqUID, rtrnSeqUID ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add change unique ID sequence, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDCHGUIDSEQ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_GETCHGUIDSEQ, "_catRecycleReturnInfo::getChangeUIDSeq" )
   BOOLEAN _catRecycleReturnInfo::getChangeUIDSeq( utilSequenceID origSeqUID,
                                                   utilSequenceID &rtrnSeqUID ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_GETCHGUIDSEQ ) ;

      _CAT_RETURN_SEQ_MAP::const_iterator iter =
                                             _changeUIDSeq.find( origSeqUID ) ;
      if ( iter != _changeUIDSeq.end() )
      {
         rtrnSeqUID = iter->second ;
         result = TRUE ;
      }

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_GETCHGUIDSEQ ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_RESETEXISTDSSET, "_catRecycleReturnInfo::resetExistDSSet" )
   void _catRecycleReturnInfo::resetExistDSSet()
   {
      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_RESETEXISTDSSET ) ;

      _existDSSet.clear() ;

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_RESETEXISTDSSET ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_CHKDSEXIST, "_catRecycleReturnInfo::checkDSExist" )
   INT32 _catRecycleReturnInfo::checkDSExist( UTIL_DS_UID dsUID,
                                              BOOLEAN &isExist,
                                              pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_CHKDSEXIST ) ;

      BSONObj boDataSource ;

      isExist = FALSE ;

      if ( _existDSSet.find( dsUID ) != _existDSSet.end() )
      {
         // in exist set
         isExist = TRUE ;
         goto done ;
      }
      else if ( _missingDSSet.find( dsUID ) != _missingDSSet.end() )
      {
         // in missing set
         goto done ;
      }

      rc = catCheckDataSourceExist( dsUID, isExist, boDataSource, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to check data source [%llu], rc: %d",
                   dsUID, rc ) ;

      try
      {
         if ( isExist )
         {
            _existDSSet.insert( dsUID ) ;
         }
         else
         {
            _missingDSSet.insert( dsUID ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save data source unique ID, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_CHKDSEXIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO__CHKCONFLICTCS, "_catRecycleReturnInfo::_checkConflictCS" )
   INT32 _catRecycleReturnInfo::_checkConflictCS( const utilRecycleItem &item,
                                                  BOOLEAN isEnforced,
                                                  catCtxLockMgr &lockMgr,
                                                  pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO__CHKCONFLICTCS ) ;

      if ( _conflictCSSet.empty() )
      {
         goto done ;
      }

      PD_LOG( PDWARNING, "Found [%llu] conflict collection spaces",
              _conflictCSSet.size() ) ;

      // we don't have truncate on collection space, so no need to consider
      // the rename case for conflict collection space

      if ( isEnforced )
      {
         for ( UTIL_RETURN_NAME_SET_IT iter = _conflictCSSet.begin() ;
               iter != _conflictCSSet.end() ;
               ++ iter )
         {
            rc = addReplaceCS( iter->c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add replace collection "
                         "space, rc: %d", rc ) ;
         }
      }
      else
      {
         ossPoolString csListStr ;

         try
         {
            StringBuilder ss ;
            UINT32 csCount = 0 ;
            for ( UTIL_RETURN_NAME_SET_IT iter = _conflictCSSet.begin() ;
                  iter != _conflictCSSet.end() ;
                  ++ iter )
            {
               if ( csCount > 0 )
               {
                  ss << ", " ;
               }
               if ( csCount > 3 )
               {
                  ss << "..." ;
                  break ;
               }
               ss << iter->c_str() ;
               ++ csCount ;
            }
            csListStr = ss.poolStr() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to generate collection space list "
                    "string, occur exception %s", e.what() ) ;
            csListStr.clear() ;
         }

         PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                           "Failed to return recycle item "
                           "[origin %s, recycle %s], "
                           "found [%llu] conflict collection space%s%s%s",
                           item.getOriginName(),
                           item.getRecycleName(),
                           _conflictCSSet.size(),
                           _conflictCSSet.size() <= 1 ? "" : "s",
                           csListStr.empty() ? "" : ": ",
                           csListStr.empty() ? "" : csListStr.c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO__CHKCONFLICTCS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO__CHKCONFLICTNAMECL, "_catRecycleReturnInfo::_checkConflictNameCL" )
   INT32 _catRecycleReturnInfo::_checkConflictNameCL( const utilRecycleItem &item,
                                                      BOOLEAN isEnforced,
                                                      BOOLEAN isReturnToName,
                                                      catCtxLockMgr &lockMgr,
                                                      pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO__CHKCONFLICTNAMECL ) ;

      _CAT_NAME_CL_MAP::iterator iter ;

      if ( _conflictNameCL.empty() )
      {
         goto done ;
      }

      PD_LOG( PDWARNING, "Found [%llu] name conflict collections",
              _conflictNameCL.size() ) ;

      iter = _conflictNameCL.begin() ;
      while ( iter != _conflictNameCL.end() )
      {
         catCheckCLInfo &checkInfo = iter->second ;
         BOOLEAN processed = FALSE ;

         if ( isReturnToName && !checkInfo.isCLRenamed() )
         {
            // if the item is not full renamed, we can rename with recycle ID
            rc = addRenameCLByRecyID( checkInfo, item.getRecycleID(), lockMgr,
                                      cb ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add rename collection, "
                         "rc: %d", rc ) ;

            processed = TRUE ;
         }
         else if ( isEnforced )
         {
            // replace for enforced mode
            rc = addReplaceCL( checkInfo.getReturnName().c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add replace collection, "
                         "rc: %d", rc ) ;

            processed = TRUE ;
         }

         if ( processed )
         {
            _conflictNameCL.erase( iter ++ ) ;
         }
         else
         {
            ++ iter ;
         }
      }

      // still have conflict collections
      if ( !_conflictNameCL.empty() )
      {
         ossPoolString clListStr ;

         try
         {
            StringBuilder ss ;
            UINT32 clCount = 0 ;
            for ( _CAT_NAME_CL_MAP::iterator iter = _conflictNameCL.begin() ;
                  iter != _conflictNameCL.end() ;
                  ++ iter )
            {
               if ( clCount > 0 )
               {
                  ss << ", " ;
               }
               if ( clCount > 3 )
               {
                  ss << "..." ;
                  break ;
               }
               ss << iter->second.getReturnName().c_str() ;
               ++ clCount ;
            }
            clListStr = ss.poolStr() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to generate collection list string, "
                    "occur exception %s", e.what() ) ;
            clListStr.clear() ;
         }

         PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                           "Failed to return recycle item "
                           "[origin %s, recycle %s], "
                           "found [%llu] name conflict collection%s%s%s",
                           item.getOriginName(),
                           item.getRecycleName(),
                           _conflictNameCL.size(),
                           _conflictNameCL.size() <= 1 ? "" : "s",
                           clListStr.empty() ? "" : ": ",
                           clListStr.empty() ? "" : clListStr.c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO__CHKCONFLICTNAMECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO__CHKCONFLICTUIDCL, "_catRecycleReturnInfo::_checkConflictUIDCL" )
   INT32 _catRecycleReturnInfo::_checkConflictUIDCL( const utilRecycleItem &item,
                                                     BOOLEAN isEnforced,
                                                     BOOLEAN isReturnToName,
                                                     catCtxLockMgr &lockMgr,
                                                     pmdEDUCB *cb,
                                                     INT16 w )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO__CHKCONFLICTUIDCL ) ;

      if ( _conflictUIDCL.empty() )
      {
         goto done ;
      }
      PD_LOG( PDWARNING, "Found [%llu] unique ID conflict collections",
              _conflictUIDCL.size() ) ;

      if ( isEnforced )
      {
         for ( _CAT_UID_CL_MAP::iterator iter = _conflictUIDCL.begin() ;
               iter != _conflictUIDCL.end() ;
               ++ iter )
         {
            rc = addReplaceCL( iter->second.getReturnName().c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add replace collection, "
                         "rc: %d", rc ) ;
         }
      }
      else if ( isReturnToName )
      {
         for ( _CAT_UID_CL_MAP::iterator iter = _conflictUIDCL.begin() ;
               iter != _conflictUIDCL.end() ;
               ++ iter )
         {
            rc = addChangeUIDCL( iter->second.getOriginName(),
                                 iter->second.getUniqueID(),
                                 lockMgr, cb, w ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add change unique ID "
                         "collection, rc: %d", rc ) ;
         }
      }
      else
      {
         ossPoolString clListStr ;

         try
         {
            StringBuilder ss ;
            UINT32 clCount = 0 ;
            for ( _CAT_UID_CL_MAP::iterator iter = _conflictUIDCL.begin() ;
                  iter != _conflictUIDCL.end() ;
                  ++ iter )
            {
               if ( clCount > 0 )
               {
                  ss << ", " ;
               }
               if ( clCount > 3 )
               {
                  ss << "..." ;
                  break ;
               }
               ss << iter->second.getReturnName().c_str() <<
                     "[" << iter->second.getUniqueID() << "]" ;
               ++ clCount ;
            }
            clListStr = ss.poolStr() ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDWARNING, "Failed to generate collection list string, "
                    "occur exception %s", e.what() ) ;
            clListStr.clear() ;
         }

         PD_LOG_MSG_CHECK( FALSE, SDB_RECYCLE_CONFLICT, error, PDERROR,
                           "Failed to return recycle item "
                           "[origin %s, recycle %s], "
                           "found [%llu] unique ID conflict collection%s%s%s",
                           item.getOriginName(),
                           item.getRecycleName(),
                           _conflictUIDCL.size(),
                           _conflictUIDCL.size() <= 1 ? "" : "s",
                           clListStr.empty() ? "" : ": ",
                           clListStr.empty() ? "" : clListStr.c_str() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO__CHKCONFLICTUIDCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO__ISMISSING_MAIN, "_catRecycleReturnInfo::_isMissing" )
   BOOLEAN _catRecycleReturnInfo::_isMissing( const _CAT_MAIN_SUB_MAP &missingMap,
                                              const CHAR *mainCLName ) const
   {
      BOOLEAN isMissing = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO__ISMISSING_MAIN ) ;

      try
      {
         if ( missingMap.end() != missingMap.find( mainCLName ) )
         {
            isMissing = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to check missing collection, "
                 "occur exception %s", e.what() ) ;

         for ( _CAT_MAIN_SUB_MAP::const_iterator iter = missingMap.begin() ;
               iter != missingMap.end() ;
               ++ iter )
         {
            INT32 keyRes = ossStrcmp( iter->first.c_str(), mainCLName ) ;
            if ( 0 == keyRes )
            {
               isMissing = TRUE ;
               break ;
            }
            else if ( keyRes > 0 )
            {
               // end search
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO__ISMISSING_MAIN ) ;

      return isMissing ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO__ISMISSING_SUB, "_catRecycleReturnInfo::_isMissing" )
   BOOLEAN _catRecycleReturnInfo::_isMissing( const _CAT_SUB_MAIN_MAP &missingMap,
                                              const CHAR *subCLName ) const
   {
      BOOLEAN isMissing = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO__ISMISSING_SUB ) ;

      try
      {
         _CAT_SUB_MAIN_MAP::const_iterator iter = missingMap.find( subCLName ) ;
         if ( iter != missingMap.end() )
         {
            isMissing = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to check missing collection, "
                 "occur exception %s", e.what() ) ;

         for ( _CAT_SUB_MAIN_MAP::const_iterator iter = missingMap.begin() ;
               iter != missingMap.end() ;
               ++ iter )
         {
            INT32 keyRes = ossStrcmp( iter->first.c_str(), subCLName ) ;
            if ( 0 == keyRes )
            {
               isMissing = TRUE ;
               break ;
            }
            else if ( keyRes > 0 )
            {
               // end search
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO__ISMISSING_SUB ) ;

      return isMissing ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDRBLDIDX, "_catRecycleReturnInfo::addRebuildIndex" )
   INT32 _catRecycleReturnInfo::addRebuildIndex( const CHAR *clName,
                                                 const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDRBLDIDX ) ;

      try
      {
         BSONObjBuilder builder ;
         BSONObjIterator iter( options ) ;
         while ( iter.more() )
         {
            BSONElement ele = iter.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            if ( 0 == ossStrcmp( IXM_FIELD_NAME_KEY, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_NAME, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_UNIQUE, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_ENFORCED, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_DROPDUPS, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_NOTNULL, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_NOTARRAY, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_GLOBAL, fieldName ) ||
                 0 == ossStrcmp( IXM_FIELD_NAME_STANDALONE, fieldName ) )
            {
               builder.append( ele ) ;
            }
         }
         BSONObj rebuildOptions = builder.obj() ;
         _rebuildIndexMap.insert( make_pair( clName, rebuildOptions ) ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add rebuild index, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDRBLDIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_PROCRBLDIDX, "_catRecycleReturnInfo::processRebuildIndex" )
   INT32 _catRecycleReturnInfo::processRebuildIndex( pmdEDUCB *cb,
                                                     _SDB_DMSCB *dmsCB,
                                                     _dpsLogWrapper *dpsCB,
                                                     INT16 w,
                                                     ossPoolSet< UINT64 > &taskSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_PROCRBLDIDX ) ;

      for ( _CAT_IDX_MAP::iterator iter = _rebuildIndexMap.begin() ;
            iter != _rebuildIndexMap.end() ;
            ++ iter )
      {
         catCMDCreateIndex indexCMD ;
         BSONObj createOptions ;
         rtnContextBuf ctxBuf ;
         INT64 contextID = -1 ;

         const CHAR *originName = iter->first.c_str() ;
         const CHAR *indexName = NULL ;
         utilReturnNameInfo clNameInfo( originName ) ;
         BSONObj indexDef = iter->second ;

         rc = getReturnCLName( clNameInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get return name of "
                      "collection [%s], rc: %d", originName, rc ) ;

         try
         {
            BSONElement ele = indexDef.getField( IXM_FIELD_NAME_NAME ) ;
            PD_CHECK( String == ele.type(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not string",
                      IXM_FIELD_NAME_NAME ) ;
            indexName = ele.valuestr() ;

            createOptions = BSON( FIELD_NAME_COLLECTION <<
                                  clNameInfo.getReturnName() <<
                                  FIELD_NAME_INDEX << indexDef ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to create index options, "
                    "occur exception %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }

         // make sure index is removed
         rc = catRemoveIndex( clNameInfo.getReturnName(), indexName, cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to remove index [%s] from "
                      "collection [%s], rc: %d", indexName,
                      clNameInfo.getReturnName(), rc ) ;

         // already locked
         indexCMD.disableLevelLock() ;

         rc = indexCMD.init( createOptions.objdata() ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to initialize create index "
                      "command [%s], rc: %d",
                      createOptions.toPoolString().c_str(), rc ) ;

         rc = indexCMD.doit( cb, ctxBuf, contextID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to execute create index "
                      "command [%s], rc: %d",
                      createOptions.toPoolString().c_str(), rc ) ;

         try
         {
            BSONObj taskDef( ctxBuf.data() ) ;
            BSONElement ele = taskDef.getField( FIELD_NAME_TASK_ID ) ;
            PD_CHECK( ele.isNumber(), SDB_SYS, error, PDERROR,
                      "Failed to get field [%s], it is not number",
                      FIELD_NAME_TASK_ID ) ;
            taskSet.insert( (UINT64)( ele.numberLong() ) ) ;
         }
         catch ( exception &e )
         {
            PD_LOG( PDERROR, "Failed to add task, occur exception %s",
                    e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_PROCRBLDIDX, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_CHKDOMAIN, "_catRecycleReturnInfo::checkCSDomain" )
   INT32 _catRecycleReturnInfo::checkCSDomain( utilCSUniqueID csUniqueID,
                                               const BSONObj &boSpace,
                                               pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_CHKDOMAIN ) ;

      try
      {
         const CHAR *domainName = NULL ;
         CAT_GROUP_LIST groupIDs ;

         // check if has domain name
         BSONElement ele = boSpace.getField( CAT_DOMAIN_NAME ) ;
         if ( EOO != ele.type() )
         {
            PD_CHECK( String == ele.type(), SDB_CAT_CORRUPTION, error, PDERROR,
                      "Failed to get field [%s], it is not a string",
                      CAT_DOMAIN_NAME ) ;
            domainName = ele.valuestr() ;
         }

         if ( NULL != domainName )
         {
            BOOLEAN isDomainExists = FALSE ;
            BSONObj boDomain ;

            rc = catCheckDomainExist( domainName, isDomainExists, boDomain,
                                      cb ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to get info of domain [%s], "
                         "rc: %d", domainName, rc ) ;

            if ( isDomainExists )
            {
               rc = catGetDomainGroups( boDomain, groupIDs ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to get groups of domain [%s], "
                            "rc: %d", domainName, rc ) ;
            }
            else
            {
               // domain had been dropped, mark it missing,
               // we can ignore it later in the return phase
               PD_LOG( PDWARNING, "Domain [%s] is missing", domainName ) ;

               rc = addMissingDomain( domainName ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to add missing domain [%s], "
                            "rc: %d", domainName, rc ) ;

               // use the system domain
               domainName = "" ;
            }
         }
         else
         {
            // use the system domain
            domainName = "" ;
         }

         rc = setDomainGroups( csUniqueID, domainName, groupIDs ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add domain groups for "
                      "collection space [%u], rc: %d", csUniqueID, rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to check check domain, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_CHKDOMAIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ADDMISSINGDOMAIN, "_catRecycleReturnInfo::addMissingDomain" )
   INT32 _catRecycleReturnInfo::addMissingDomain( const CHAR *domainName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ADDMISSINGDOMAIN ) ;

      try
      {
         _missingDomains.insert( domainName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add missing domain, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_ADDMISSINGDOMAIN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ISMISSINGDOMAIN, "_catRecycleReturnInfo::isMissingDomain" )
   BOOLEAN _catRecycleReturnInfo::isMissingDomain( const CHAR *domainName )
   {
      BOOLEAN isFound = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ISMISSINGDOMAIN ) ;

      SDB_ASSERT( NULL != domainName, "domain name is invalid" ) ;

      try
      {
         isFound = _missingDomains.count( domainName ) > 0 ? TRUE : FALSE ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to find missing domain, "
                 "occur exception %s", e.what() ) ;
         for ( UTIL_RETURN_NAME_SET_IT iter = _missingDomains.begin() ;
               iter != _missingDomains.end() ;
               ++ iter )
         {
            if ( 0 == ossStrcmp( iter->c_str(), domainName ) )
            {
               isFound = TRUE ;
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_ISMISSINGDOMAIN ) ;

      return isFound ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_SETDOMAINGRPS_LIST, "_catRecycleReturnInfo::setDomainGroups" )
   INT32 _catRecycleReturnInfo::setDomainGroups( utilCSUniqueID csUniqueID,
                                                 const CHAR *domainName,
                                                 const CAT_GROUP_LIST &groups )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_SETDOMAINGRPS_LIST ) ;

      try
      {
         _CAT_CS_GROUP_SET::iterator iter = _csDomainGroups.find( csUniqueID ) ;
         if ( iter != _csDomainGroups.end() )
         {
            _CAT_DOMAIN_GROUPS &domainGroups = iter->second ;
            PD_LOG( PDWARNING, "Already found domain [%s] for collection "
                    "space [%u]", domainGroups.first.c_str(), csUniqueID ) ;
         }
         else
         {
            _CAT_DOMAIN_GROUPS &domainGroups = _csDomainGroups[ csUniqueID ] ;
            domainGroups.first.assign( domainName ) ;
            domainGroups.second.insert( groups.begin(), groups.end() ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to set domain groups, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_SETDOMAINGRPS_LIST, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_CHKDOMAINGRP, "_catRecycleReturnInfo::checkDomainGroup" )
   INT32 _catRecycleReturnInfo::checkDomainGroup( utilCSUniqueID csUniqueID,
                                                  UINT32 groupID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_CHKDOMAINGRP ) ;

      sdbCatalogueCB *catCB = sdbGetCatalogueCB() ;
      const CHAR *groupName = NULL ;
      _CAT_CS_GROUP_SET::iterator iter ;

      // check if group exists and is activated
      BOOLEAN isActived = FALSE, isExist = FALSE ;
      isActived = catCB->checkGroupActived( groupID, isExist ) ;
      PD_LOG_MSG_CHECK( isExist, SDB_CLS_GRP_NOT_EXIST, error, PDERROR,
                        "Failed to check group [%u], "
                        "it does not exist", groupID ) ;
      groupName = catCB->groupID2Name( groupID ) ;
      PD_LOG_MSG_CHECK( isActived,
                        SDB_REPL_GROUP_NOT_ACTIVE, error, PDERROR,
                        "Failed to check group [ID: %u, name: %s], "
                        "it is not activated", groupID, groupName ) ;

      // check if group in domain of collection space
      iter = _csDomainGroups.find( csUniqueID ) ;
      if ( iter != _csDomainGroups.end() )
      {
         _CAT_DOMAIN_GROUPS &domainGroups = iter->second ;
         // NOTE: "" means system domain, for system domain, only check if
         // group exists and is activated, we already done earlier
         if ( !domainGroups.first.empty() )
         {
            PD_LOG_MSG_CHECK( domainGroups.second.count( groupID ),
                              SDB_CAT_GROUP_NOT_IN_DOMAIN, error, PDERROR,
                              "Failed to check group [ID: %u, name: %s], "
                              "it is not in domain [%s]", groupID, groupName,
                              domainGroups.first.c_str() ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_CHKDOMAINGRP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_ISCSDOMAINCHKED, "_catRecycleReturnInfo::isCSDomainChecked" )
   BOOLEAN _catRecycleReturnInfo::isCSDomainChecked( utilCSUniqueID csUniqueID )
   {
      BOOLEAN isChecked = FALSE ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_ISCSDOMAINCHKED ) ;

      isChecked = _csDomainGroups.count( csUniqueID ) > 0 ? TRUE : FALSE ;

      PD_TRACE_EXIT( SDB_CATRECYRTRNINFO_ISCSDOMAINCHKED ) ;

      return isChecked ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATRECYRTRNINFO_LOCKDOMAINS, "_catRecycleReturnInfo::lockDomains" )
   INT32 _catRecycleReturnInfo::lockDomains( catCtxLockMgr &lockMgr )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATRECYRTRNINFO_LOCKDOMAINS ) ;

      try
      {
         UTIL_RETURN_NAME_SET lockedDomains ;

         for ( _CAT_CS_GROUP_SET::iterator iter = _csDomainGroups.begin() ;
               iter != _csDomainGroups.end() ;
               ++ iter )
         {
            const ossPoolString &domainName = iter->second.first ;
            if ( ( !domainName.empty() ) &&
                 ( !lockedDomains.count( domainName ) ) )
            {
               PD_CHECK( lockMgr.tryLockDomain( domainName.c_str(), SHARED ),
                         SDB_LOCK_FAILED, error, PDERROR,
                         "Failed to lock domain [%s]", domainName.c_str() ) ;
               lockedDomains.insert( domainName ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to lock domains, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATRECYRTRNINFO_LOCKDOMAINS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
