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

   Source File Name = optAccessPlanRuntime.hpp

   Descriptive Name = Optimizer Access Plan Runtime Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains runtime structure for
   access plan.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2017  HGM  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPTACCESSPLANRUNTIME_HPP__
#define OPTACCESSPLANRUNTIME_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "monCB.hpp"
#include "mthMatchRuntime.hpp"
#include "optAccessPlan.hpp"

using namespace bson ;

namespace engine
{
   class _rtnContextBase ;
   typedef class _rtnContextBase rtnContext ;

   class _optAccessPlanManager ;
   typedef class _optAccessPlanManager optAccessPlanManager ;

   /*
      _optQueryActivity define
    */
   class _optQueryActivity
   {
      public :
         _optQueryActivity () ;

         _optQueryActivity ( MON_OPERATION_TYPES optrType,
                             const monContextCB &monCtxCB,
                             const rtnReturnOptions &returnOptions,
                             BOOLEAN hitEnd ) ;

         virtual ~_optQueryActivity () ;

         virtual void reset () ;

         _optQueryActivity & operator = ( const _optQueryActivity & activity ) ;

         void toBSON ( BSONObjBuilder &builder ) const ;

         OSS_INLINE MON_OPERATION_TYPES getOptrType () const
         {
            return _optrType ;
         }

         OSS_INLINE BSONObj getParameters () const
         {
            return _parameters ;
         }

         void setParameters ( const rtnParamList & parameters ) ;

         OSS_INLINE const ossTickDelta & getQueryTime () const
         {
            return _contextMonitor.getQueryTime() ;
         }

         OSS_INLINE BOOLEAN isValid () const
         {
            return ( -1 != _contextMonitor.getContextID() ||
                     MON_COUNTER_OPERATION_NONE != _optrType ) ;
         }

      protected :
         MON_OPERATION_TYPES  _optrType ;
         BSONObj              _parameters ;
         rtnReturnOptions     _returnOptions ;
         monContextCB         _contextMonitor ;
         BOOLEAN              _hitEnd ;
   } ;

   typedef class _optQueryActivity optQueryActivity ;

   /*
      _optCLScanInfo define
    */
   class _optCLScanInfo : public SDBObject,
                          public _optCollectionInfo
   {
      public :
         _optCLScanInfo () ;

         _optCLScanInfo ( const _optCLScanInfo & info ) ;

         virtual ~_optCLScanInfo () ;

         OSS_INLINE virtual void setIndexExtID ( dmsExtentID indexExtID )
         {
            _indexExtID = indexExtID ;
         }

         OSS_INLINE virtual void setIndexLID ( dmsExtentID indexLID )
         {
            _indexLID = indexLID ;
         }

         virtual void setCLFullName ( const CHAR *pCLFullName ) ;

         OSS_INLINE virtual dmsExtentID getIndexExtID () const
         {
            return _indexExtID ;
         }

         OSS_INLINE virtual dmsExtentID getIndexLID () const
         {
            return _indexLID ;
         }

         OSS_INLINE virtual const CHAR *getCLFullName () const
         {
            return _clFullName ;
         }

      protected :
         dmsExtentID _indexExtID ;
         dmsExtentID _indexLID ;
         CHAR        _clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
   } ;

   typedef class _optCLScanInfo optCLScanInfo ;

