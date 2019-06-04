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
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _netRouteAgent ;
   class _IOmProxy ;

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

      typedef std::map< UINT32, CoordGroupInfoPtr >   MAP_GROUP_INFO ;
      typedef MAP_GROUP_INFO::iterator                MAP_GROUP_INFO_IT ;

      typedef std::map<std::string, UINT32>           MAP_GROUP_NAME ;
      typedef MAP_GROUP_NAME::iterator                MAP_GROUP_NAME_IT ;

      typedef std::map<const CHAR*, CoordCataInfoPtr, cmp_str> MAP_CATA_INFO ;
#if defined (_WINDOWS)
      typedef MAP_CATA_INFO::iterator                 MAP_CATA_INFO_IT ;
      typedef MAP_CATA_INFO::const_iterator           MAP_CATA_INFO_CIT ;
#else
      typedef std::map<const CHAR*, CoordCataInfoPtr>::iterator         MAP_CATA_INFO_IT ;
      typedef std::map<const CHAR*, CoordCataInfoPtr>::const_iterator   MAP_CATA_INFO_CIT ;
#endif // _WINDOWS

      public:
         _coordResource() ;
         ~_coordResource() ;

         INT32       init( _netRouteAgent *pAgent,
                           pmdOptionsCB *pOptionsCB ) ;
         void        fini() ;

         void        invalidateCataInfo() ;
         void        invalidateGroupInfo( UINT64 identify = 0 ) ;

         _netRouteAgent*   getRouteAgent() ;
         _IOmProxy*        getOmProxy() ;
         _coordOmStrategyAgent* getOmStrategyAgent() ;

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
                                   BOOLEAN exceptCoord ) ;

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
                                       BOOLEAN exceptCoord = FALSE ) ;

         INT32       updateGroupList( CoordGroupList &groupList,
                                      _pmdEDUCB *cb,
                                      const BSONObj *pCondObj = NULL,
                                      BOOLEAN exceptCata = FALSE,
                                      BOOLEAN exceptCoord = FALSE,
                                      BOOLEAN useLocalWhenFailed = TRUE ) ;

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

         INT32       getOrUpdateCataInfo( const CHAR *collectionName,
                                          CoordCataInfoPtr &cataPtr,
                                          _pmdEDUCB *cb ) ;

      protected:
         void        setCataGroupInfo( CoordGroupInfoPtr &groupPtr ) ;
         void        addGroupInfo( CoordGroupInfoPtr &groupPtr ) ;

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

         INT32       _buildOmGroupInfo() ;

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

         _netRouteAgent                   *_pAgent ;
         pmdOptionsCB                     *_pOptionsCB ;
         CoordGroupInfoPtr                _emptyGroupPtr ;

         _IOmProxy                        *_pOmProxy ;
         _coordOmStrategyAgent            *_pOmStrategyAgent ;

   } ;
   typedef _coordResource coordResource ;

}

#endif // COORD_RESOURCE_HPP__

