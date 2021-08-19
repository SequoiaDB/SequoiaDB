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

   Source File Name = monDump.hpp

   Descriptive Name = Monitor Dump Header

   When/how to use: this program may be used on binary and text-formatted
   versions of monitoring component. This file contains declare for
   functions that generate result dataset for given resources.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MONDUMP_HPP_
#define MONDUMP_HPP_

#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "rtnContextDump.hpp"
#include "rtnFetchBase.hpp"
#include "dpsTransCB.hpp"
#include "../bson/bson.h"
#include "rtnCommandDef.hpp"
#include "monClass.hpp"
#include "dmsScanner.hpp"

using namespace bson ;

namespace engine
{
   /*
      Base define
   */
   #define MON_MASK_NODE_NAME          0x00000001
   #define MON_MASK_HOSTNAME           0x00000002
   #define MON_MASK_SERVICE_NAME       0x00000004
   #define MON_MASK_GROUP_NAME         0x00000008
   #define MON_MASK_IS_PRIMARY         0x00000010
   #define MON_MASK_SERVICE_STATUS     0x00000020
   #define MON_MASK_LSN_INFO           0x00000040
   #define MON_MASK_NODEID             0x00000080
   #define MON_MASK_TRANSINFO          0x00000100
   #define MON_MASK_ALL                0xFFFFFFFF

