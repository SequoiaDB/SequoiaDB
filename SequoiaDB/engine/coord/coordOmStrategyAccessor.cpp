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

   Source File Name = coordOmStrategyAccessor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordOmStrategyAccessor.hpp"
#include "omDef.hpp"
#include "pmd.hpp"
#include "coordCB.hpp"
#include "sdbIOmProxy.hpp"
#include "rtn.hpp"
#include "rtnCB.hpp"
#include "pd.hpp"
#include "coordTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _coordOmStrategyAccessor implement
   */
   _coordOmStrategyAccessor::_coordOmStrategyAccessor( INT64 timeout )
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      CoordCB *pCoord = krcb->getCoordCB() ;
      pmdOptionsCB *optionsCB = krcb->getOptionCB() ;

      _pOmProxy = pCoord->getResource()->getOmProxy() ;
      _oprTimeout = timeout ;

      optionsCB->getFieldStr( PMD_OPTION_CLUSTER_NAME, _clsName, "" ) ;
      optionsCB->getFieldStr( PMD_OPTION_BUSINESS_NAME, _bizName, "" ) ;
   }

   _coordOmStrategyAccessor::~_coordOmStrategyAccessor()
   {
      _pOmProxy = NULL ;
   }

   void _coordOmStrategyAccessor::setTimeout( INT64 timeout )
   {
      _oprTimeout = timeout ;
   }

   INT32 _coordOmStrategyAccessor::getMetaInfoFromOm( omStrategyMetaInfo &metaInfo,
                                                      pmdEDUCB *cb,
                                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      rtnQueryOptions queryOpt ;
      INT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      BSONObj tmpObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;

      try
      {
         condition = BSON( FIELD_NAME_NAME << OM_STRATEGY_BS_TASK_META_NAME <<
                           OM_BSON_CLUSTER_NAME << _clsName <<
                           OM_BSON_BUSINESS_NAME << _bizName ) ;

         queryOpt.setCLFullName( OM_CS_STRATEGY_CL_META_DATA ) ;
         queryOpt.setQuery( condition ) ;
         queryOpt.setLimit( 1 ) ;
         queryOpt.setFlag( FLG_QUERY_WITH_RETURNDATA ) ;

         _pOmProxy->setOprTimeout( _oprTimeout ) ;
         rc = _pOmProxy->queryOnOm( queryOpt, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query strategy meta from om failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         rc = rtnGetMore( contextID, 1, bufObj, cb, rtnCB ) ;
         if ( rc )
         {
            contextID = -1 ;
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_OK ;
               goto done ;
            }
            PD_LOG( PDERROR, "Getmore failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = bufObj.nextObj( tmpObj ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = metaInfo.fromBSON( tmpObj ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordOmStrategyAccessor::getStrategyInfoFromOm(
                                       vector<omTaskStrategyInfoPtr> &vecInfo,
                                       pmdEDUCB *cb,
                                       rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      rtnQueryOptions queryOpt ;
      INT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      BSONObj tmpObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      omTaskStrategyInfo *pStrategyInfo = NULL ;

      try
      {
         condition = BSON( OM_REST_FIELD_STATUS << OM_STRATEGY_STATUS_ENABLE <<
                           OM_BSON_CLUSTER_NAME << _clsName <<
                           OM_BSON_BUSINESS_NAME << _bizName ) ;

         queryOpt.setCLFullName( OM_CS_STRATEGY_CL_STRATEGY_PRO ) ;
         queryOpt.setQuery( condition ) ;
         queryOpt.setFlag( FLG_QUERY_WITH_RETURNDATA ) ;

         _pOmProxy->setOprTimeout( _oprTimeout ) ;
         rc = _pOmProxy->queryOnOm( queryOpt, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query strategy meta from om failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         while( TRUE )
         {
            rc = rtnGetMore( contextID, -1, bufObj, cb, rtnCB ) ;
            if ( rc )
            {
               contextID = -1 ;
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }
               PD_LOG( PDERROR, "Getmore failed, rc: %d", rc ) ;
               goto error ;
            }

            while( TRUE )
            {
               rc = bufObj.nextObj( tmpObj ) ;
               if ( rc )
               {
                  if ( SDB_DMS_EOC == rc )
                  {
                     rc = SDB_OK ;
                  }
                  break ;
               }

               pStrategyInfo = SDB_OSS_NEW omTaskStrategyInfo() ;
               if ( !pStrategyInfo )
               {
                  PD_LOG( PDERROR, "Allocate task strategy info failed " ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
               vecInfo.push_back( omTaskStrategyInfoPtr( pStrategyInfo ) ) ;

               rc = pStrategyInfo->fromBSON( tmpObj ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      goto done ;
   }

   INT32 _coordOmStrategyAccessor::getTaskInfoFromOm( vector<omTaskInfoPtr> &vecInfo,
                                                      pmdEDUCB *cb,
                                                      rtnContextBuf *buf )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      rtnQueryOptions queryOpt ;
      INT64 contextID = -1 ;
      rtnContextBuf bufObj ;
      BSONObj tmpObj ;
      SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
      omTaskInfo *pTaskInfo = NULL ;

      try
      {
         condition = BSON( OM_REST_FIELD_STATUS << OM_STRATEGY_STATUS_ENABLE <<
                           OM_BSON_CLUSTER_NAME << _clsName <<
                           OM_BSON_BUSINESS_NAME << _bizName ) ;

         queryOpt.setCLFullName( OM_CS_STRATEGY_CL_TASK_PRO ) ;
         queryOpt.setQuery( condition ) ;
         queryOpt.setFlag( FLG_QUERY_WITH_RETURNDATA ) ;

         _pOmProxy->setOprTimeout( _oprTimeout ) ;
         rc = _pOmProxy->queryOnOm( queryOpt, cb, contextID, buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Query strategy meta from om failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         while( TRUE )
         {
            rc = rtnGetMore( contextID, -1, bufObj, cb, rtnCB ) ;
            if ( rc )
            {
               contextID = -1 ;
               if ( SDB_DMS_EOC == rc )
               {
                  rc = SDB_OK ;
                  break ;
               }
               PD_LOG( PDERROR, "Getmore failed, rc: %d", rc ) ;
               goto error ;
            }

            while( TRUE )
            {
               rc = bufObj.nextObj( tmpObj ) ;
               if ( rc )
               {
                  if ( SDB_DMS_EOC == rc )
                  {
                     rc = SDB_OK ;
                  }
                  break ;
               }

               pTaskInfo = SDB_OSS_NEW omTaskInfo() ;
               if ( !pTaskInfo )
               {
                  PD_LOG( PDERROR, "Allocate task info failed " ) ;
                  rc = SDB_OOM ;
                  goto error ;
               }
               vecInfo.push_back( omTaskInfoPtr( pTaskInfo ) ) ;

               rc = pTaskInfo->fromBSON( tmpObj ) ;
               if ( rc )
               {
                  goto error ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( -1 != contextID )
      {
         rtnCB->contextDelete( contextID, cb ) ;
      }
      goto done ;
   }

}

