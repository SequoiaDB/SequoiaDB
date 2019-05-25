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

using namespace bson ;

namespace engine
{

   INT32 coordParseBoolean( BSONElement &e, BOOLEAN &value, UINT32 mask )
   {
      INT32 rc = SDB_INVALIDARG ;
      if ( e.isBoolean() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.boolean() ? TRUE : FALSE ;
         rc = SDB_OK ;
      }
      else if ( e.isNumber() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = 0 != e.numberInt() ? TRUE : FALSE ;
         rc = SDB_OK ;
      }
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
      if ( e.isNumber() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.numberInt() ;
         rc = SDB_OK ;
      }
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
      if ( String == e.type() && ( mask & COORD_PARSE_MASK_ET_DFT ) )
      {
         value = e.valuestr() ;
         rc = SDB_OK ;
      }
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
                                  CoordCataInfoPtr & cataPtr)
   {
      INT32 rc = SDB_OK ;
      CoordCataInfo *pCataInfoTmp = NULL ;

      try
      {
         BSONElement eName, eVersion ;
         eName = obj.getField( CAT_CATALOGNAME_NAME ) ;
         if ( String != eName.type() )
         {
            PD_LOG( PDERROR, "Failed to get field[%s] from obj[%s]",
                    CAT_CATALOGNAME_NAME, obj.toString().c_str() ) ;
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
                                                   eName.valuestr() ) ;
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
                        sub.append( tmpNew ) ;
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
               param._parseMask |= COORD_CTRL_MASK_ROLE ;
               ossMemcpy( param._role, tmpRole, sizeof( tmpRole ) ) ;

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
                        sub.append( tmpNew ) ;
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
                                            pNewObj ? &tmpNew : NULL,
                                            strictCheck ) ;
                  PD_RC_CHECK( rc, PDERROR, "Parse obj[%s] nodes failed ",
                               obj.toString().c_str() ) ;
                  if ( pNewObj )
                  {
                     if ( tmpNew.objdata() != tmpObj.objdata() )
                     {
                        isModify = TRUE ;
                        sub.append( tmpNew ) ;
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

}

