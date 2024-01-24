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

   Source File Name = coordCommon.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/05/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordCommon.hpp"
#include "msgCatalogDef.h"
#include "utilCommon.hpp"
#include "pmdOptions.h"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "coordSequenceAgent.hpp"

using namespace bson ;

namespace engine
{

   INT32 coordParseBoolean( BSONElement &e, BOOLEAN &value, UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      /// a:true
      if ( e.isBoolean() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.boolean() ? TRUE : FALSE ;
         rc = SDB_OK ;
      }
      /// a:1
      else if ( e.isNumber() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = 0 != e.numberInt() ? TRUE : FALSE ;
         rc = SDB_OK ;
      }
      /// a:{$et:true} or a:{$et:1}
      else if ( Object == e.type() && ( mask & COORD_PARSE_MASK_ET_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() &&
              0 == ossStrcmp( "$et", tmpE.fieldName() ) )
         {
            rc = coordParseBoolean( tmpE, value,
                                    COORD_PARSE_MASK_ET_DFT ) ;
         }
      }
      return rc ;
   }

   INT32 coordParseInt( BSONElement &e, INT32 &value, UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      /// a:1
      if ( e.isNumber() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.numberInt() ;
         rc = SDB_OK ;
      }
      /// a:{$et:1}
      else if ( Object == e.type() && ( mask & COORD_PARSE_MASK_ET_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() &&
              0 == ossStrcmp( "$et", tmpE.fieldName() ) )
         {
            rc = coordParseInt( tmpE, value,
                                COORD_PARSE_MASK_ET_DFT ) ;
         }
      }
      return rc ;
   }