   #define MON_MASK_FETCH_DEFAULT      ( MON_MASK_NODE_NAME |\
                                         MON_MASK_HOSTNAME |\
                                         MON_MASK_SERVICE_NAME |\
                                         MON_MASK_GROUP_NAME |\
                                         MON_MASK_NODEID )

   INT32 monAppendSystemInfo ( BSONObjBuilder &ob,
                               UINT32 mask = MON_MASK_ALL ) ;

   void  monAppendVersion ( BSONObjBuilder &ob ) ;

   void  monAppendUlimit ( BSONObjBuilder &ob ) ;

   INT32 monAppendFileDesp( BSONObjBuilder &ob ) ;

   INT32 monAppendHostMemory ( BSONObjBuilder &ob ) ;

   INT32 monAppendNodeMemory( BSONObjBuilder &ob ) ;

   INT32 monAppendDisk ( BSONObjBuilder &ob,
                         BOOLEAN appendDbPath = TRUE ) ;

   INT32 monDumpIndexes( MON_IDX_LIST &indexes, rtnContextDump *context ) ;

   INT32 monDumpTraceStatus ( rtnContextDump *context ) ;

   INT32 monDumpDatablocks( std::vector<dmsExtentID> &datablocks,
                            rtnContextDump *context ) ;

   INT32 monDumpIndexblocks( std::vector< BSONObj > &idxBlocks,
                             std::vector< dmsRecordID > &idxRIDs,
                             const CHAR *indexName,
                             dmsExtentID indexLID,
                             INT32 direction,
                             rtnContextDump *context ) ;

   INT32  monResetMon ( RTN_COMMAND_TYPE type,
                        BOOLEAN resetAllEDU,
                        EDUID eduID,
                        const CHAR * collectionSpace,
                        const CHAR * collection ) ;
   INT32 monDBDumpStorageInfo( BSONObjBuilder &ob );

   INT32 monDBDumpProcMemInfo( BSONObjBuilder &ob );

   INT32 monDBDumpNetInfo( BSONObjBuilder &ob );

   INT32 monDBDumpLogInfo( BSONObjBuilder &ob );

   INT32 monDumpLastOpInfo( BSONObjBuilder &ob, const monAppCB &moncb ) ;

   void  monDumpSvcTaskInfo( BSONObjBuilder &ob,
                             const monSvcTaskInfo *pInfo,
                             BOOLEAN exceptIDName = FALSE ) ;

   INT32 monParseArchiveOpt ( const BSONObj &obj , BOOLEAN &archiveOpt ) ;

   INT32 monDetailObj2Info( const BSONObj &obj, detailedInfo &info ) ;

   INT32 monDetailInfo2Obj( const detailedInfo &info,
                            INT32 sequence,
                            BSONObjBuilder &ob ) ;

   INT32 monCollection2Obj ( const monCollection &full, UINT32 addInfoMask,
                             BSONObjBuilder &ob ) ;

   INT32 monBuildStatResult( BSONObj &stat, UINT32 addInfoMask,
                             BSONObjBuilder &ob ) ;

   /*
      _monTransFetcher define
   */
   class _monTransFetcher : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monTransFetcher() ;
         virtual ~_monTransFetcher() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32    _fetchNextTransInfo() ;

      private:
         TRANS_EDU_LIST             _eduList ;
         monTransInfo               _curTransInfo ;
         VEC_TRANSLOCKCUR::iterator _pos ;
         BSONObj                    _curEduInfo ;
         BOOLEAN                    _dumpCurrent ;
         BOOLEAN                    _detail ;
         UINT32                     _addInfoMask ;
         UINT32                     _slice ;
   } ;
   typedef _monTransFetcher monTransFetcher ;

   /*
      _monContextFetcher define
   */
   class _monContextFetcher : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monContextFetcher() ;
         virtual ~_monContextFetcher() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNextSimple( BSONObj &obj ) ;
         INT32       _fetchNextDetail( BSONObj &obj ) ;

      private:
         BOOLEAN                    _dumpCurrent ;
         BOOLEAN                    _detail ;
         UINT32                     _addInfoMask ;

         std::map<UINT64, pmdEDUCB::SET_CONTEXT >  _contextList ;
         std::map<UINT64, std::set<monContextFull> >  _contextInfoList ;
   } ;
   typedef _monContextFetcher monContextFetcher ;

   /*
      _monSessionFetcher define
   */
   class _monSessionFetcher : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monSessionFetcher() ;
         virtual ~_monSessionFetcher() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNextSimple( BSONObj &obj ) ;
         INT32       _fetchNextDetail( BSONObj &obj ) ;

      private:
         BOOLEAN                    _dumpCurrent ;
         BOOLEAN                    _detail ;
         UINT32                     _addInfoMask ;

         std::set<monEDUSimple>     _setInfoSimple ;
         std::set<monEDUFull>       _setInfoDetail ;
   } ;
   typedef _monSessionFetcher monSessionFetcher ;

   /*
      _monCollectionFetch define
   */
   class _monCollectionFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monCollectionFetch() ;
         virtual ~_monCollectionFetch() ;
         /*
            Use isCurrent for isIncludeSystem
         */
         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNextSimple( BSONObj &obj ) ;
         INT32       _fetchNextDetail( BSONObj &obj ) ;

      private:
         BOOLEAN                    _detail ;
         BOOLEAN                    _includeSys ;
         UINT32                     _addInfoMask ;

         MON_CL_SIM_LIST            _collectionList ;
         MON_CL_LIST                _collectionInfo ;
   } ;
   typedef _monCollectionFetch monCollectionFetch ;

   /*
      _monCollectionSpaceFetch define
   */
   class _monCollectionSpaceFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monCollectionSpaceFetch() ;
         virtual ~_monCollectionSpaceFetch() ;
         /*
            Use isCurrent for isIncludeSystem
         */
         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNextSimple( BSONObj &obj ) ;
         INT32       _fetchNextDetail( BSONObj &obj ) ;

      private:
         BOOLEAN                    _detail ;
         BOOLEAN                    _includeSys ;
         UINT32                     _addInfoMask ;

         MON_CS_SIM_LIST            _csList ;
         MON_CS_LIST                _csInfo ;
   } ;
   typedef _monCollectionSpaceFetch monCollectionSpaceFetch ;

   /*
      _monDataBaseFetch define
   */
   class _monDataBaseFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monDataBaseFetch() ;
         virtual ~_monDataBaseFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;

   } ;
   typedef _monDataBaseFetch monDataBaseFetch ;

   /*
      _monSystemFetch define
   */
   class _monSystemFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monSystemFetch() ;
         virtual ~_monSystemFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;

   } ;
   typedef _monSystemFetch monSystemFetch ;

   /*
      _monHealthFetch define
   */
   class _monHealthFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monHealthFetch() ;
         virtual ~_monHealthFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;

   } ;
   typedef _monHealthFetch monHealthFetch ;

   /*
      _monStorageUnitFetch define
   */
   class _monStorageUnitFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monStorageUnitFetch() ;
         virtual ~_monStorageUnitFetch() ;

         /*
            use isCurrent for isIncludeSystem
         */
         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNext( BSONObj &obj ) ;

      private:
         BOOLEAN                 _includeSys ;
         UINT32                  _addInfoMask ;

         MON_SU_LIST             _suInfo ;
   } ;
   typedef _monStorageUnitFetch monStorageUnitFetch ;

   /*
      _monIndexFetch define
   */
   class _monIndexFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monIndexFetch() ;
         virtual ~_monIndexFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNext( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;

         UINT32                  _pos ;
         MON_IDX_LIST            _indexInfo ;
   } ;
   typedef _monIndexFetch monIndexFetch ;

   /*
      _monCLBlockFetch define
   */
   class _monCLBlockFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monCLBlockFetch() ;
         virtual ~_monCLBlockFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNext( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;

         UINT32                  _pos ;
         vector<dmsExtentID>     _vecBlock ;
   } ;
   typedef _monCLBlockFetch monCLBlockFetch ;

   /*
      _monBackupFetch define
   */
   class _monBackupFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monBackupFetch() ;
         virtual ~_monBackupFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      protected:
         INT32       _fetchNext( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;

         UINT32                  _pos ;
         vector < BSONObj >      _vecBackup ;
   } ;
   typedef _monBackupFetch monBackupFetch ;

   /*
      _monAccessPlansFetch define
    */
   class _monAccessPlansFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monAccessPlansFetch () ;
         virtual ~_monAccessPlansFetch () ;

         virtual INT32 init ( pmdEDUCB *cb,
                              BOOLEAN isCurrent,
                              BOOLEAN isDetail,
                              UINT32 addInfoMask,
                              const BSONObj obj = BSONObj() ) ;

         virtual const CHAR* getName () const ;

      public :
         virtual INT32 fetch ( BSONObj &obj ) ;

      protected :
         INT32 _fetchNext ( BSONObj &obj ) ;

      private :
         UINT32               _addInfoMask ;
         BSONObj              _sysInfo ;
         UINT32               _pos ;
         vector<BSONObj>      _cachedPlanList ;
   } ;

   typedef _monAccessPlansFetch monAccessPlansFetch ;

   /*
      _monHealthFetch define
   */
   class _monConfigsFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monConfigsFetch() ;
         virtual ~_monConfigsFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;
         BOOLEAN                 _isLocalMode ;
         BOOLEAN                 _isExpand ;

   } ;
   typedef _monConfigsFetch monConfigsFetch ;

   /*
      _monVCLSessionInfoFetch define
   */
   class _monVCLSessionInfoFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monVCLSessionInfoFetch() ;
         virtual ~_monVCLSessionInfoFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         BSONObj                 _info ;
   } ;
   typedef _monVCLSessionInfoFetch monVCLSessionInfoFetch ;

   /*
      _monSvcTasksFetch define
   */
   class _monSvcTasksFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monSvcTasksFetch() ;
         virtual ~_monSvcTasksFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;
         BOOLEAN                 _isDetail ;

         MAP_SVCTASKINFO_PTR     _mapSvcTask ;
   } ;
   typedef _monSvcTasksFetch monSvcTasksFetch ;

   /*
      _monQueriesFetch define
   */
   class _monQueriesFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monQueriesFetch() ;
         virtual ~_monQueriesFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;
         BOOLEAN                 _viewArchive ;
         monClassReadScanner    *_scanner ;
         monClassQuery          *_queryCB ;
         BOOLEAN                 _isDetail ;
   } ;
   typedef _monQueriesFetch monQueriesFetch ;

   /*
      _monLatchWaitsFetch define
   */
   class _monLatchWaitsFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monLatchWaitsFetch() ;
         virtual ~_monLatchWaitsFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         UINT32                  _addInfoMask ;
         BOOLEAN                 _viewArchive ;
         monClassReadScanner    *_scanner ;
         monClassLatch          *_latchCB ;
         BOOLEAN                 _isDetail ;
   } ;
   typedef _monLatchWaitsFetch monLatchWaitsFetch ;

   /*
      _monLockWaitsFetch define
   */
   class _monLockWaitsFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monLockWaitsFetch() ;
         virtual ~_monLockWaitsFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         BOOLEAN                 _viewArchive ;
         UINT32                  _addInfoMask ;
         monClassReadScanner    *_scanner ;
         monClassLock           *_lockCB ;
         BOOLEAN                 _isDetail ;
   } ;
   typedef _monLockWaitsFetch monLockWaitsFetch ;

   /*
      _monIndexStatsFetch define
   */
   class _monIndexStatsFetch : public rtnFetchBase
   {
      DECLARE_FETCH_AUTO_REGISTER()

      public:
         _monIndexStatsFetch() ;
         virtual ~_monIndexStatsFetch() ;

         virtual INT32        init( pmdEDUCB *cb,
                                    BOOLEAN isCurrent,
                                    BOOLEAN isDetail,
                                    UINT32 addInfoMask,
                                    const BSONObj obj = BSONObj() ) ;

         virtual const CHAR*  getName() const ;

      public:
         virtual INT32     fetch( BSONObj &obj ) ;

      private:
         typedef  ossPoolVector<CHAR *> IDX_STAT_LIST ;

         INT32    _bulkFetch( IDX_STAT_LIST &result ) ;

         void     _buildResult( BSONObj &stat, BSONObjBuilder &ob ) ;

      private:
         UINT32                  _addInfoMask ;
         BOOLEAN                 _isDetail ;
         dmsStorageUnit         *_su ;
         dmsStorageUnitID        _suID ;
         dmsMBContext           *_mbContext ;
         dmsExtentID             _curExtentID ;
         BOOLEAN                 _noMoreStat ;
         IDX_STAT_LIST::iterator _pos ;
         IDX_STAT_LIST           _statCache ;
   } ;
   typedef _monIndexStatsFetch monIndexStatsFetch ;
}

#endif //MONDUMP_HPP_

