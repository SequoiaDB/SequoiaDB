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

   Source File Name = utilRecycleReturnInfo.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for recycle item.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilRecycleReturnInfo.hpp"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "../bson/bson.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _utilRecycleReturnInfo implement
    */
   _utilRecycleReturnInfo::_utilRecycleReturnInfo()
   {
   }

   _utilRecycleReturnInfo::~_utilRecycleReturnInfo()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_ADDREPLACECS, "_utilRecycleReturnInfo::addReplaceCS" )
   INT32 _utilRecycleReturnInfo::addReplaceCS( const CHAR *csName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_ADDREPLACECS ) ;

      try
      {
         _replaceCSSet.insert( csName ) ;
         PD_LOG( PDEVENT, "Found replace collection space [%s]", csName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add replace collection space, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_ADDREPLACECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_ADDREPLACECL, "_utilRecycleReturnInfo::addReplaceCL" )
   INT32 _utilRecycleReturnInfo::addReplaceCL( const CHAR *clName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_ADDREPLACECL ) ;

      try
      {
         _replaceCLSet.insert( clName ) ;
         PD_LOG( PDEVENT, "Found replace collection [%s]", clName ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add replace collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_ADDREPLACECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_ADDRENAMECS, "_utilRecycleReturnInfo::addRenameCS" )
   INT32 _utilRecycleReturnInfo::addRenameCS( const CHAR *origCSName,
                                              const CHAR *returnCSName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_ADDRENAMECS ) ;

      try
      {
         if ( 0 != ossStrcmp( origCSName, returnCSName ) )
         {
            _renameCSMap.insert( make_pair( origCSName, returnCSName ) ) ;
            PD_LOG( PDEVENT, "Return collection space [%s] to [%s]",
                    origCSName, returnCSName ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add rename collection space, occur "
                 "exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_ADDRENAMECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_ADDRENAMECL, "_utilRecycleReturnInfo::addRenameCL" )
   INT32 _utilRecycleReturnInfo::addRenameCL( const CHAR *origCLName,
                                              const CHAR *returnCLName )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_ADDRENAMECL ) ;

      try
      {
         if ( 0 != ossStrcmp( origCLName, returnCLName ) )
         {
            _renameCLMap.insert( make_pair( origCLName, returnCLName ) ) ;
            PD_LOG( PDEVENT, "Return collection [%s] to [%s]",
                    origCLName, returnCLName) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add rename collection, occur "
                 "exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_ADDRENAMECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_ADDCHGUIDCL, "_utilRecycleReturnInfo::addChangeUIDCL" )
   INT32 _utilRecycleReturnInfo::addChangeUIDCL( utilCLUniqueID origCLUniqueID,
                                                 utilCLUniqueID returnCLUniqueID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_ADDCHGUIDCL ) ;

      try
      {
         if ( origCLUniqueID != returnCLUniqueID )
         {
            _changeUIDCL.insert( make_pair( origCLUniqueID,
                                            returnCLUniqueID ) ) ;
            PD_LOG( PDEVENT, "Return collection [%llu] to [%llu]",
                    origCLUniqueID, returnCLUniqueID ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to add change unique ID collection, occur "
                 "exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_ADDCHGUIDCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__GETRENAMECS, "_utilRecycleReturnInfo::_getRenameCS" )
   BOOLEAN _utilRecycleReturnInfo::_getRenameCS( const CHAR *origCSName,
                                                 const CHAR *&returnCSName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__GETRENAMECS ) ;

      result = _getRename( _renameCSMap, origCSName, returnCSName ) ;

      PD_TRACE_EXIT( SDB_UTILRECYRTRNINFO__GETRENAMECS ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__GETRENAMECL, "_utilRecycleReturnInfo::_getRenameCL" )
   BOOLEAN _utilRecycleReturnInfo::_getRenameCL( const CHAR *origCLName,
                                                 const CHAR *&returnCLName ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__GETRENAMECL ) ;

      result = _getRename( _renameCLMap, origCLName, returnCLName ) ;

      PD_TRACE_EXIT( SDB_UTILRECYRTRNINFO__GETRENAMECL ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__GETRENAME, "_utilRecycleReturnInfo::_getRename" )
   BOOLEAN _utilRecycleReturnInfo::_getRename( const UTIL_RETURN_NAME_MAP &nameMap,
                                               const CHAR *origName,
                                               const CHAR *&returnName )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__GETRENAME ) ;

      try
      {
         UTIL_RETURN_NAME_MAP_CIT iter = nameMap.find( origName ) ;
         if ( iter != nameMap.end() )
         {
            returnName = iter->second.c_str() ;
            result = TRUE ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to get rename info, occur exception %s",
                 e.what() ) ;

         for ( UTIL_RETURN_NAME_MAP_CIT iter = nameMap.begin() ;
               iter != nameMap.end() ;
               ++ iter )
         {
            if ( 0 == ossStrcmp( origName, iter->first.c_str() ) )
            {
               returnName = iter->second.c_str() ;
               result = TRUE ;
               break ;
            }
         }
      }

      PD_TRACE_EXIT( SDB_UTILRECYRTRNINFO__GETRENAME ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__GETCHGUIDCL, "_utilRecycleReturnInfo::_getChangeUIDCL" )
   BOOLEAN _utilRecycleReturnInfo::_getChangeUIDCL( utilCLUniqueID origCLUID,
                                                    utilCLUniqueID &returnCLUID ) const
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__GETCHGUIDCL ) ;

      result = _getChangeUID( _changeUIDCL, origCLUID, returnCLUID ) ;

      PD_TRACE_EXIT( SDB_UTILRECYRTRNINFO__GETCHGUIDCL ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__GETCHGUID, "_utilRecycleReturnInfo::_getChangeUID" )
   BOOLEAN _utilRecycleReturnInfo::_getChangeUID( const UTIL_RETURN_UID_MAP &uidMap,
                                                  utilCLUniqueID origUID,
                                                  utilCLUniqueID &returnUID )
   {
      BOOLEAN result = FALSE ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__GETCHGUID ) ;

      UTIL_RETURN_UID_MAP_CIT iter = uidMap.find( origUID ) ;
      if ( iter != uidMap.end() )
      {
         returnUID = iter->second ;
         result = TRUE ;
      }

      PD_TRACE_EXIT( SDB_UTILRECYRTRNINFO__GETCHGUID ) ;

      return result ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_GETRTRNCSNAME, "_utilRecycleReturnInfo::getReturnCSName" )
   INT32 _utilRecycleReturnInfo::getReturnCSName( utilReturnNameInfo &info ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_GETRTRNCSNAME ) ;

      const CHAR *originCSName = info.getOriginName() ;
      const CHAR *returnCSName = NULL ;

      if ( hasRenameCS() &&
           _getRenameCS( originCSName, returnCSName ) )
      {
         if ( 0 != ossStrcmp( originCSName, returnCSName ) )
         {
            info.setReturnName( returnCSName, UTIL_RETURN_RENAMED_CS ) ;
         }
         else
         {
            info.setReturnName( returnCSName, UTIL_RETURN_NO_RENAME ) ;
         }
      }

      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_GETRTRNCSNAME, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_GETRTRNCLNAME, "_utilRecycleReturnInfo::getReturnCLName" )
   INT32 _utilRecycleReturnInfo::getReturnCLName( utilReturnNameInfo &info ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_GETRTRNCLNAME ) ;

      BOOLEAN isChecked = FALSE ;
      const CHAR *originName = info.getOriginName() ;

      if ( hasRenameCL() )
      {
         const CHAR *returnName = NULL ;
         if ( _getRenameCL( originName, returnName ) )
         {
            SDB_ASSERT( NULL != returnName, "return name is invalid" ) ;
            if ( 0 != ossStrcmp( returnName, originName ) )
            {
               info.setReturnName( returnName, UTIL_RETURN_RENAMED_CL ) ;
            }
            else
            {
               info.setReturnName( returnName, UTIL_RETURN_NO_RENAME ) ;
            }
            isChecked = TRUE ;
         }
      }

      if ( !isChecked && hasRenameCS() )
      {
         CHAR origCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] = { 0 } ;
         CHAR origCLShortName[ DMS_COLLECTION_NAME_SZ + 1 ] = { 0 } ;

         const CHAR *rtrnCSName = NULL ;

         rc = rtnResolveCollectionName( originName, ossStrlen( originName ),
                                        origCSName,
                                        DMS_COLLECTION_SPACE_NAME_SZ,
                                        origCLShortName,
                                        DMS_COLLECTION_NAME_SZ ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to resolve collection name [%s], "
                      "rc: %d", originName, rc ) ;

         if ( _getRenameCS( origCSName, rtrnCSName ) )
         {
            if ( 0 != ossStrcmp( origCSName, rtrnCSName ) )
            {
               info.setReturnName( rtrnCSName,
                                   origCLShortName,
                                   UTIL_RETURN_RENAMED_CS ) ;
            }
            else
            {
               info.setReturnName( rtrnCSName,
                                   origCLShortName,
                                   UTIL_RETURN_NO_RENAME ) ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_GETRTRNCLNAME, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_GETRTRNCLUID, "_utilRecycleReturnInfo::getReturnCLUID" )
   INT32 _utilRecycleReturnInfo::getReturnCLUID( utilReturnUIDInfo &info ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_GETRTRNCLUID ) ;

      utilCLUniqueID origCLUID = info.getOriginUID() ;
      utilCLUniqueID rtrnCLUID = UTIL_UNIQUEID_NULL ;

      if ( hasChangeUIDCL() &&
           _getChangeUIDCL( origCLUID, rtrnCLUID ) )
      {
         info.setReturnUID( rtrnCLUID ) ;
      }

      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_GETRTRNCLUID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_REBLDBSON, "_utilRecycleReturnInfo::rebuildBSON" )
   INT32 _utilRecycleReturnInfo::rebuildBSON( const BSONObj &object,
                                              BSONObjBuilder &builder,
                                              UINT8 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_REBLDBSON ) ;

      if ( UTIL_RETURN_MASK_EMPTY == mask )
      {
         goto done ;
      }

      try
      {
         BSONObjIterator iter( object ) ;
         while ( iter.more() )
         {
            BSONElement element = iter.next() ;
            // if mask is not set, will not build to BSON
            if ( ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_REPLACE_CS ) &&
                   0 == ossStrcmp( element.fieldName(),
                                   FIELD_NAME_REPLACE_CS ) ) ||
                 ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_REPLACE_CL ) &&
                   0 == ossStrcmp( element.fieldName(),
                                   FIELD_NAME_REPLACE_CL ) ) ||
                 ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_RENAME_CS ) &&
                   0 == ossStrcmp( element.fieldName(),
                                   FIELD_NAME_RENAME_CS ) ) ||
                 ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_RENAME_CL ) &&
                   0 == ossStrcmp( element.fieldName(),
                                   FIELD_NAME_RENAME_CL ) ) ||
                 ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_CHANGEUID_CL ) &&
                   0 == ossStrcmp( element.fieldName(),
                                   FIELD_NAME_CHANGEUID_CL ) ) )
            {
               builder.append( element ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to rebuild BSON for return info, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_REBLDBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_TOBSON, "_utilRecycleReturnInfo::toBSON" )
   INT32 _utilRecycleReturnInfo::toBSON( BSONObjBuilder &builder,
                                         UINT8 mask ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_TOBSON ) ;

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_REPLACE_CS ) )
      {
         rc = _buildReplaceCS( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for replace "
                      "collection spaces, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_REPLACE_CL ) )
      {
         rc = _buildReplaceCL( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for replace "
                      "collections, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_RENAME_CS ) )
      {
         rc = _buildRenameCS( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for rename "
                      "collection spaces, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_RENAME_CL ) )
      {
         rc = _buildRenameCL( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for rename "
                      "collections, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_CHANGEUID_CL ) )
      {
         rc = _buildChangeUIDCL( builder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for change unique ID "
                      "collections, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDREPLACECS, "_utilRecycleReturnInfo::_buildReplaceCS" )
   INT32 _utilRecycleReturnInfo::_buildReplaceCS( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDREPLACECS ) ;

      rc = _buildNameSet( FIELD_NAME_REPLACE_CS, _replaceCSSet, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for replace collection "
                   "spaces, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDREPLACECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDREPLACECL, "_utilRecycleReturnInfo::_buildReplaceCL" )
   INT32 _utilRecycleReturnInfo::_buildReplaceCL( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDREPLACECL ) ;

      rc = _buildNameSet( FIELD_NAME_REPLACE_CL, _replaceCLSet, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for replace "
                   "collections, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDREPLACECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDRENAMECS, "_utilRecycleReturnInfo::_buildRenameCS" )
   INT32 _utilRecycleReturnInfo::_buildRenameCS( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDRENAMECS ) ;

      rc = _buildNameMap( FIELD_NAME_RENAME_CS, _renameCSMap, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed build BSON for rename collection "
                   "spaces, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDRENAMECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDRENAMECL, "_utilRecycleReturnInfo::_buildRenameCL" )
   INT32 _utilRecycleReturnInfo::_buildRenameCL( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDRENAMECL ) ;

      rc = _buildNameMap( FIELD_NAME_RENAME_CL, _renameCLMap, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed build BSON for rename collections, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDRENAMECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDCHGUIDCL, "_utilRecycleReturnInfo::_buildChangeUIDCL" )
   INT32 _utilRecycleReturnInfo::_buildChangeUIDCL( BSONObjBuilder &builder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDCHGUIDCL ) ;

      rc = _buildUIDMap( FIELD_NAME_CHANGEUID_CL, _changeUIDCL, builder ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build BSON for change unique ID "
                 "collections, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDCHGUIDCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILRECYRTRNINFO__BLDNAMESET, "_utilRecycleReturnInfo::_buildNameSet" )
   INT32 _utilRecycleReturnInfo::_buildNameSet( const CHAR *fieldName,
                                                const UTIL_RETURN_NAME_SET &nameSet,
                                                BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__UTILRECYRTRNINFO__BLDNAMESET ) ;

      if ( nameSet.empty() )
      {
         goto done ;
      }

      try
      {
         BSONArrayBuilder setBuilder( builder.subarrayStart( fieldName ) ) ;
         for ( UTIL_RETURN_NAME_SET_CIT iter = nameSet.begin() ;
               iter != nameSet.end() ;
               ++ iter )
         {
            setBuilder.append( iter->c_str() ) ;
         }
         setBuilder.done() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed build BSON for name set [%s], "
                 "occur exception %s", fieldName, e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__UTILRECYRTRNINFO__BLDNAMESET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDNAMEMAP, "_utilRecycleReturnInfo::_buildNameMap" )
   INT32 _utilRecycleReturnInfo::_buildNameMap( const CHAR *fieldName,
                                                const UTIL_RETURN_NAME_MAP &nameMap,
                                                BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDNAMEMAP ) ;

      if ( nameMap.empty() )
      {
         goto done ;
      }

      try
      {
         BSONArrayBuilder mapBuilder( builder.subarrayStart( fieldName ) ) ;
         for ( UTIL_RETURN_NAME_MAP_CIT iter = nameMap.begin() ;
               iter != nameMap.end() ;
               ++ iter )
         {
            BSONArrayBuilder subBuilder( mapBuilder.subarrayStart() ) ;
            subBuilder.append( iter->first.c_str() ) ;
            subBuilder.append( iter->second.c_str() ) ;
            subBuilder.done() ;
         }
         mapBuilder.done() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed build BSON for name map, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDNAMEMAP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__BLDUIDMAP, "_utilRecycleReturnInfo::_buildUIDMap" )
   INT32 _utilRecycleReturnInfo::_buildUIDMap( const CHAR *fieldName,
                                               const UTIL_RETURN_UID_MAP &uidMap,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__BLDUIDMAP ) ;

      if ( uidMap.empty() )
      {
         goto done ;
      }

      try
      {
         BSONArrayBuilder mapBuilder( builder.subarrayStart( fieldName ) ) ;
         for ( UTIL_RETURN_UID_MAP_CIT iter = uidMap.begin() ;
               iter != uidMap.end() ;
               ++ iter )
         {
            BSONArrayBuilder subBuilder( mapBuilder.subarrayStart() ) ;
            subBuilder.append( (INT64)( iter->first ) ) ;
            subBuilder.append( (INT64)( iter->second ) ) ;
            subBuilder.done() ;
         }
         mapBuilder.done() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed build BSON for unique ID map, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__BLDUIDMAP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_FROMBSON, "_utilRecycleReturnInfo::fromBSON" )
   INT32 _utilRecycleReturnInfo::fromBSON( const BSONObj &object, UINT8 mask )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO_FROMBSON ) ;

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_REPLACE_CS ) )
      {
         rc = _parseReplaceCS( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse set of replace "
                      "collection spaces, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_REPLACE_CL ) )
      {
         rc = _parseReplaceCL( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse set of replace "
                      "collections, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_RENAME_CS ) )
      {
         rc = _parseRenameCS( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse map of rename "
                      "collection spaces, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_RENAME_CL ) )
      {
         rc = _parseRenameCL( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse map of rename "
                      "collections, rc: %d", rc ) ;
      }

      if ( OSS_BIT_TEST( mask, UTIL_RETURN_MASK_CHANGEUID_CL ) )
      {
         rc = _parseChangeUIDCL( object ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse map of change "
                      "unique ID collections, rc: %d", rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSEREPLACECS, "_utilRecycleReturnInfo::_parseReplaceCS" )
   INT32 _utilRecycleReturnInfo::_parseReplaceCS( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSEREPLACECS ) ;

      rc = _parseNameSet( FIELD_NAME_REPLACE_CS, object, _replaceCSSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse set of replace collection "
                   "spaces, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSEREPLACECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSEREPLACECL, "_utilRecycleReturnInfo::_parseReplaceCL" )
   INT32 _utilRecycleReturnInfo::_parseReplaceCL( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSEREPLACECL ) ;

      rc = _parseNameSet( FIELD_NAME_REPLACE_CL, object, _replaceCLSet ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse set of replace collections, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSEREPLACECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSERENAMECS, "_utilRecycleReturnInfo::_parseRenameCS" )
   INT32 _utilRecycleReturnInfo::_parseRenameCS( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSERENAMECS ) ;

      rc = _parseNameMap( FIELD_NAME_RENAME_CS, object, _renameCSMap ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse map of rename collection "
                   "spaces, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSERENAMECS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSERENAMECL, "_utilRecycleReturnInfo::_parseRenameCL" )
   INT32 _utilRecycleReturnInfo::_parseRenameCL( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSERENAMECL ) ;

      rc = _parseNameMap( FIELD_NAME_RENAME_CL, object, _renameCLMap ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse map of rename collections, "
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSERENAMECL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSECHGUIDCL, "_utilRecycleReturnInfo::_parseChangeUIDCL" )
   INT32 _utilRecycleReturnInfo::_parseChangeUIDCL( const BSONObj &object )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSECHGUIDCL ) ;

      rc = _parseUIDMap( FIELD_NAME_CHANGEUID_CL, object, _changeUIDCL ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse map of change unique ID "
                   "collections, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSECHGUIDCL, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSENAMESET, "_utilRecycleReturnInfo::_parseNameSet" )
   INT32 _utilRecycleReturnInfo::_parseNameSet( const CHAR *fieldName,
                                                const BSONObj &object,
                                                UTIL_RETURN_NAME_SET &nameSet )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSENAMESET ) ;

      try
      {
         BSONElement element = object.getField( fieldName ) ;
         if ( EOO == element.type() )
         {
            goto done ;
         }
         PD_CHECK( Array == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not array", fieldName ) ;

         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONElement subElement = iter.next() ;
               PD_CHECK( String == subElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get sub element of field [%s], "
                         "it is no string", fieldName ) ;

               nameSet.insert( subElement.valuestr() ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse name set, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSENAMESET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSENAMEMAP, "_utilRecycleReturnInfo::_parseNameMap" )
   INT32 _utilRecycleReturnInfo::_parseNameMap( const CHAR *fieldName,
                                                const BSONObj &object,
                                                UTIL_RETURN_NAME_MAP &nameMap )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSENAMEMAP ) ;

      try
      {
         BSONElement element = object.getField( fieldName ) ;
         if ( EOO == element.type() )
         {
            goto done ;
         }
         PD_CHECK( Array == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not array", fieldName ) ;

         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONObj subObject ;
               BSONElement keyElement, valueElement ;

               BSONElement subElement = iter.next() ;
               PD_CHECK( Array == subElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get sub element of field [%s], "
                         "it is no array", fieldName ) ;
               subObject = subElement.embeddedObject() ;

               keyElement = subObject.getField( "0" ) ;
               PD_CHECK( String == keyElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get key field, it is not string" ) ;

               valueElement = subObject.getField( "1" ) ;
               PD_CHECK( String == valueElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get value field, it is not string" ) ;

               nameMap.insert( make_pair( keyElement.valuestr(),
                                          valueElement.valuestr() ) ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse name map, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSENAMEMAP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO__PARSEUIDMAP, "_utilRecycleReturnInfo::_parseUIDMap" )
   INT32 _utilRecycleReturnInfo::_parseUIDMap( const CHAR *fieldName,
                                               const BSONObj &object,
                                               UTIL_RETURN_UID_MAP &uidMap )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_UTILRECYRTRNINFO__PARSEUIDMAP ) ;

      try
      {
         BSONElement element = object.getField( fieldName ) ;
         if ( EOO == element.type() )
         {
            goto done ;
         }
         PD_CHECK( Array == element.type(), SDB_SYS, error, PDERROR,
                   "Failed to get field [%s], it is not array", fieldName ) ;

         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               BSONObj subObject ;
               BSONElement keyElement, valueElement ;

               BSONElement subElement = iter.next() ;
               PD_CHECK( Array == subElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get sub element of field [%s], "
                         "it is no array", fieldName ) ;
               subObject = subElement.embeddedObject() ;

               keyElement = subObject.getField( "0" ) ;
               PD_CHECK( NumberLong == keyElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get key field, it is not long number" ) ;

               valueElement = subObject.getField( "1" ) ;
               PD_CHECK( NumberLong == valueElement.type(),
                         SDB_SYS, error, PDERROR,
                         "Failed to get value field, it is not long number" ) ;

               uidMap.insert( make_pair( keyElement.numberLong(),
                                         valueElement.numberLong() ) ) ;
            }
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse name map, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_UTILRECYRTRNINFO__PARSEUIDMAP, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_UTILRECYRTRNINFO_CLEAR, "_utilRecycleReturnInfo::clear" )
   void _utilRecycleReturnInfo::clear()
   {
      _replaceCSSet.clear() ;
      _replaceCLSet.clear() ;
      _renameCSMap.clear() ;
      _renameCLMap.clear() ;
      _changeUIDCL.clear() ;
   }

}