   INT32 coordParseInt( BSONElement &e, vector<INT32> &vecValue, UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      INT32 value = 0 ;

      /// a:1 or a:{$et:1}
      if ( ( !e.isABSONObj() ||
             0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                             "$et") ) &&
           ( mask & COORD_PARSE_MASK_ET ) )
      {
         rc = coordParseInt( e, value, mask ) ;
         if ( SDB_OK == rc )
         {
            vecValue.push_back( value ) ;
         }
      }
      /// a:[1,2,3]
      else if ( Array == e.type() && ( mask & COORD_PARSE_MASK_IN_DFT ) )
      {
         BSONObjIterator it( e.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONElement tmpE = it.next() ;
            rc = coordParseInt( tmpE, value,
                                COORD_PARSE_MASK_ET_DFT ) ;
            if ( rc )
            {
               break ;
            }
            vecValue.push_back( value ) ;
         }
      }
      /// a:{$in:[1,2,3]}
      else if ( Object == e.type() &&
                0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                                "$in") &&
                ( mask & COORD_PARSE_MASK_IN_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() )
         {
            rc = coordParseInt( tmpE, vecValue,
                                COORD_PARSE_MASK_IN_DFT ) ;
         }
      }
      return rc ;
   }

   INT32 coordParseString( BSONElement &e, const CHAR *&value,
                           UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      /// a:"xxx"
      if ( String == e.type() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.valuestr() ;
         rc = SDB_OK ;
      }
      /// a:{$et:"xxx"}
      else if ( Object == e.type() && ( mask & COORD_PARSE_MASK_ET_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() &&
              0 == ossStrcmp( "$et", tmpE.fieldName() ) )
         {
            rc = coordParseString( tmpE, value,
                                   COORD_PARSE_MASK_ET_DFT ) ;
         }
      }
      return rc ;
   }

   INT32 coordParseString( BSONElement &e,
                           vector<const CHAR*> &vecValue,
                           UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      const CHAR *value = NULL ;

      /// a:"xxx" or a:{$et:"xxx"}
      if ( ( !e.isABSONObj() ||
             0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                             "$et") ) &&
           ( mask & COORD_PARSE_MASK_ET ) )
      {
         rc = coordParseString( e, value, mask ) ;
         if ( SDB_OK == rc )
         {
            vecValue.push_back( value ) ;
         }
      }
      /// a:["xxx", "yyy", "zzz"]
      else if ( Array == e.type() && ( mask & COORD_PARSE_MASK_IN_DFT ) )
      {
         BSONObjIterator it( e.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONElement tmpE = it.next() ;
            rc = coordParseString( tmpE, value,
                                   COORD_PARSE_MASK_ET_DFT ) ;
            if ( rc )
            {
               break ;
            }
            vecValue.push_back( value ) ;
         }
      }
      /// a:{$in:["xxx", "yyy", "zzz"]}
      else if ( Object == e.type() &&
                0 == ossStrcmp( e.embeddedObject().firstElement().fieldName(),
                                "$in") &&
                ( mask & COORD_PARSE_MASK_IN_OPR ) )
      {
         BSONObj obj = e.embeddedObject() ;
         BSONElement tmpE = obj.firstElement() ;
         if ( 1 == obj.nFields() )
         {
            rc = coordParseString( tmpE, vecValue,
                                   COORD_PARSE_MASK_IN_DFT ) ;
         }
      }
      return rc ;
   }

   INT32 coordInitCataPtrFromObj( const BSONObj &obj,
                                  CoordCataInfoPtr & cataPtr )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfo *pCataInfoTmp = NULL ;

      try
      {
         BSONElement eName, eID, eVersion ;
         utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ;

         // Check types of name, id, version elements
         eName = obj.getField( CAT_CATALOGNAME_NAME ) ;
         if ( String != eName.type() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s]",
                    CAT_CATALOGNAME_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         eID = obj.getField( CAT_CL_UNIQUEID ) ;
         if ( eID.eoo() )
         {
            // it is ok, catalog hasn't been upgraded to new version.
         }
         else if ( eID.isNumber() )
         {
            clUniqueID = ( utilCLUniqueID ) eID.numberLong() ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s]",
                    CAT_CL_UNIQUEID, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         eVersion = obj.getField( CAT_CATALOGVERSION_NAME ) ;
         if ( !eVersion.isNumber() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s]",
                    CAT_CATALOGVERSION_NAME, obj.toString().c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         pCataInfoTmp = SDB_OSS_NEW CoordCataInfo( eVersion.number(),
                                                   eName.valuestr(),
                                                   clUniqueID ) ;
         if ( !pCataInfoTmp )
         {
            PD_LOG( PDERROR, "Allocate memory for catalog info failed" ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         cataPtr = CoordCataInfoPtr( pCataInfoTmp ) ;
         rc = cataPtr->fromBSONObj( obj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to init catalog info from obj[%s], "
                    "rc: %d", obj.toString().c_str(), rc ) ;
            goto error ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG ( PDERROR, "Occur exception when parse catalog info "
                  "object: %s", e.what() ) ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN coordIsCataAddrSame( const CoordVecNodeInfo &left,
                                const CoordVecNodeInfo &right )
   {
      if ( left.size() != right.size() )
      {
         return FALSE ;
      }
      for ( UINT32 i = 0 ; i < left.size() ; ++i )
      {
         const clsNodeItem &leftItem = left[i] ;
         const clsNodeItem &rightItem = right[i] ;

         if ( 0 != ossStrcmp( leftItem._host, rightItem._host ) ||
              0 != ossStrcmp( leftItem._service[MSG_ROUTE_CAT_SERVICE].c_str(),
                              rightItem._service[MSG_ROUTE_CAT_SERVICE].c_str() ) )
         {
            return FALSE ;
         }
      }
      return TRUE ;
   }

   BOOLEAN coordCataCheckFlag( INT32 flag )
   {
      return ( SDB_CLS_COORD_NODE_CAT_VER_OLD == flag ||
               SDB_CLS_NO_CATALOG_INFO == flag ||
               SDB_CLS_GRP_NOT_EXIST == flag ||
               SDB_CLS_NODE_NOT_EXIST == flag ||
               SDB_CAT_NO_MATCH_CATALOG == flag ) ;
   }

   // return TRUE if we should delete the node
   BOOLEAN  coordCheckNodeReplyFlag( INT32 flag )
   {
      switch( flag )
      {
      case SDB_INVALIDARG:
      case SDB_DMS_RECORD_TOO_BIG:
      case SDB_IXM_MULTIPLE_ARRAY:
      case SDB_IXM_DUP_KEY:
      case SDB_IXM_KEY_TOO_LARGE:
      case SDB_CLS_COORD_NODE_CAT_VER_OLD:
         return FALSE ;
      default:
         return TRUE ;
      }
   }

   BSONObj* coordGetFilterByID( FILTER_BSON_ID filterID,
                                rtnQueryOptions &queryOption )
   {
      BSONObj *pFilter = NULL ;
      switch ( filterID )
      {
         case FILTER_ID_SELECTOR:
            pFilter = queryOption.getSelectorPtr() ;
            break ;
         case FILTER_ID_ORDERBY:
            pFilter = queryOption.getOrderByPtr() ;
            break ;
         case FILTER_ID_HINT:
            pFilter = queryOption.getHintPtr() ;
            break ;
         default:
            pFilter = queryOption.getQueryPtr() ;
            break ;
      }
      return pFilter ;
   }

   INT32 coordParseControlParam( const BSONObj &obj,
                                 coordCtrlParam &param,
                                 UINT32 mask,
                                 BSONObj *pNewObj,
                                 BOOLEAN strictCheck )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN modify = FALSE ;
      BSONObjBuilder builder ;
      const CHAR *tmpStr = NULL ;
      vector< const CHAR* > tmpVecStr ;

      BSONObjIterator it( obj ) ;
      while( it.more() )
      {
         BSONElement e = it.next() ;

         /// $and:[{a:1},{b:2}]
         if ( Array == e.type() &&
              0 == ossStrcmp( e.fieldName(), "$and" ) )
         {
            BSONArrayBuilder sub( builder.subarrayStart( e.fieldName() ) ) ;
            BSONObj tmpNew ;
            BSONObjIterator tmpItr( e.embeddedObject() ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpE = tmpItr.next() ;
               if ( Object != tmpE.type() )
               {
                  PD_LOG( PDERROR, "Parse obj[%s] conrtol param failed: "
                          "invalid $and", obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj tmpObj = tmpE.embeddedObject() ;
                  rc = coordParseControlParam( tmpObj, param, mask,
                                               pNewObj ? &tmpNew : NULL,
                                               strictCheck ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] conrtol param "
                               "failed", obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        modify = TRUE ;

                        if ( !tmpNew.isEmpty() )
                        {
                           sub.append( tmpNew ) ;
                        }
                     }
                     else
                     {
                        sub.append( tmpObj ) ;
                     }
                  }
               }
            }
            sub.done() ;
         } /// end $and
         else if ( ( mask & COORD_CTRL_MASK_GLOBAL ) &&
                   0 == ossStrcasecmp( e.fieldName(), FIELD_NAME_GLOBAL ) )
         {
            rc = coordParseBoolean( e, param._isGlobal,
                                    COORD_PARSE_MASK_ET ) ;
            if ( SDB_OK == rc )
            {
               modify = TRUE ;
               param._parseMask |= COORD_CTRL_MASK_GLOBAL ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( ( mask & COORD_CTRL_MASK_GLOBAL ) &&
                   e.isNull() &&
                   ( 0 == ossStrcasecmp( e.fieldName(),
                                        FIELD_NAME_GROUPS ) ||
                     0 == ossStrcasecmp( e.fieldName(),
                                        FIELD_NAME_GROUPNAME ) ||
                     0 == ossStrcasecmp( e.fieldName(),
                                        FIELD_NAME_GROUPID ) )
                 )
         {
            param._isGlobal = FALSE ;
            modify = TRUE ;
            param._parseMask |= COORD_CTRL_MASK_GLOBAL ;
         }
         else if ( ( mask & COORD_CTRL_MASK_NODE_SELECT ) &&
                   0 == ossStrcasecmp( e.fieldName(),
                                       FIELD_NAME_NODE_SELECT ) )
         {
            rc = coordParseString( e, tmpStr, COORD_PARSE_MASK_ET ) ;
            if ( SDB_OK == rc )
            {
               if ( 0 == ossStrcasecmp( tmpStr, "primary" ) ||
                    0 == ossStrcasecmp( tmpStr, "master" ) ||
                    0 == ossStrcasecmp( tmpStr, "m" ) ||
                    0 == ossStrcasecmp( tmpStr, "p" ) )
               {
                  param._emptyFilterSel = NODE_SEL_PRIMARY ;
               }
               else if ( 0 == ossStrcasecmp( tmpStr, "any" ) ||
                         0 == ossStrcasecmp( tmpStr, "a" ) )
               {
                  param._emptyFilterSel = NODE_SEL_ANY ;
               }
               else if ( 0 == ossStrcasecmp( tmpStr, "secondary" ) ||
                         0 == ossStrcasecmp( tmpStr, "s" ) )
               {
                  param._emptyFilterSel = NODE_SEL_SECONDARY ;
               }
               else if ( 0 == ossStrcasecmp( tmpStr, "all" ) )
               {
                  param._emptyFilterSel = NODE_SEL_ALL ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
               }
            }

            if ( SDB_OK == rc )
            {
               modify = TRUE ;
               param._parseMask |= COORD_CTRL_MASK_NODE_SELECT ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( ( mask & COORD_CTRL_MASK_ROLE ) &&
                   0 == ossStrcasecmp( e.fieldName(),
                                       FIELD_NAME_ROLE ) )
         {
            INT32 tmpRole[ SDB_ROLE_MAX ] = { 0 } ;
            rc = coordParseString( e, tmpVecStr,
                                   COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               for ( UINT32 i = 0 ; i < tmpVecStr.size() ; ++i )
               {
                  if ( 0 == ossStrcasecmp( tmpVecStr[i], "all" ) )
                  {
                     for ( UINT32 k = 0 ; k < (UINT32)SDB_ROLE_MAX ; ++k )
                     {
                        tmpRole[ k ] = 1 ;
                     }
                     break ;
                  }
                  else if ( 0 == *(tmpVecStr[i]) )
                  {
                     rc = SDB_INVALIDARG ;
                     break ;
                  }
                  if ( SDB_ROLE_MAX != utilGetRoleEnum( tmpVecStr[ i ] ) )
                  {
                     tmpRole[ utilGetRoleEnum( tmpVecStr[ i ] ) ] = 1 ;
                  }
                  else
                  {
                     rc = SDB_INVALIDARG ;
                     break ;
                  }
               }
            }

            if ( SDB_OK == rc )
            {
               modify = TRUE ;
               param.setParseRole( tmpRole ) ;

               if ( mask & COORD_CTRL_MASK_GLOBAL )
               {
                  param._isGlobal = TRUE ;
               }
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
            tmpVecStr.clear() ;
         }
         else if ( ( mask & COORD_CTRL_MASK_ROLE ) &&
                   ( 0 == ossStrcasecmp( e.fieldName(), FIELD_NAME_INSTANCEID ) ||
                     0 == ossStrcasecmp( e.fieldName(), PMD_OPTION_INSTANCE_ID ) ) )
         {
            // Instance ID could be only used in DATA nodes
            INT32 tmpRole[ SDB_ROLE_MAX ] = { 0 } ;
            tmpRole[ SDB_ROLE_DATA ] = 1 ;
            modify = TRUE ;
            param.setParseRole( tmpRole ) ;

            if ( mask & COORD_CTRL_MASK_GLOBAL )
            {
               param._isGlobal = TRUE ;
            }
            if ( pNewObj )
            {
               builder.append( e ) ;
            }
         }
         else if ( ( mask & COORD_CTRL_MASK_RAWDATA ) &&
                   0 == ossStrcasecmp( e.fieldName(), FIELD_NAME_RAWDATA ) )
         {
            rc = coordParseBoolean( e, param._rawData,
                                    COORD_PARSE_MASK_ET ) ;
            if ( SDB_OK == rc )
            {
               modify = TRUE ;
               param._parseMask |= COORD_CTRL_MASK_RAWDATA ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( pNewObj )
         {
            builder.append( e ) ;
         }
      }

      if ( pNewObj )
      {
         if ( modify )
         {
            *pNewObj = builder.obj() ;
         }
         else
         {
            *pNewObj = obj ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordParseGroupsInfo( const BSONObj &obj,
                               vector< INT32 > &vecID,
                               vector< const CHAR* > &vecName,
                               BSONObj *pNewObj,
                               BOOLEAN strictCheck )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder( obj.objsize() ) ;
      BOOLEAN isModify = FALSE ;

      BSONObjIterator it( obj ) ;
      while( it.more() )
      {
         BSONElement ele = it.next() ;

         // $and:[{GroupID:1000}, {GroupName:"xxx"}]
         if ( Array == ele.type() &&
              0 == ossStrcmp( ele.fieldName(), "$and" ) )
         {
            BSONArrayBuilder sub( builder.subarrayStart( ele.fieldName() ) ) ;
            BSONObj tmpNew ;
            BSONObjIterator tmpItr( ele.embeddedObject() ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpE = tmpItr.next() ;
               if ( Object != tmpE.type() )
               {
                  PD_LOG( PDERROR, "Parse obj[%s] groups failed: "
                          "invalid $and", obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj tmpObj = tmpE.embeddedObject() ;
                  rc = coordParseGroupsInfo( tmpObj, vecID, vecName,
                                             pNewObj ? &tmpNew : NULL,
                                             strictCheck ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] groups failed",
                               obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        isModify = TRUE ;

                        if ( !tmpNew.isEmpty() )
                        {
                           sub.append( tmpNew ) ;
                        }
                     }
                     else
                     {
                        sub.append( tmpObj ) ;
                     }
                  }
               }
            }
            sub.done() ;
         } /// end $and
         // group id
         else if ( 0 == ossStrcasecmp( ele.fieldName(), CAT_GROUPID_NAME ) )
         {
            rc = coordParseInt( ele, vecID, COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         // group name
         else if ( ( 0 == ossStrcasecmp( ele.fieldName(),
                                         FIELD_NAME_GROUPNAME ) ||
                     0 == ossStrcasecmp( ele.fieldName(),
                                         FIELD_NAME_GROUPS ) ) )
         {
            rc = coordParseString( ele, vecName,
                                   COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( pNewObj )
         {
            builder.append( ele ) ;
         }
      }

      if ( pNewObj )
      {
         if ( isModify )
         {
            *pNewObj = builder.obj() ;
         }
         else
         {
            *pNewObj = obj ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordParseNodesInfo( const BSONObj &obj,
                              vector< INT32 > &vecNodeID,
                              vector< const CHAR* > &vecHostName,
                              vector< const CHAR* > &vecSvcName,
                              vector< const CHAR* > &vecNodeName,
                              vector< INT32 > & vecInstanceID,
                              BSONObj *pNewObj,
                              BOOLEAN strictCheck )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder( obj.objsize() ) ;
      BOOLEAN isModify = FALSE ;

      BSONObjIterator itr( obj ) ;
      while( itr.more() )
      {
         BSONElement ele = itr.next() ;

         // $and:[{NodeID:1001, HostName:"xxxx" }]
         if ( Array == ele.type() &&
              0 == ossStrcmp( ele.fieldName(), "$and" ) )
         {
            BSONArrayBuilder sub( builder.subarrayStart( ele.fieldName() ) ) ;
            BSONObj tmpNew ;
            BSONObjIterator tmpItr( ele.embeddedObject() ) ;
            while ( tmpItr.more() )
            {
               BSONElement tmpE = tmpItr.next() ;
               if ( Object != tmpE.type() )
               {
                  PD_LOG( PDERROR, "Parse obj[%s] nodes failed: "
                          "invalid $and", obj.toString().c_str() ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  BSONObj tmpObj = tmpE.embeddedObject() ;
                  rc = coordParseNodesInfo( tmpObj, vecNodeID,
                                            vecHostName, vecSvcName,
                                            vecNodeName, vecInstanceID,
                                            pNewObj ? &tmpNew : NULL,
                                            strictCheck ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] nodes failed ",
                               obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        isModify = TRUE ;

                        if ( !tmpNew.isEmpty() )
                        {
                           sub.append( tmpNew ) ;
                        }
                     }
                     else
                     {
                        sub.append( tmpObj ) ;
                     }
                  }
               }
            }
            sub.done() ;
         } /// end $and
         else if ( 0 == ossStrcasecmp( ele.fieldName(), CAT_NODEID_NAME ) )
         {
            rc = coordParseInt( ele, vecNodeID,
                                COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( 0 == ossStrcasecmp( ele.fieldName(), FIELD_NAME_HOST ) )
         {
            rc = coordParseString( ele, vecHostName,
                                   COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( ( 0 == ossStrcasecmp( ele.fieldName(),
                                         FIELD_NAME_SERVICE_NAME ) ||
                     0 == ossStrcasecmp( ele.fieldName(),
                                         PMD_OPTION_SVCNAME ) ) )
         {
            rc = coordParseString( ele, vecSvcName,
                                   COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( 0 == ossStrcasecmp( ele.fieldName(),
                                       FIELD_NAME_NODE_NAME ) )
         {
            rc = coordParseString( ele, vecNodeName,
                                   COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               rc = coordCheckNodeName( vecNodeName ) ;
            }

            /// check result
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( 0 == ossStrcasecmp( ele.fieldName(),
                                       FIELD_NAME_INSTANCEID ) ||
                   0 == ossStrcasecmp( ele.fieldName(),
                                       PMD_OPTION_INSTANCE_ID ) )
         {
            rc = coordParseInt( ele, vecInstanceID, COORD_PARSE_MASK_ALL ) ;
            if ( SDB_OK == rc )
            {
               rc = coordCheckInstanceID( vecInstanceID ) ;
            }

            /// check result
            if ( SDB_OK == rc )
            {
               isModify = TRUE ;
            }
            else if ( strictCheck )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
            }
         }
         else if ( pNewObj )
         {
            builder.append( ele ) ;
         } // end if
      } // end while

      if ( pNewObj )
      {
         if ( isModify )
         {
            *pNewObj = builder.obj() ;
         }
         else
         {
            *pNewObj = obj ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void coordFilterGroupsByRole( CoordGroupList &groupList,
                                 INT32 *pRoleFilter )
   {
      CoordGroupList::iterator it = groupList.begin() ;
      while( it != groupList.end() )
      {
         if ( ( !pRoleFilter[ SDB_ROLE_DATA ] &&
                it->second >= DATA_GROUP_ID_BEGIN &&
                it->second <= DATA_GROUP_ID_END ) ||
              ( !pRoleFilter[ SDB_ROLE_CATALOG ] &&
                CATALOG_GROUPID == it->second ) ||
              ( !pRoleFilter[ SDB_ROLE_COORD ] &&
                COORD_GROUPID == it->second ) )
         {
            groupList.erase( it++ ) ;
         }
         else
         {
            ++it ;
         }
      }
   }

   void coordFilterNodesByRole( SET_ROUTEID &nodes, INT32 *pRoleFilter )
   {
      MsgRouteID nodeID ;
      SET_ROUTEID::iterator it = nodes.begin() ;
      while( it != nodes.end() )
      {
         nodeID.value = *it ;

         if ( ( !pRoleFilter[ SDB_ROLE_DATA ] &&
                nodeID.columns.groupID >= DATA_GROUP_ID_BEGIN &&
                nodeID.columns.groupID <= DATA_GROUP_ID_END ) ||
              ( !pRoleFilter[ SDB_ROLE_CATALOG ] &&
                CATALOG_GROUPID == nodeID.columns.groupID ) ||
              ( !pRoleFilter[ SDB_ROLE_COORD ] &&
                COORD_GROUPID == nodeID.columns.groupID ) )
         {
            nodes.erase( it++ ) ;
         }
         else
         {
            ++it ;
         }
      }
   }

   INT32 coordCheckNodeName( const CHAR *pNodeName )
   {
      /*
         Valid format : HostName:svcname[:svcname2[:svcname3]...]
      */
      INT32 rc = SDB_INVALIDARG ;

      if ( pNodeName )
      {
         const CHAR *pCh = ossStrchr( pNodeName, ':' ) ;
         if ( pCh && pCh[1] && ':' != pCh[1] )
         {
            rc = SDB_OK ;
         }
         else
         {
            PD_LOG_MSG( PDERROR, "Invalid node name: %s", pNodeName ) ;
         }
      }

      return rc ;
   }

   INT32 coordCheckNodeName( const vector <const CHAR*> &vecNodeName )
   {
      INT32 rc = SDB_OK ;

      for ( UINT32 i = 0 ; i < vecNodeName.size() ; ++i )
      {
         rc = coordCheckNodeName( vecNodeName[i] ) ;
         if ( rc )
         {
            break ;
         }
      }

      return rc ;
   }

   INT32 coordCheckInstanceID ( const vector< INT32 > & vecInstanceID )
   {
      INT32 rc = SDB_OK ;

      for ( UINT32 i = 0 ; i < vecInstanceID.size() ; ++ i )
      {
         if ( !utilCheckInstanceID( (UINT32)( vecInstanceID[ i ] ), FALSE ) )
         {
            rc = SDB_INVALIDARG ;
            break ;
         }
      }

      return rc ;
   }

   BOOLEAN coordMatchNodeName( const CHAR *pNodeName,
                               const CHAR *pHostName,
                               const CHAR *pSvcName )
   {
      /*
         Valid format : HostName:svcname[:svcname2[:svcname3]...]
      */
      BOOLEAN hasMatch = FALSE ;

      const CHAR *p = NULL ;
      const CHAR *pn = NULL ;
      INT32 hostNameLen = 0 ;
      INT32 svcNameLen = 0 ;

      if( NULL == pHostName || NULL == pSvcName )
      {
         // not match
         goto done ;
      }

      hostNameLen = ossStrlen( pHostName ) ;
      svcNameLen = ossStrlen( pSvcName ) ;

      /// HostName match
      p = ossStrchr( pNodeName, ':' ) ;
      if ( !p ||
           hostNameLen != p - pNodeName ||
           0 != ossStrncmp( pHostName, pNodeName, p - pNodeName ) )
      {
         goto done ;
      }

      /// Service name match
      ++p ;
      pn = ossStrchr( p, ':' ) ;
      while( pn )
      {
         if ( svcNameLen == pn - p &&
              0 == ossStrncmp( pSvcName, p, pn - p ) )
         {
            hasMatch = TRUE ;
            goto done ;
         }
         p = pn + 1 ;
         pn = ossStrchr( p, ':' ) ;
      }

      /// The last service name
      if ( *p && 0 == ossStrcmp( pSvcName, p ) )
      {
         hasMatch = TRUE ;
         goto done ;
      }

   done:
      return hasMatch ;
   }

   BOOLEAN coordMatchNodeName( const vector<const CHAR*> &vecNodeName,
                               const CHAR *pHostName,
                               const CHAR *pSvcName )
   {
      BOOLEAN hasMatch = FALSE ;

      for ( UINT32 i = 0 ; i < vecNodeName.size() ; ++i )
      {
         hasMatch = coordMatchNodeName( vecNodeName[i], pHostName, pSvcName ) ;
         if ( hasMatch )
         {
            break ;
         }
      }

      return hasMatch ;
   }

   INT32 coordInvalidateSequenceCache( CoordCataInfoPtr cataPtr, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      const clsAutoIncItem* pItem = NULL ;
      const CHAR *seqName = NULL ;
      utilSequenceID seqID ;

      if ( cataPtr->hasAutoIncrement() )
      {
         const clsAutoIncSet &set = cataPtr->getAutoIncSet() ;
         clsAutoIncIterator it( set, clsAutoIncIterator::RECURS ) ;

         while ( it.more() )
         {
            pItem = it.next() ;
            seqName = pItem->sequenceName() ;
            seqID = pItem->sequenceID() ;

            rc = coordSequenceInvalidateCache( seqName, seqID, cb ) ;
            if ( SDB_SEQUENCE_NOT_EXIST == rc )
            {
               PD_LOG( PDWARNING, "Sequence not found, name[%s], id[%llu]",
                       seqName, seqID ) ;
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR,
                         "Failed to invalidate cache of sequence[%s], rc: %d",
                         seqName, rc ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 coordParseShowErrorHint ( const BSONObj & hint,
                                   UINT32 mask,
                                   COORD_SHOWERROR_TYPE & showError,
                                   COORD_SHOWERRORMODE_TYPE & showErrorMode )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr ( hint.getObjectField( "$Options" ) ) ;
         BSONElement elem ;
         while ( itr.more() )
         {
            elem = itr.next() ;
            if ( 0 == ossStrcasecmp( elem.fieldName(), COORD_SHOWERROR ) )
            {
               PD_CHECK( String == elem.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field [%s] is not string", COORD_SHOWERROR ) ;
               if ( 0 == ossStrcasecmp( elem.valuestr(),
                                        COORD_SHOWERROR_VALUE_SHOW ) )
               {
                  if ( mask & COORD_MASK_SHOWERROR_SHOW )
                  {
                     showError = COORD_SHOWERROR_SHOW ;
                  }
               }
               else if ( 0 == ossStrcasecmp( elem.valuestr(),
                                             COORD_SHOWERROR_VALUE_IGNORE ) )
               {
                  if ( mask & COORD_MASK_SHOWERROR_IGNORE )
                  {
                     showError = COORD_SHOWERROR_IGNORE ;
                  }
               }
               else if ( 0 == ossStrcasecmp( elem.valuestr(),
                                             COORD_SHOWERROR_VALUE_ONLY ) )
               {
                  if ( mask & COORD_MASK_SHOWERROR_ONLY )
                  {
                     showError = COORD_SHOWERROR_ONLY ;
                  }
               }
               else
               {
                  PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                            "Field [%s] is unknown option [%s]",
                            COORD_SHOWERROR, elem.valuestr() ) ;
               }
            }
            else if ( 0 == ossStrcasecmp( elem.fieldName(),
                                          COORD_SHOWERRORMODE ) )
            {
               PD_CHECK( String == elem.type(), SDB_INVALIDARG, error, PDERROR,
                         "Field [%s] is not string", COORD_SHOWERRORMODE ) ;
               if ( 0 == ossStrcasecmp( elem.valuestr(),
                                        COORD_SHOWERRORMODE_VALUE_AGGR ) )
               {
                  if ( mask & COORD_MASK_SHOWERRORMODE_AGGR )
                  {
                     showErrorMode = COORD_SHOWERRORMODE_AGGR ;
                  }
               }
               else if ( 0 == ossStrcasecmp( elem.valuestr(),
                                             COORD_SHOWERRORMODE_VALUE_FLAT ) )
               {
                  if ( mask & COORD_MASK_SHOWERRORMODE_FLAT )
                  {
                     showErrorMode = COORD_SHOWERRORMODE_FLAT ;
                  }
               }
               else
               {
                  PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                            "Field [%s] is unknown option [%s]",
                            COORD_SHOWERRORMODE, elem.valuestr() ) ;
               }
            }
         }
      }
      catch ( std::exception & e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to handle hints, received unexpected "
                 "error:%s", e.what() ) ;
         goto error ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

}

