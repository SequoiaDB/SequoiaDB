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

   Source File Name = rtnReorg.cpp

   Descriptive Name = Runtime Reorg

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for collection
   reorgnization.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtn.hpp"
#include "dmsStorageUnit.hpp"
#include "rtnRecover.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREORGRECOVER, "rtnReorgRecover" )
   INT32 rtnReorgRecover ( const CHAR *pCollectionName,
                           pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNREORGRECOVER ) ;

      SDB_ASSERT ( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( rtnCB, "rtnCB can't be NULL" ) ;

      dmsStorageUnitID suID = DMS_INVALID_CS ;
      dmsStorageUnit *su = NULL ;
      const CHAR *pCollectionShortName = NULL ;

      if ( ossStrlen ( pCollectionName ) > DMS_COLLECTION_FULL_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Collection name is too long: %s",
                  pCollectionName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                  pCollectionName ) ;
         goto error ;
      }

      {
         rtnCLRebuilderFactory *factory = rtnGetCLRebuilderFactory() ;
         rtnCLRebuilderBase *rebuilder = NULL ;
         rc = factory->create( su, pCollectionShortName, rebuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Create collection rebuilder failed, rc: %d",
                      rc ) ;
         rc = rebuilder->recover( cb ) ;
         factory->release( rebuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Recover collection[%s] failed, rc: %d",
                      pCollectionName, rc ) ;
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNREORGRECOVER, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNREORGOFFLINE, "rtnReorgOffline" )
   INT32 rtnReorgOffline ( const CHAR *pCollectionName,
                           const BSONObj &hint,
                           pmdEDUCB *cb,
                           SDB_DMSCB *dmsCB,
                           SDB_RTNCB *rtnCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNREORGOFFLINE ) ;

      dmsStorageUnitID suID = DMS_INVALID_CS ;
      dmsStorageUnit *su = NULL ;
      const CHAR *pCollectionShortName = NULL ;

      SDB_ASSERT ( pCollectionName, "collection name can't be NULL" ) ;
      SDB_ASSERT ( cb, "educb can't be NULL" ) ;
      SDB_ASSERT ( dmsCB, "dmsCB can't be NULL" ) ;

      if ( ossStrlen ( pCollectionName ) > DMS_COLLECTION_FULL_NAME_SZ )
      {
         PD_LOG ( PDERROR, "Collection name is too long: %s",
                  pCollectionName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = rtnResolveCollectionNameAndLock ( pCollectionName, dmsCB, &su,
                                             &pCollectionShortName, suID ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to resolve collection name %s",
                  pCollectionName ) ;
         goto error ;
      }

      {
         rtnCLRebuilderFactory *factory = rtnGetCLRebuilderFactory() ;
         rtnCLRebuilderBase *rebuilder = NULL ;
         rc = factory->create( su, pCollectionShortName, rebuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Create collection rebuilder failed, rc: %d",
                      rc ) ;
         rc = rebuilder->reorg( cb, hint ) ;
         factory->release( rebuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Reorganize collection[%s] failed, rc: %d",
                      pCollectionName, rc ) ;
      }

   done :
      if ( DMS_INVALID_CS != suID )
      {
         dmsCB->suUnlock ( suID ) ;
      }
      PD_TRACE_EXITRC ( SDB_RTNREORGOFFLINE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

}

