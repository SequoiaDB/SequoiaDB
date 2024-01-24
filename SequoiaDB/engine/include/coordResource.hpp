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

   Source File Name = coordResource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_RESOURCE_HPP__
#define COORD_RESOURCE_HPP__

#include "coordDef.hpp"
#include "pmdOptionsMgr.hpp"
#include "coordOmCache.hpp"
#include "IDataSource.hpp"
#include "ossMemPool.hpp"
#include "../bson/bson.h"
#include "pmdEnv.hpp"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _netRouteAgent ;
   class _IOmProxy ;
   class _coordSequenceAgent ;
   class _coordDataSourceMgr ;

   /*
      _coordResource define
   */
   class _coordResource : public SDBObject
   {

      struct cmp_str
      {
         bool operator() ( const char *a, const char *b )
         {
            return ossStrcmp( a, b ) < 0 ;
         }
      } ;

      typedef ossPoolMap< UINT32, CoordGroupInfoPtr > MAP_GROUP_INFO ;
      typedef MAP_GROUP_INFO::iterator                MAP_GROUP_INFO_IT ;

      typedef ossPoolMap<std::string, UINT32>         MAP_GROUP_NAME ;
      typedef MAP_GROUP_NAME::iterator                MAP_GROUP_NAME_IT ;

      typedef ossPoolMap<const CHAR*, CoordCataInfoPtr, cmp_str>  MAP_CATA_INFO ;
#if defined (_WINDOWS)
      typedef MAP_CATA_INFO::iterator                 MAP_CATA_INFO_IT ;
      typedef MAP_CATA_INFO::const_iterator           MAP_CATA_INFO_CIT ;
#else
      typedef ossPoolMap<const CHAR*, CoordCataInfoPtr>::iterator       MAP_CATA_INFO_IT ;
      typedef ossPoolMap<const CHAR*, CoordCataInfoPtr>::const_iterator MAP_CATA_INFO_CIT ;
#endif // _WINDOWS

      public:
         friend class _coordCacheCleaner ;

         _coordResource() ;
         ~_coordResource() ;

         INT32       init( _netRouteAgent *pAgent,
                           pmdOptionsCB *pOptionsCB,
                           _coordDataSourceMgr *pDSMgr = NULL ) ;
         void        fini() ;

         void        invalidateCataInfo( const CHAR *clFullName = NULL ) ;
         void        invalidateGroupInfo( UINT64 identify = 0 ) ;
         void        invalidateStrategy() ;
         void        invalidateDataSourceInfo( const CHAR *name = NULL ) ;

         _netRouteAgent*   getRouteAgent() ;
         _IOmProxy*        getOmProxy() ;
         _coordOmStrategyAgent* getOmStrategyAgent() ;
         OSS_INLINE _coordSequenceAgent* getSequenceAgent()
         {
            return _pSequenceAgent ;
         }
         _coordDataSourceMgr* getDSManager() { return _pDataSourceMgr ; }

   public:

         INT32       getGroupInfo( UINT32 groupID,
                                   CoordGroupInfoPtr &groupPtr ) ;
         INT32       getGroupInfo( const CHAR *groupName,
                                   CoordGroupInfoPtr &groupPtr ) ;

         UINT32      getGroupsInfo( GROUP_VEC &vecGroupPtr,
                                    BOOLEAN exceptCata,
                                    BOOLEAN exceptCoord ) ;

         UINT32      getGroupList( CoordGroupList &groupList,
                                   BOOLEAN exceptCata,
                                   BOOLEAN exceptCoord,
                                   BOOLEAN exceptEmptyGroup ) ;

         INT32       updateGroupInfo( UINT32 groupID,
                                      CoordGroupInfoPtr &groupPtr,
                                      _pmdEDUCB *cb ) ;
         INT32       updateGroupInfo( const CHAR *groupName,
                                      CoordGroupInfoPtr &groupPtr,
                                      _pmdEDUCB *cb ) ;

         INT32       getOrUpdateGroupInfo( const CHAR *groupName,
                                           CoordGroupInfoPtr &groupPtr,
                                           _pmdEDUCB *cb ) ;

         INT32       getOrUpdateGroupInfo( UINT32 groupID,
                                           CoordGroupInfoPtr &groupPtr,
                                           _pmdEDUCB *cb ) ;

         INT32       updateGroupsInfo( GROUP_VEC &vecGroupPtr,
                                       _pmdEDUCB *cb,
                                       const BSONObj *pCondObj = NULL,
                                       BOOLEAN exceptCata = FALSE,
                                       BOOLEAN exceptCoord = FALSE,
                                       BOOLEAN exceptEmptyGroup = FALSE ) ;

         INT32       updateGroupList( CoordGroupList &groupList,
                                      _pmdEDUCB *cb,
                                      const BSONObj *pCondObj = NULL,
                                      BOOLEAN exceptCata = FALSE,
                                      BOOLEAN exceptCoord = FALSE,
                                      BOOLEAN useLocalWhenFailed = TRUE,
                                      BOOLEAN exceptEmptyGroup = FALSE ) ;

         void        removeGroupInfo( UINT32 groupID ) ;
         void        removeGroupInfo( const CHAR *groupName ) ;

         CoordGroupInfoPtr    getCataGroupInfo() ;
         INT32                updateCataGroupInfo( CoordGroupInfoPtr &groupPtr,
                                                   _pmdEDUCB *cb ) ;

         INT32       groupID2Name ( UINT32 id, std::string &name ) ;
         INT32       groupName2ID ( const CHAR* name, UINT32 &id ) ;

         void        getCataNodeAddrList( CoordVecNodeInfo &vecCata ) ;
         INT32       syncAddress2Options( BOOLEAN flush = TRUE,
                                          BOOLEAN force = FALSE ) ;

         void        clearCataNodeAddrList() ;
         BOOLEAN     addCataNodeAddrWhenEmpty( const CHAR *pHostName,
                                               const CHAR *pSvcName ) ;

         CoordGroupInfoPtr    getOmGroupInfo() ;
         INT32                updateOmGroupInfo( CoordGroupInfoPtr &groupPtr,
                                                 _pmdEDUCB *cb ) ;

         UINT64      getTotalCataInfoSize() const ;

         INT32       active() ;

   public:
         void        addCataInfo( CoordCataInfoPtr &cataPtr ) ;

         INT32       getCataInfo( const CHAR *collectionName,
                                  CoordCataInfoPtr &cataPtr ) ;

         void        removeCataInfo( const CHAR *collectionName ) ;
         void        removeCataInfoWithMain( const CHAR *collectionName ) ;

         void        removeCataInfoByCS( const CHAR *csName,
                                         vector< string > *pRelatedCLs = NULL ) ;

         INT32       updateCataInfo( const CHAR *collectionName,
                                     CoordCataInfoPtr &cataPtr,
                                     _pmdEDUCB *cb ) ;

         INT32       updateCataInfoByCLUID( utilCLUniqueID clUID,
                                            CoordCataInfoPtr &cataPtr,
                                            _pmdEDUCB *cb ) ; ;

         INT32       getOrUpdateCataInfo( const CHAR *collectionName,
                                          CoordCataInfoPtr &cataPtr,
                                          _pmdEDUCB *cb ) ;

         void        updateNodeStat( const MsgRouteID &nodeID, INT32 rc ) ;

      protected:
         void        setCataGroupInfo( CoordGroupInfoPtr &groupPtr,
                                       BOOLEAN inheritStat = FALSE ) ;
         void        setOmGroupInfo( CoordGroupInfoPtr &groupPtr ) ;
         void        addGroupInfo( CoordGroupInfoPtr &groupPtr,
                                   BOOLEAN inheritStat = FALSE ) ;

         UINT32      checkAndRemoveCataInfoBySub( const CHAR *collectionName ) ;

      protected:

         void        _clearGroupName( UINT32 groupID ) ;
         void        _addGroupName( const std::string &name, UINT32 id ) ;

         INT32       _updateCataGroupInfoByAddr( _pmdEDUCB *cb,
                                                 CoordGroupInfoPtr &groupPtr ) ;
         INT32       _updateCataGroupInfo( _pmdEDUCB *cb,
                                           const CoordGroupInfoPtr &cataGroupPtr,
                                           CoordGroupInfoPtr &groupPtr ) ;

         INT32       _processGroupReply( MsgHeader *pMsg,
                                         CoordGroupInfoPtr &groupPtr ) ;

         INT32       _processGroupContextReply( INT64 &contextID,
                                                GROUP_VEC &vecGroupPtr,
                                                _pmdEDUCB *cb ) ;

         INT32       _updateRouteInfo( const CoordGroupInfoPtr &groupPtr,
                                       MSG_ROUTE_SERVICE_TYPE type ) ;

         INT32       _updateGroupInfo( MsgHeader *pMsg,
                                       _pmdEDUCB *cb,
                                       MSG_ROUTE_SERVICE_TYPE type,
                                       CoordGroupInfoPtr &groupPtr ) ;

         void        _addAddrNode( const MsgRouteID &id,
                                   const CHAR *pHostName,
                                   const CHAR *pSvcName,
                                   INT32 serviceType,
                                   CoordVecNodeInfo &vecAddr ) ;

         void        _initAddressFromPair( const vector< pmdAddrPair > &vecAddrPair,
                                           INT32 serviceType,
                                           CoordVecNodeInfo &vecAddr ) ;

         INT32       _updateCataInfo( const BSONObj &obj,
                                      const CHAR *collectionName,
                                      CoordCataInfoPtr &cataPtr,
                                      _pmdEDUCB *cb ) ;

         INT32       _processCatalogReply( MsgHeader *pMsg,
                                           const CHAR *collectionName,
                                           CoordCataInfoPtr &cataPtr ) ;

         BSONObj     _buildOmGroupInfo() ;

         INT32       _updateCataInfoByCLUID( utilCLUniqueID clUID,
                                             CoordCataInfoPtr &cataPtr,
                                             _pmdEDUCB *cb ) ;

         INT32       _processCatalogReplyByCLUID( MsgHeader *pMsg,
                                                  CoordCataInfoPtr &cataPtr ) ;

         void        _removeCataInfo( const CHAR *collectionName ) ;

         void        _removeCataInfo( MAP_CATA_INFO_IT it ) ;

         void        _removeAllCataInfo() ;

         BOOLEAN     _canCleanCataInfo() ;

         INT32       _doCleanCataInfo() ;

      private:
         MAP_GROUP_INFO                   _mapGroupInfo ;
         MAP_GROUP_NAME                   _mapGroupName ;
         ossSpinSLatch                    _nodeMutex ;

         CoordGroupInfoPtr                _cataGroupInfo ;
         CoordGroupInfoPtr                _omGroupInfo ;

         UINT64                           _upGrpIndentify ;
         CoordVecNodeInfo                 _cataNodeAddrList ;
         BOOLEAN                          _cataAddrChanged ;

         CoordVecNodeInfo                 _omNodeAddrList ;

         MAP_CATA_INFO                    _mapCataInfo ;
         ossSpinSLatch                    _cataMutex ;
         UINT64                           _totalCataInfoSize ;

         _netRouteAgent                   *_pAgent ;
         pmdOptionsCB                     *_pOptionsCB ;
         CoordGroupInfoPtr                _emptyGroupPtr ;

         _IOmProxy                        *_pOmProxy ;
         _coordOmStrategyAgent            *_pOmStrategyAgent ;

         _coordSequenceAgent              *_pSequenceAgent ;

         _coordDataSourceMgr              *_pDataSourceMgr ;
   } ;
   typedef _coordResource coordResource ;

}

#endif // COORD_RESOURCE_HPP__