   /*
      _optAccessPlanRuntime define
    */
   class _optAccessPlanRuntime : public SDBObject,
                                 public _mthMatchRuntimeHolder
   {
      public :
         _optAccessPlanRuntime () ;

         virtual ~_optAccessPlanRuntime () ;

         void reset () ;

         void inheritRuntime ( _optAccessPlanRuntime *planRuntime ) ;

         INT32 createCLScanInfo () ;

         void deleteCLScanInfo () ;

         void setCLScanInfo ( optCLScanInfo * clScanInfo ) ;

         INT32 bindPlanInfo ( const CHAR *pCLFullName,
                              dmsStorageUnit *su,
                              dmsMBContext *mbContext,
                              dmsExtentID indexExtID,
                              dmsExtentID indexLID ) ;

         virtual const mthMatchRuntime * getMatchRuntime () const ;
         virtual mthMatchRuntime * getMatchRuntime () ;
         virtual mthMatchRuntime * getMatchRuntime ( BOOLEAN checkValid ) ;

         mthMatchTree * getMatchTree () ;
         const mthMatchTree * getMatchTree () const ;

         rtnPredicateList * getPredList () ;
         const rtnPredicateList * getPredList () const ;

         rtnParamList & getParameters () ;
         const rtnParamList & getParameters () const ;

         BSONObj getEqualityQueryObject () ;
         BSONObj getParsedMatcher () const ;
         BSONObj getPredIXBound () const ;

         OSS_INLINE void setPlan ( optAccessPlan *plan,
                                   optAccessPlanManager *apm,
                                   BOOLEAN isNewPlan )
         {
            _plan = plan ;
            _apm = apm ;
            _isNewPlan = isNewPlan ;
         }

         INT32 bindParamPlan ( optAccessPlanHelper &planHelper,
                               optAccessPlan *plan ) ;

         OSS_INLINE optAccessPlan *getPlan ()
         {
            return _plan ;
         }

         OSS_INLINE const optAccessPlan *getPlan () const
         {
            return _plan ;
         }

         OSS_INLINE BOOLEAN hasPlan () const
         {
            return NULL != _plan ;
         }

         void releasePlan () ;

         OSS_INLINE BOOLEAN isNewPlan () const
         {
            return _isNewPlan ;
         }

         OSS_INLINE const optCLScanInfo * getCLScanInfo () const
         {
            return _clScanInfo ;
         }

         OSS_INLINE optCLScanInfo * getCLScanInfo ()
         {
            return _clScanInfo ;
         }

         OSS_INLINE optScanType getScanType () const
         {
            return _plan ? _plan->getScanType() : UNKNOWNSCAN ;
         }

         OSS_INLINE BOOLEAN sortRequired () const
         {
            return _plan ? _plan->sortRequired() : FALSE ;
         }

         OSS_INLINE BOOLEAN isHintFailed () const
         {
            return _plan ? _plan->isHintFailed() : TRUE ;
         }

         OSS_INLINE BOOLEAN isAutoGen () const
         {
            return _plan ? _plan->isAutoGen() : FALSE ;
         }

         OSS_INLINE INT32 getDirection () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _plan ? _plan->getDirection() : 1 ;
         }

         OSS_INLINE INT64 getAccessPlanID() const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _plan ? _plan->getAccessPlanID() : -1 ;
         }

         OSS_INLINE const CHAR *getIndexName () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _plan ? _plan->getIndexName() : NULL ;
         }

         OSS_INLINE dmsExtentID getIndexCBExtent () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _clScanInfo ? _clScanInfo->getIndexExtID() :
                                 _plan->getIndexCBExtent() ;
         }

         OSS_INLINE dmsExtentID getIndexLID () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _clScanInfo ? _clScanInfo->getIndexLID() :
                                 _plan->getIndexLID() ;
         }

         OSS_INLINE const CHAR *getCLFullName () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _clScanInfo ? _clScanInfo->getCLFullName() :
                                 _plan->getCLFullName() ;
         }

         OSS_INLINE UINT16 getCLMBID () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _clScanInfo ? _clScanInfo->getCLMBID() :
                                 _plan->getCLMBID() ;
         }

         OSS_INLINE UINT32 getCLLID () const
         {
            SDB_ASSERT( _plan, "_plan is invalid" ) ;
            return _clScanInfo ? _clScanInfo->getCLLID() :
                                 _plan->getCLLID() ;
         }

         OSS_INLINE BOOLEAN canSetQueryActivity () const
         {
            return ( NULL != _plan && NULL != _apm && !_hasQueryActivity ) ?
                   TRUE : FALSE ;
         }

         void setQueryActivity ( MON_OPERATION_TYPES optrType,
                                 const monContextCB &monCtxCB,
                                 const rtnReturnOptions &returnOptions,
                                 BOOLEAN hitEnd ) ;

         INT32 toExplainPath ( optExplainScanPath &expPath,
                               const rtnContext *context ) const ;

      protected :
         // Pointer to access plan
         optAccessPlan *         _plan ;

         // Pointer to access plan manager
         optAccessPlanManager *  _apm ;

         // Whether query activity is set
         BOOLEAN                 _hasQueryActivity ;

         // Mark the plan is new created or got from cache
         BOOLEAN                 _isNewPlan ;

         // Used for main CL plan, bind sub-collection and index
         BOOLEAN                 _ownedPlanInfo ;
         optCLScanInfo *         _clScanInfo ;
   } ;

   typedef class _optAccessPlanRuntime optAccessPlanRuntime ;

}

#endif //OPTACCESSPLANRUNTIME_HPP__
