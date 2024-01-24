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

   Source File Name = catLocation.cpp

   Descriptive Name = N/A

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/20/2023  LCX Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_LOCATION_CPP__
#define CAT_LOCATION_CPP__

#include "catLocation.hpp"

namespace engine
{

   INT32 catGetLocationInfo( const BSONObj &groupObj,
                             CLS_LOC_INFO_MAP *pLocationInfo )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement ele ;
         UINT32 groupID = INVALID_GROUPID ;

         // Group ID
         ele = groupObj.getField( CAT_GROUPID_NAME ) ;
         PD_CHECK( NumberInt == ele.type(), SDB_INVALIDARG, error, PDWARNING,
                   "Failed to parse [%s]", CAT_GROUPID_NAME ) ;
         groupID = ele.numberInt() ;

         // Get Locations array
         ele = groupObj.getField( CAT_LOCATIONS_NAME ) ;
         if ( ! ele.eoo() && Array != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDWARNING, "Failed to parse [%s], type[%d] is not array",
                    CAT_LOCATIONS_NAME, ele.type() ) ;
            goto error ;
         }

         // Parse Locations array
         if ( NULL != pLocationInfo && Array == ele.type() )
         {
            BSONObjIterator i( ele.embeddedObject() ) ;
            while ( i.more() )
            {
               BSONElement beLocations = i.next() ;
               PD_CHECK( Object == beLocations.type(), SDB_INVALIDARG, error,
                         PDWARNING, "Failed to parse [%s]", CAT_LOCATIONS_NAME ) ;

               _clsLocationInfoItem item ;
               BSONObj boLocation = beLocations.embeddedObject() ;

               BSONElement beLocation = boLocation.getField( CAT_LOCATION_NAME ) ;
               BSONElement beLocationID = boLocation.getField( CAT_LOCATIONID_NAME ) ;
               BSONElement bePrimary = boLocation.getField( CAT_PRIMARY_NAME ) ;

               if ( beLocation.eoo() || String != beLocation.type() )
               {
                  PD_LOG( PDWARNING, "Failed to parse [%s]", CAT_LOCATION_NAME ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               item._location = beLocation.valuestrsafe() ;

               if ( beLocationID.eoo() || NumberInt != beLocationID.type() )
               {
                  PD_LOG( PDWARNING, "Failed to parse [%s]", CAT_LOCATIONID_NAME ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               item._locationID = beLocationID.numberInt() ;

               if ( ! bePrimary.eoo() && NumberInt == bePrimary.type() )
               {
                  item._primary.columns.groupID = groupID ;
                  item._primary.columns.nodeID = bePrimary.numberInt() ;
               }

               pLocationInfo->insert( CLS_LOC_INFO_MAP::value_type( item._locationID, item ) ) ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catGetLocationID( const BSONObj &groupObj,
                           const CHAR *pLocation,
                           UINT32 &locationID )
   {
      INT32 rc = SDB_OK ;

      CLS_LOC_INFO_MAP locMap ;
      CLS_LOC_INFO_MAP::const_iterator itr ;

      rc = catGetLocationInfo( groupObj, &locMap ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get location info map" ) ;

      itr = locMap.begin() ;
      while ( locMap.end() != itr )
      {
         if ( 0 == ossStrcmp( pLocation, itr->second._location.c_str() ) )
         {
            locationID = itr->second._locationID ;
            break ;
         }
         ++itr ;
      }

      if ( locMap.end() == itr )
      {
         locationID = CAT_INVALID_LOCATIONID ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 catCheckAndGetActiveLocation( const BSONObj &groupObj,
                                       const UINT32 groupID,
                                       const ossPoolString &newActLoc,
                                       ossPoolString &oldActLoc )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONElement optionEle ;
         UINT32 locationID = CAT_INVALID_LOCATIONID ;

         // Check the groupID, only the node in cata and data group can set active location
         if ( CATALOG_GROUPID != groupID &&
              ( DATA_GROUP_ID_BEGIN > groupID || DATA_GROUP_ID_END < groupID ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Group[%u] doesn't support to set ActiveLocation", groupID ) ;
            goto error ;
         }

         // If the new active location is not "", need to get and check if the new location exists
         if ( ! newActLoc.empty() )
         {
            rc = catGetLocationID( groupObj, newActLoc.c_str(), locationID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to check location[%s] info in group[%u]",
                         newActLoc.c_str(), groupID ) ;
            if ( CAT_INVALID_LOCATIONID == locationID )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Location:[%s] doesn't exist in group[%u]",
                           newActLoc.c_str(), groupID ) ;
               goto error ;
            }
         }

         // Get old ActiveLocation, this field can be empty
         optionEle = groupObj.getField( CAT_ACTIVE_LOCATION_NAME ) ;
         if ( ! optionEle.eoo() && String != optionEle.type() )
         {
            rc = SDB_CAT_CORRUPTION ;
            PD_LOG( PDWARNING, "Failed to parse [%s], type[%d] is not string",
                    CAT_ACTIVE_LOCATION_NAME, optionEle.type() ) ;
            goto error ;
         }
         oldActLoc = optionEle.valuestrsafe() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception happened: %s, rc: %d", e.what(), rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }


}

#endif // CAT_LOCATION_CPP__