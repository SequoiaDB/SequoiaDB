/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = dms.cpp

   Descriptive Name = Data Management Service

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data insert/update/delete. This file does NOT include index logic.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/19/2012  JWH  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dms.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "pmdEDU.hpp"
#include "ixm.hpp"
#include "rtn.hpp"
#include "pmdStartup.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"

namespace engine
{

   /*
      DMS TOOL FUNCTIONS:
   */
   BOOLEAN dmsAccessAndFlagCompatiblity ( UINT16 collectionFlag,
                                          DMS_ACCESS_TYPE accessType )
   {
      // if we are in crash recovery mode, only recovery thread is able to
      // perform query, in this case we always return TRUE
      if ( !pmdGetStartup().isOK() )
      {
         return TRUE ;
      }
      else if ( DMS_IS_MB_FREE(collectionFlag) ||
                DMS_IS_MB_DROPPED(collectionFlag) )
      {
         return FALSE ;
      }
      else if ( DMS_IS_MB_NORMAL(collectionFlag) )
      {
         return TRUE ;
      }
      else if ( DMS_IS_MB_OFFLINE_REORG(collectionFlag) )
      {
         if ( DMS_IS_MB_OFFLINE_REORG_TRUNCATE(collectionFlag) &&
            ( accessType == DMS_ACCESS_TYPE_TRUNCATE ) )
         {
            return TRUE ;
         }
         else if ( ( DMS_IS_MB_OFFLINE_REORG_SHADOW_COPY ( collectionFlag ) ||
                     DMS_IS_MB_OFFLINE_REORG_REBUILD( collectionFlag ) ) &&
                  ( ( accessType == DMS_ACCESS_TYPE_QUERY ) ||
                    ( accessType == DMS_ACCESS_TYPE_FETCH ) ) )
         {
            return TRUE ;
         }
         return FALSE ;
      }
      else if ( DMS_IS_MB_ONLINE_REORG(collectionFlag) )
      {
         return TRUE ;
      }
      else if ( DMS_IS_MB_LOAD ( collectionFlag ) &&
                DMS_ACCESS_TYPE_TRUNCATE != accessType )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN dmsIsSysCSName ( const CHAR *collectionSpaceName )
   {
      if ( collectionSpaceName && ossStrlen ( collectionSpaceName ) >= 3 &&
           'S' == collectionSpaceName[0] &&
           'Y' == collectionSpaceName[1] &&
           'S' == collectionSpaceName[2] )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCHKCSNM, "dmsCheckCSName" )
   INT32 dmsCheckCSName ( const CHAR *collectionSpaceName,
                          BOOLEAN sys )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DMSCHKCSNM );
      INT32 csNameLen = 0 ;
      CHAR  invalidArray[] = {
         '.',
         '\\',
         '/',
         ':',
         '*',
         '?',
         '"',
         '<',
         '>',
         '|' } ;
      csNameLen = ossStrlen ( collectionSpaceName ) ;
      if ( DMS_COLLECTION_SPACE_NAME_SZ < csNameLen || 0 >= csNameLen )
      {
         PD_LOG_MSG ( PDERROR,
                      "collection space name length is not valid: %s",
                      collectionSpaceName ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( '$' == collectionSpaceName[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "collection space name shouldn't be started with '$': %s",
                      collectionSpaceName ) ;
         goto error ;
      }

      if ( !sys && dmsIsSysCSName(collectionSpaceName) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "collection space name should not start with 'SYS': %s",
                      collectionSpaceName );
         goto error ;
      }

      for ( UINT32 i = 0; i < sizeof(invalidArray); ++i )
      {
         if ( ossStrchr ( collectionSpaceName, invalidArray[i] ) )
         {
            PD_LOG_MSG ( PDERROR,
                         "collection space name should not include '%c': %s",
                         invalidArray[i], collectionSpaceName ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB_DMSCHKCSNM, rc );
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN dmsIsSysCLName ( const CHAR *collectionName )
   {
      if ( collectionName && ossStrlen ( collectionName ) >= 3 &&
           'S' == collectionName[0] &&
           'Y' == collectionName[1] &&
           'S' == collectionName[2] )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCHKFULLCLNM, "dmsCheckFullCLName" )
   INT32 dmsCheckFullCLName ( const CHAR *fullCollectionName,
                              BOOLEAN sys )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DMSCHKFULLCLNM );
      CHAR buffer [ DMS_COLLECTION_NAME_SZ +
                    DMS_COLLECTION_SPACE_NAME_SZ + 2 ] = {0} ;
      ossStrncpy ( buffer, fullCollectionName,
                   DMS_COLLECTION_NAME_SZ +
                   DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
      CHAR *p = ossStrchr ( buffer, '.' ) ;
      if ( !p )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "full collection name should include '.': %s",
                  fullCollectionName ) ;
         goto error ;
      }
      *p = '\0' ;
      p++ ;
      rc = dmsCheckCSName ( buffer, sys ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = dmsCheckCLName ( p, sys ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_DMSCHKFULLCLNM, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCHKCLNM, "dmsCheckCLName" )
   INT32 dmsCheckCLName ( const CHAR *collectionName,
                          BOOLEAN sys )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DMSCHKCLNM );
      INT32 clNameLen = 0 ;
      clNameLen = ossStrlen ( collectionName ) ;
      if ( DMS_COLLECTION_NAME_SZ < clNameLen || 0 >= clNameLen )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "collection name length is not valid: %s",
                      collectionName ) ;
         goto error ;
      }
      if ( '$' == collectionName[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "collection name shouldn't be started with '$': %s",
                      collectionName ) ;
         goto error ;
      }
      if ( ossStrchr ( collectionName, '.' ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR,
                  "collection name should not include '.': %s",
                  collectionName ) ;
         goto error ;
      }
      if ( !sys && dmsIsSysCLName(collectionName) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "collection name is system: %s", collectionName ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_DMSCHKCLNM, rc );
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN dmsIsSysIndexName ( const CHAR *indexName )
   {
      if ( indexName && '$' == indexName[0] )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_DMSCHKINXNM, "dmsCheckIndexName" )
   INT32 dmsCheckIndexName ( const CHAR *indexName,
                             BOOLEAN sys )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_DMSCHKINXNM );
      INT32 indexLen = 0 ;
      indexLen = ossStrlen ( indexName ) ;
      if ( IXM_INDEX_NAME_SIZE < indexLen || 0 >= indexLen )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "index name length is not valid: %s",
                      indexName ) ;

         goto error ;
      }
      if ( !sys && dmsIsSysIndexName(indexName) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "index name shouldn't be started with '$': %s",
                      indexName ) ;
         goto error ;
      }
      if ( ossStrchr ( indexName, '.' ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG ( PDERROR,
                      "index name should not include '.': %s",
                      indexName ) ;
         goto error ;
      }
   done :
      PD_TRACE_EXITRC ( SDB_DMSCHKINXNM, rc );
      return rc ;
   error :
      goto done ;
   }

   std::string dmsGetCSNameFromFullName( const std::string &fullName )
   {
      return fullName.substr( 0, fullName.find( '.' ) ) ;
   }

   std::string dmsGetCLShortNameFromFullName( const std::string &fullName )
   {
      return fullName.substr( fullName.find( '.' ) + 1 ) ;
   }

}

