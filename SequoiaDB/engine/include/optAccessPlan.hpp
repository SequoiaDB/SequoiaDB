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

   Source File Name = optAccessPlan.hpp

   Descriptive Name = Optimizer Access Plan Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Optimizer component. This file contains structure for access
   plan, which is indicating how to run a given query.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OPTACCESSPLAN_HPP__
#define OPTACCESSPLAN_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "rtnPredicate.hpp"
#include "ossAtomic.hpp"
#include "../bson/oid.h"
#include "../bson/bson.h"
#include "ossUtil.hpp"
#include "optPlanPath.hpp"
#include "utilHashTable.hpp"
#include "optAccessPlanKey.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   class _optAccessPlanManager ;

   #define OPT_INVALID_ACT_ID          ( -1 )
   #define OPT_PARAM_VALID_PLAN_NUM    ( 5 )
   #define OPT_MAINCL_VALID_PLAN_NUM   ( 5 )

   enum OPT_PLAN_TYPE
   {
      OPT_PLAN_TYPE_NORMAL,
      OPT_PLAN_TYPE_PARAMETERIZED,
      OPT_PLAN_TYPE_MAINCL
   } ;

   /*
      _optAccessPlan define
    */
   // note _optAccessPlan object should NEVER been deleted by caller other than
   // _optAccessPlanManager, user should call getAccessPlan and releasePlan to
   // make sure all plans are properly tracked
   class _optAccessPlan : public _utilHashTableItem,
                          public _mthMatchTreeStackHolder,
                          public _mthMatchRuntimeHolder
   {
      public :
         _optAccessPlan ( optAccessPlanKey &planKey,
                          const mthNodeConfig &config ) ;

         virtual ~_optAccessPlan () ;

         OSS_INLINE BOOLEAN isInitialized () const
         {
            return _isInitialized ;
         }

         OSS_INLINE BOOLEAN isInvalid() const
         {
            return _isInvalid ;
         }

         void markInvalid() ;

         virtual OPT_PLAN_TYPE getPlanType () const = 0 ;

         /// Information for plan execution and explanation
         OSS_INLINE BOOLEAN isHintFailed () const
         {
            return _hintFailed ;
         }

         OSS_INLINE BOOLEAN isAutoGen () const
         {
            return _isAutoPlan ;
         }

         // Information of collection space
         OSS_INLINE dmsStorageUnitID getSUID () const
         {
            return _key.getSUID() ;
         }

         OSS_INLINE UINT32 getSULID () const
         {
            return _key.getSULID() ;
         }

         // Information of collection
         OSS_INLINE const CHAR *getCLFullName () const
         {
            return _key.getCLFullName() ;
         }

         OSS_INLINE UINT16 getCLMBID () const
         {
            return _key.getCLMBID() ;
         }

         OSS_INLINE UINT32 getCLLID () const
         {
            return _key.getCLLID() ;
         }

         OSS_INLINE INT64 getAccessPlanID () const
         {
            return _accessPlanID ;
         }

         OSS_INLINE void setAccessPlanID (INT64 accessPlanID )
         {
            _accessPlanID = accessPlanID ;
         }

         string toString () const ;

         virtual INT32 toBSON ( BSONObjBuilder &builder ) const ;

         /// Plan cache related
         OSS_INLINE const optAccessPlanKey &getKey () const
         {
            return _key ;
         }

         OSS_INLINE optAccessPlanKey &getKey ()
         {
            return _key ;
         }

         OSS_INLINE INT32 getKeyOwned ()
         {
            return _key.getOwned() ;
         }

         OSS_INLINE BOOLEAN isEqual ( const optAccessPlanKey &key ) const
         {
            return _key.isEqual( key ) ;
         }

         OSS_INLINE BOOLEAN isEqual ( const _optAccessPlan &item ) const
         {
            return _key.isEqual( item._key ) ;
         }

         OSS_INLINE UINT32 getKeyCode () const
         {
            return _key.getKeyCode () ;
         }

         OSS_INLINE void setActivityID ( INT32 activityID )
         {
            // No need to check
            _activityID.init( activityID ) ;
         }

         OSS_INLINE INT32 resetActivityID ()
         {
            // Already reset by others
            return _activityID.swap( OPT_INVALID_ACT_ID ) ;
         }

         OSS_INLINE INT32 getActivityID () const
         {
            return _activityID.peek() ;
         }

         OSS_INLINE UINT32 getRefCount () const
         {
            return _refCount.peek() ;
         }

         OSS_INLINE UINT32 incRefCount ()
         {
            return _refCount.inc() ;
         }

         OSS_INLINE UINT32 decRefCount ()
         {
            return _refCount.dec() ;
         }

         OSS_INLINE BOOLEAN isCached () const
         {
            return ( NULL != _pList ) ;
         }

         virtual void setCachedBitmap () = 0 ;

         void release () ;

         OSS_INLINE OPT_PLAN_CACHE_LEVEL getCacheLevel () const
         {
            return _key.getCacheLevel() ;
         }

         OSS_INLINE optScanType getScanType () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getScanType() ;
         }

         OSS_INLINE const CHAR * getIndexName () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getIndexName() ;
         }

         OSS_INLINE virtual dmsExtentID getIndexCBExtent () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getIndexExtID() ;
         }

         OSS_INLINE virtual dmsExtentID getIndexLID () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getIndexLID() ;
         }

         OSS_INLINE INT32 getDirection () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getDirection() ;
         }

         OSS_INLINE BSONObj getKeyPattern () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getKeyPattern() ;
         }

         OSS_INLINE BOOLEAN sortRequired () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.isSortRequired() ;
         }

         OSS_INLINE virtual double getScore () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getSelectivity() ;
         }

         OSS_INLINE UINT32 getInputPages () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getInputPages() ;
         }

         OSS_INLINE UINT64 getInputRecords () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.getInputRecords() ;
         }

         OSS_INLINE BOOLEAN isEstimatedFromStat () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath.isEstimatedFromStat() ;
         }

         OSS_INLINE const optScanPath &getScanPath () const
         {
            SDB_ASSERT ( _isInitialized, "optAccessPlan must be optimized "
                         "before start using" ) ;
            return _scanPath ;
         }

         OSS_INLINE virtual const optScanPathList * getSearchPaths () const
         {
            return NULL ;
         }

         virtual INT32 bindMatchRuntime ( mthMatchRuntime *matchRuntime ) = 0 ;

         // Virtual functions for parameterized plans
         OSS_INLINE virtual BOOLEAN isParamValid () const
         {
            return FALSE ;
         }

         OSS_INLINE virtual BOOLEAN isMainCLValid () const
         {
            return FALSE ;
         }

      protected :
         OSS_INLINE virtual INT32 _toBSONInternal ( BSONObjBuilder &builder ) const
         {
            return SDB_OK ;
         }

         virtual INT32 _prepareMatchTree ( optAccessPlanHelper &planHelper ) ;

      protected :
         optAccessPlanKey  _key ;

         INT64   _accessPlanID ;
         BOOLEAN _isInitialized ;
         BOOLEAN _isInvalid ;

         // Hint related
         BOOLEAN _hintFailed ;
         // auto plan, TRUE when the plan is not generated by hint or
         // hint is not valid
         BOOLEAN _isAutoPlan ;

         /// Used in plan cache
         // Activity ID to index the activity statistics
         ossAtomicSigned32 _activityID ;
         // Reference count to the plan
         // Note, the plan could be only deleted when reference count is zero
         ossAtomic32 _refCount ;

         /// Scan path
         optPlanAllocator _planAllocator ;
         optScanPath _scanPath ;
   } ;

   typedef class _optAccessPlan optAccessPlan ;

   /*
      _optGeneralAccessPlan define
    */
   class _optGeneralAccessPlan : public _optAccessPlan
   {
      public :
         _optGeneralAccessPlan ( optAccessPlanKey &planKey,
                                 const mthNodeConfig &config ) ;

         virtual ~_optGeneralAccessPlan () ;

         INT32 optimize ( dmsStorageUnit *su,
                          dmsMBContext *mbContext,
                          optAccessPlanHelper &planHelper ) ;

         OSS_INLINE virtual OPT_PLAN_TYPE getPlanType () const
         {
            return OPT_PLAN_TYPE_NORMAL ;
         }

         OSS_INLINE virtual void setCachedBitmap ()
         {
            // Set the cached plan bitmap for collection space
            if ( NULL != _cachedPlanMgr )
            {
               _cachedPlanMgr->setCacheBitmapForPlan( _key.getKeyCode() ) ;
            }
         }

         OSS_INLINE virtual BOOLEAN validateParameterized ( const _optAccessPlan &plan,
                                                            const BSONObj &parameters )
         {
            return FALSE ;
         }

         OSS_INLINE virtual INT32 markParamInvalid ( dmsMBContext *mbContext )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual const optScanPathList * getSearchPaths () const
         {
            return _searchPaths ;
         }

         virtual INT32 bindMatchRuntime ( mthMatchRuntime *matchRuntime ) ;

      protected :
         INT32 _checkOrderBy () ;

         INT32 _estimateHintPlans ( dmsStorageUnit *su,
                                    dmsMBContext *mbContext,
                                    optAccessPlanHelper &planHelper,
                                    dmsStatCache *statCache ) ;

         INT32 _estimatePlans ( dmsStorageUnit *su,
                                dmsMBContext *mbContext,
                                optAccessPlanHelper &planHelper,
                                dmsStatCache *statCache ) ;

         INT32 _estimateIxScanPlan ( dmsStorageUnit *su,
                                     dmsMBContext *mbContext,
                                     optCollectionStat *collectionStat,
                                     optAccessPlanHelper &planHelper,
                                     const CHAR *pIndexName,
                                     OPT_PLAN_PATH_PRIORITY priority,
                                     optScanPath &ixScanPath ) ;

         INT32 _estimateIxScanPlan ( dmsStorageUnit *su,
                                     dmsMBContext *mbContext,
                                     optCollectionStat *collectionStat,
                                     optAccessPlanHelper &planHelper,
                                     const OID &indexOID,
                                     OPT_PLAN_PATH_PRIORITY priority,
                                     optScanPath &ixScanPath ) ;

         INT32 _estimateIxScanPlan ( dmsStorageUnit *su,
                                     optCollectionStat *collectionStat,
                                     optAccessPlanHelper &planHelper,
                                     dmsExtentID indexCBExtent,
                                     OPT_PLAN_PATH_PRIORITY priority,
                                     optScanPath &ixScanPath ) ;

         INT32 _estimateTbScanPlan ( optCollectionStat *collectionStat,
                                     optAccessPlanHelper &planHelper,
                                     optScanPath &tbScanPath ) ;

         INT32 _usePath ( dmsStorageUnit *su,
                          optAccessPlanHelper &planHelper,
                          optScanPath &path ) ;

         INT32 _prepareSUCaches ( dmsStorageUnit *su,
                                  dmsMBContext *mbContext ) ;

         INT32 _createSearchPaths () ;

         void _deleteSearchPaths () ;

         INT32 _addSearchPath ( optScanPath & path,
                                const optAccessPlanHelper & planHelper ) ;

      protected :
         dmsCachedPlanMgr *_cachedPlanMgr ;
         BOOLEAN _autoHint ;
         optScanPathList * _searchPaths ;
   } ;

   typedef class _optGeneralAccessPlan optGeneralAccessPlan ;

   /*
      _optParamAccessPlan define
    */
   class _optParamAccessPlan : public _optGeneralAccessPlan,
                               public _mthParamPredListStackHolder
   {
      protected :
         typedef struct _optParamRecord
         {
            _optParamRecord ()
            : _score( OPT_PRED_DEFAULT_SELECTIVITY )
            {
            }

            BSONObj _parameters ;
            double _score ;
         } _optParamRecord, optParamRecord ;

      public :
         _optParamAccessPlan ( optAccessPlanKey &planKey,
                               const mthNodeConfig &config ) ;

         virtual ~_optParamAccessPlan () ;

         OSS_INLINE virtual OPT_PLAN_TYPE getPlanType () const
         {
            return OPT_PLAN_TYPE_PARAMETERIZED ;
         }

         virtual OSS_INLINE double getScore () const
         {
            return _score ;
         }

         virtual OSS_INLINE BOOLEAN isParamValid () const
         {
            return _isParamValid ;
         }

         virtual BOOLEAN validateParameterized ( const _optAccessPlan &plan,
                                                 const BSONObj &parameters ) ;

         BOOLEAN checkSavedParam ( const BSONObj &parameters ) ;

         virtual INT32 markParamInvalid ( dmsMBContext *mbContext ) ;

         virtual INT32 bindMatchRuntime ( mthMatchRuntime *matchRuntime ) ;

      protected :
         void _saveParam ( const BSONObj &parameters, double score ) ;

         virtual INT32 _toBSONInternal ( BSONObjBuilder &builder ) const ;

      protected :
         optParamRecord    _records[ OPT_PARAM_VALID_PLAN_NUM ] ;
         BOOLEAN           _isParamValid ;
         ossAtomic32       _paramValidCount ;
         double            _score ;
   } ;

   typedef class _optParamAccessPlan optParamAccessPlan ;

   /*
      _optMainCLAccessPlan define
    */
   class _optMainCLAccessPlan : public _optAccessPlan,
                                public _mthParamPredListStackHolder
   {
      protected:
         typedef struct _optSubCLRecord
         {
            _optSubCLRecord ()
            : _score( OPT_PRED_DEFAULT_SELECTIVITY )
            {
               _subCLName[0] = '\0' ;
            }

            CHAR     _subCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
            BSONObj  _parameters ;
            double   _score ;
         } _optSubCLRecord, optSubCLRecord ;

      public :
         _optMainCLAccessPlan ( optAccessPlanKey &planKey,
                                const mthNodeConfig &config ) ;

         virtual ~_optMainCLAccessPlan () ;

         OSS_INLINE virtual OPT_PLAN_TYPE getPlanType () const
         {
            return OPT_PLAN_TYPE_MAINCL ;
         }

         OSS_INLINE virtual dmsExtentID getIndexCBExtent () const
         {
            return DMS_INVALID_EXTENT ;
         }

         OSS_INLINE virtual dmsExtentID getIndexLID () const
         {
            return DMS_INVALID_EXTENT ;
         }

         OSS_INLINE virtual double getScore () const
         {
            return _score ;
         }

         OSS_INLINE virtual void setCachedBitmap () {}

         OSS_INLINE virtual BOOLEAN isMainCLValid () const
         {
            return _isMainCLValid ;
         }

         INT32 prepareBindSubCL ( optAccessPlanHelper &planHelper ) ;

         INT32 bindSubCLAccessPlan ( optAccessPlanHelper &planHelper,
                                     optGeneralAccessPlan *subPlan,
                                     const BSONObj &parameters ) ;

         BOOLEAN validateSubCLPlan ( const optGeneralAccessPlan *plan,
                                     const BSONObj &parameters ) ;

         INT32 validateSubCL ( dmsStorageUnit *su,
                               dmsMBContext *mbContext,
                               dmsExtentID &indexExtID,
                               dmsExtentID &indexLID ) ;

         BOOLEAN checkSavedSubCL ( const CHAR * subCLName,
                                   const BSONObj & parameters ) ;

         INT32 markMainCLInvalid ( dmsCachedPlanMgr *pCachedPlanMgr,
                                   dmsMBContext *mbContext,
                                   BOOLEAN markInvalid ) ;

         virtual INT32 bindMatchRuntime ( mthMatchRuntime *matchRuntime ) ;

         INT32 bindMatchRuntime ( optAccessPlanHelper & planHelper,
                                  optGeneralAccessPlan * subPlan ) ;

      protected :
         void _saveSubCL ( const CHAR *pSubCLName, double score,
                           const BSONObj &parameters ) ;

         virtual INT32 _toBSONInternal ( BSONObjBuilder &builder ) const ;

      protected :
         optSubCLRecord    _records[ OPT_MAINCL_VALID_PLAN_NUM ] ;
         BOOLEAN           _isMainCLValid ;
         ossAtomic32       _mainCLValidCount ;
         double            _score ;
   } ;

   typedef class _optMainCLAccessPlan optMainCLAccessPlan ;

}

#endif //OPTACCESSPLAN_HPP__
