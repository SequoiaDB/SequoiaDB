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

   Source File Name = catCommon.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/07/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_COMMON_HPP__
#define CAT_COMMON_HPP__

#include "core.hpp"
#include "pd.hpp"
#include "oss.hpp"
#include "ossErr.h"
#include "ossMemPool.hpp"
#include "../bson/bson.h"
#include "catDef.hpp"
#include "catContext.hpp"
#include "catContextData.hpp"
#include "catContextNode.hpp"

using namespace bson ;

namespace engine
{

   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   struct cmp_str
   {
      BOOLEAN operator() ( const CHAR *a, const CHAR *b )
      {
         return ossStrcmp( a, b ) < 0 ;
      }
   } ;

   /* Check group name is valid */
   INT32 catGroupNameValidate ( const CHAR *pName, BOOLEAN isSys = FALSE ) ;

   /* Check domain name is valid */
   INT32 catDomainNameValidate( const CHAR *pName ) ;


   /* extract options of domain. when builder is NULL only check validation */
   INT32 catDomainOptionsExtract( const BSONObj &options,
                                  pmdEDUCB *cb,
                                  BSONObjBuilder *builder = NULL,
                                  vector< string > *pVecGroups = NULL ) ;

   /* Query and return result */
   INT32 catQueryAndGetMore ( MsgOpReply **ppReply,
                              const CHAR *collectionName,
                              BSONObj &selector,
                              BSONObj &matcher,
                              BSONObj &orderBy,
                              BSONObj &hint,
                              SINT32 flags,
                              pmdEDUCB *cb,
                              SINT64 numToSkip,
                              SINT64 numToReturn ) ;

   /* Query and get one object */
   INT32 catGetOneObj( const CHAR *collectionName,
                       const BSONObj &selector,
                       const BSONObj &matcher,
                       const BSONObj &hint,
                       pmdEDUCB *cb,
                       BSONObj &obj ) ;

   /* Query and get count of objects */
   INT32 catGetObjectCount ( const CHAR * collectionName,
                             const BSONObj & selector,
                             const BSONObj & matcher,
                             const BSONObj & hint,
                             pmdEDUCB * cb,
                             INT64 & count ) ;

   /* Collection[CAT_NODE_INFO_COLLECTION] functions: */
   INT32 catGetGroupObj( const CHAR *groupName,
                         BOOLEAN dataGroupOnly,
                         BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catGetGroupObj( UINT32 groupID, BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catGetGroupObj( UINT16 nodeID, BSONObj &obj, pmdEDUCB *cb ) ;

   INT32 catGroupCheck( const CHAR *groupName, BOOLEAN &exist, pmdEDUCB *cb ) ;
   INT32 catServiceCheck( const CHAR *hostName, const CHAR *serviceName,
                          BOOLEAN &exist, pmdEDUCB *cb ) ;

   INT32 catGroupID2Name( UINT32 groupID, string &groupName, pmdEDUCB *cb ) ;
   INT32 catGroupName2ID( const CHAR *groupName, UINT32 &groupID,
                          BOOLEAN dataGroupOnly, pmdEDUCB *cb ) ;

   INT32 catGroupCount( INT64 & count, pmdEDUCB * cb ) ;

   /* Collection[CAT_DOMAIN_COLLECTION] functions: */
   INT32 catGetDomainObj( const CHAR *domainName, BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catCheckDomainExist( const CHAR *pDomainName,
                              BOOLEAN &isExist,
                              BSONObj &obj,
                              pmdEDUCB *cb ) ;

   INT32 catGetDomainGroups( const BSONObj &domain,
                             map<string, UINT32> &groups ) ;
   INT32 catGetDomainGroups( const BSONObj &domain,
                             vector< UINT32 > &groupIDs ) ;
   INT32 catAddGroup2Domain( const CHAR *domainName, const CHAR *groupName,
                             INT32 groupID, pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                             _dpsLogWrapper *dpsCB, INT16 w ) ;

   INT32 catGetSplitCandidateGroups ( const CHAR * collection,
                                      std::map< std::string, UINT32 > & groups,
                                      pmdEDUCB * cb ) ;

   /*
      Note: domainName == NULL, while del group from all domain
   */
   INT32 catDelGroupFromDomain( const CHAR *domainName, const CHAR *groupName,
                                UINT32 groupID, pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                                _dpsLogWrapper *dpsCB, INT16 w ) ;

   INT32 catUpdateDomain ( const CHAR * domainName,
                           const BSONObj & domainObject,
                           pmdEDUCB * cb,
                           _SDB_DMSCB * dmsCB,
                           _dpsLogWrapper * dpsCB,
                           INT16 w ) ;

   /* Collection[CAT_COLLECTION_SPACE_COLLECTION] functions: */
   INT32 catAddCL2CS( const CHAR *csName, const CHAR *clName,
                      utilCLUniqueID clUniqueID,
                      pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                      _dpsLogWrapper * dpsCB, INT16 w ) ;

   INT32 catDelCLFromCS( const string &clFullName,
                         pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                         _dpsLogWrapper * dpsCB,
                         INT16 w ) ;

   INT32 catRenameCLFromCS( const string &csName,
                            const string &clShortName,
                            const string &newCLShortName,
                            pmdEDUCB * cb, SDB_DMSCB * dmsCB,
                            SDB_DPSCB * dpsCB, INT16 w ) ;

   INT32 catDelCLsFromCS( const string &csName,
                          const vector<string> &deleteCLLst,
                          pmdEDUCB * cb, SDB_DMSCB * dmsCB, SDB_DPSCB * dpsCB,
                          INT16 w ) ;
   INT32 catUpdateCSCLs( const string &csName,
                         vector< PAIR_CLNAME_ID > &collections,
                         pmdEDUCB *cb, _SDB_DMSCB * dmsCB, _dpsLogWrapper * dpsCB,
                         INT16 w ) ;
   INT32 catUpdateCS ( const CHAR * csName,
                       const BSONObj & setObject,
                       const BSONObj & unsetObject,
                       pmdEDUCB * cb, _SDB_DMSCB * dmsCB,
                       _dpsLogWrapper * dpsCB,
                       INT16 w ) ;
   INT32 catCheckSpaceExist( const char *pSpaceName,
                             BOOLEAN &isExist,
                             BSONObj &obj,
                             pmdEDUCB *cb ) ;
   INT32 catCheckSpaceExist( const char *pSpaceName,
                             utilCSUniqueID csUniqueID,
                             BOOLEAN &isExist,
                             BSONObj &obj,
                             pmdEDUCB *cb ) ;
   INT32 catGetDomainCSs ( const CHAR * domain, pmdEDUCB * cb,
                           ossPoolList< std::string > & collectionSpaces ) ;

   /* Collection[CAT_COLLECTION_INFO_COLLECTION] functions: */
   INT32 catRemoveCL( const CHAR *clFullName, pmdEDUCB *cb, _SDB_DMSCB * dmsCB,
                      _dpsLogWrapper * dpsCB, INT16 w );

   INT32 catCheckCollectionExist( const CHAR *pCollectionName,
                                  BOOLEAN &isExist,
                                  BSONObj &obj,
                                  pmdEDUCB *cb ) ;

   INT32 catUpdateCatalog ( const CHAR * clFullName, const BSONObj & setInfo,
                            const BSONObj & unsetInfo, pmdEDUCB * cb, INT16 w,
                            BOOLEAN incVersion = TRUE ) ;

   INT32 catUpdateCatalogByPush ( const CHAR * clFullName,
                                  const CHAR *field, const BSONObj &boObj,
                                  pmdEDUCB * cb, INT16 w ) ;

   INT32 catUpdateCatalogByUnset( const CHAR * clFullName, const CHAR * field,
                                  pmdEDUCB * cb, INT16 w ) ;

   INT32 catGetCSGroupsFromCLs( const CHAR *csName, pmdEDUCB *cb,
                                vector< UINT32 > &groups,
                                BOOLEAN includeSubCLGroups = FALSE ) ;
   INT32 catGetCSGroups ( const CHAR * csName,
                          pmdEDUCB * cb,
                          ossPoolSet< UINT32 > & groups,
                          BOOLEAN includeSubCLGroups = FALSE ) ;

   /* Collection[CAT_TASK_INFO_COLLECTION] functions: */
   INT32 catAddTask( BSONObj & taskObj, pmdEDUCB *cb, INT16 w ) ;
   INT32 catGetTask( UINT64 taskID, BSONObj &obj, pmdEDUCB *cb ) ;
   INT32 catGetTaskCount ( const CHAR *collection, pmdEDUCB *cb, INT64 &count );
   INT32 catGetTaskCountByCS( const CHAR *csName, pmdEDUCB *cb, INT64 &count ) ;
   INT32 catGetCLTaskCountByType( const CHAR * collection, pmdEDUCB * cb,
                                  CLS_TASK_TYPE type, INT64 & count ) ;
   INT32 catGetTaskStatus( UINT64 taskID, INT32 &status, pmdEDUCB *cb ) ;
   INT32 catUpdateTaskStatus( UINT64 taskID, INT32 status, pmdEDUCB *cb,
                              INT16 w ) ;
   INT64 catGetMaxTaskID( pmdEDUCB *cb ) ;
   INT32 catRemoveTask( BSONObj & match, BOOLEAN checkExist, pmdEDUCB * cb,
                        INT16 w ) ;
   INT32 catRemoveTask( UINT64 taskID, BOOLEAN checkExist, pmdEDUCB *cb,
                        INT16 w ) ;
   INT32 catRemoveCLTasks( const string &clName, pmdEDUCB *cb, INT16 w ) ;
   INT32 catRemoveSequenceTasks ( const CHAR * sequenceName, pmdEDUCB * cb,
                                  INT16 w ) ;
   INT32 catRemoveTasksByType ( CLS_TASK_TYPE type, pmdEDUCB * cb, INT16 w ) ;
   INT32 catGetCSGroupsFromTasks( const CHAR *csName, pmdEDUCB *cb,
                                  vector< UINT32 > &groups ) ;
   INT32 catGetCSTaskGroups ( const CHAR * csName,
                              pmdEDUCB * cb,
                              ossPoolSet< UINT32 > & groups ) ;

   /* Collection[CAT_HISTORY_COLLECTION] functions */
   INT32 catGetBucketVersion( const CHAR *pCLName, pmdEDUCB *cb ) ;
   INT32 catSaveBucketVersion( const CHAR *pCLName, INT32 version,
                               pmdEDUCB *cb, INT16 w ) ;

   /* Collection[CAT_SYSDCBASE_COLLECTION_NAME] functions */
   INT32 catCheckBaseInfoExist( const CHAR *pTypeStr,
                                BOOLEAN &isExist,
                                BSONObj &obj,
                                pmdEDUCB *cb ) ;
   INT32 catUpdateBaseInfoAddr( const CHAR *pAddr,
                                BOOLEAN self,
                                pmdEDUCB *cb,
                                INT16 w ) ;
   INT32 catEnableImage( BOOLEAN enable, pmdEDUCB *cb, INT16 w,
                         _SDB_DMSCB *dmsCB, _dpsLogWrapper * dpsCB ) ;

   INT32 catUpdateDCStatus( const CHAR *pField, BOOLEAN status,
                            pmdEDUCB *cb, INT16 w,
                            _SDB_DMSCB *dmsCB, _dpsLogWrapper *dpsCB ) ;

   INT32 catSetCSUniqueHWM( pmdEDUCB *cb, INT16 w, UINT32 csUniqueHWM ) ;

   INT32 catUpdateCSUniqueID( pmdEDUCB *cb, INT16 w,
                              utilCSUniqueID& csUniqueID ) ;
   INT32 catUpdateGlobalID( pmdEDUCB *cb, INT16 w,
                            utilGlobalID& globalID ) ;

   /* Other Tools */
   INT32 catPraseFunc( const BSONObj &func, BSONObj &parsed ) ;

   UINT32 catCalcBucketID( const CHAR *pData, UINT32 length,
                           UINT32 bucketSize = CAT_BUCKET_SIZE ) ;

   INT32 catCreateContext ( MSG_TYPE cmdType,
                            catContext **context,
                            SINT64 &contextID,
                            _pmdEDUCB * pEDUCB ) ;

   INT32 catFindContext ( SINT64 contextID,
                          catContext **pCtx,
                          _pmdEDUCB *pEDUCB ) ;

   INT32 catDeleteContext ( SINT64 contextID,
                            _pmdEDUCB *pEDUCB ) ;

   /* Get Collection */
   INT32 catGetCollection ( const string &clName, BSONObj &boCollection,
                            _pmdEDUCB *cb ) ;

   /* Check whether collection is main collection */
   INT32 catCheckMainCollection ( const BSONObj &boCollection,
                                  BOOLEAN expectMain ) ;

   /* Check whether the collection will be re-linked */
   INT32 catCheckRelinkCollection ( const BSONObj &boCollection,
                                    string &mainCLName ) ;

   /* Get group list from collection */
   INT32 catGetCollectionGroups ( const BSONObj &boCollection,
                                  vector<UINT32> &groupIDList,
                                  vector<string> &groupNameList ) ;

   /* Get and lock Domain */
   INT32 catGetAndLockDomain ( const string &domainName, BSONObj &boDomain,
                               _pmdEDUCB *cb,
                               catCtxLockMgr *pLockMgr, OSS_LATCH_MODE mode ) ;

   /* Get and lock Collection Space */
   INT32 catGetAndLockCollectionSpace ( const string &csName, BSONObj &boSpace,
                                        _pmdEDUCB *cb,
                                        catCtxLockMgr *pLockMgr,
                                        OSS_LATCH_MODE mode ) ;

   /* Get and lock Collection */
   INT32 catGetAndLockCollection ( const string &clName, BSONObj &boCollection,
                                   _pmdEDUCB *cb,
                                   catCtxLockMgr *pLockMgr,
                                   OSS_LATCH_MODE mode ) ;

   /* Get and lock groups of Collection */
   INT32 catGetAndLockCollectionGroups ( const BSONObj &boCollection,
                                         vector<UINT32> &groupIDList,
                                         catCtxLockMgr &lockMgr,
                                         OSS_LATCH_MODE mode ) ;

   /* Get groups of Collection */
   INT32 catGetCollectionGroupSet ( const BSONObj &boCollection,
                                    vector<UINT32> &groupIDList ) ;

   /* Lock groups */
   INT32 catLockGroups( vector<UINT32> &groupIDList,
                        _pmdEDUCB *cb,
                        catCtxLockMgr &lockMgr,
                        OSS_LATCH_MODE mode ) ;

   /* Get groups of Collection including its sub-collections */
   INT32 catGetCollectionGroupsCascade ( const std::string &clName,
                                         const BSONObj &boCollection,
                                         _pmdEDUCB *cb,
                                         std::vector<UINT32> &groupIDList ) ;

   /* Check available of groups */
   INT32 catCheckGroupsByID ( std::vector<UINT32> &groupIDList ) ;

   INT32 catCheckGroupsByName ( std::vector<std::string> &groupNameList ) ;

   INT32 catMainCLRename( const string &mainCLName, const string &newMainCLName,
                          clsCatalogSet &mainclCata,
                          _pmdEDUCB *cb, INT16 w ) ;

   INT32 catSubCLRename( const string &subCLName, const string &newSubCLName,
                         clsCatalogSet &subclCata,
                         _pmdEDUCB *cb, INT16 w ) ;


   /* Drop Collection Space */
   INT32 catDropCSStep ( const string &csName,
                         _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                         INT16 w ) ;

   /* Rename Collection Space */
   INT32 catRenameCSStep ( const string &oldCSName, const string &newCSName,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w ) ;

   /* Rename Collection */
   INT32 catRenameCLStep ( const string &oldCLName, const string &newCLName,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w ) ;

   /* Create Collection */
   INT32 catCreateCLStep ( const string &clName, utilCLUniqueID clUniqueID,
                           BSONObj &boCollection,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w ) ;

   /* Drop Collection */
   INT32 catDropCLStep ( const string &clName, INT32 version, BOOLEAN delFromCS,
                         _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                         INT16 w ) ;

   /* Alter Collection */
   INT32 catAlterCLStep ( const string &clName, const BSONObj &boNewData,
                          const BSONObj & boUnsetData,
                          _pmdEDUCB *cb,
                          SDB_DMSCB *pDmsCB,
                          SDB_DPSCB *pDpsCB,
                          INT16 w ) ;

   /* Link main Collection */
   INT32 catLinkMainCLStep ( const string &mainCLName, const string &subCLName,
                             const BSONObj &lowBound, const BSONObj &upBound,
                             _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                             INT16 w ) ;

   /* Link sub Collection */
   INT32 catLinkSubCLStep ( const string &mainCLName, const string &subCLName,
                            _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                            INT16 w ) ;

   /* Unlink main Collection */
   INT32 catUnlinkMainCLStep ( const string &mainCLName,
                               const string &subCLName,
                               BOOLEAN needBounds,
                               BSONObj &lowBound, BSONObj &upBound,
                               _pmdEDUCB *cb,
                               SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                               INT16 w ) ;

   /* Unlink sub Collection */
   INT32 catUnlinkSubCLStep ( const string &subCLName,
                              _pmdEDUCB *cb,
                              SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                              INT16 w ) ;

   /* Unlink sub Collections from the same Space */
   INT32 catUnlinkCSStep ( const string &mainCLName, const string &csName,
                           _pmdEDUCB *cb, SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                           INT16 w ) ;

   /* Check and build Collection record */
   INT32 catCheckAndBuildCataRecord ( const BSONObj &boCollection,
                                      UINT32 &fieldMask,
                                      catCollectionInfo &clInfo ) ;

   INT32 catBuildInitRangeBound ( const BSONObj & shardingKey,
                                  const Ordering & order,
                                  BSONObj & lowBound, BSONObj & upBound ) ;

   INT32 catBuildInitHashBound ( BSONObj & lowBound, BSONObj & upBound,
                                 INT32 paritition ) ;

   INT32 catBuildHashBound ( BSONObj & lowBound, BSONObj & upBound,
                             INT32 beginBound, INT32 endBound ) ;

   INT32 catBuildHashSplitTask ( const CHAR * collection,
                                 utilCLUniqueID clUniqueID,
                                 const CHAR * srcGroup,
                                 const CHAR * dstGroup,
                                 UINT32 beginBound,
                                 UINT32 endBound,
                                 BSONObj & splitTask ) ;

   /* Build Collection record */
   INT32 catBuildCatalogRecord ( _pmdEDUCB *cb,
                                 catCollectionInfo &clInfo,
                                 UINT32 mask,
                                 UINT32 attribute,
                                 const std::vector<UINT32> &grpIDLst,
                                 const std::map<std::string, UINT32> &splitLst,
                                 BSONObj &catRecord,
                                 INT16 w ) ;

   /* Create Node */
   INT32 catCreateNodeStep ( const string &groupName, const string &hostName,
                             const string &dbPath, UINT32 instanceID,
                             const string &localSvc, const string &replSvc,
                             const string &shardSvc, const string &cataSvc,
                             INT32 nodeRole, UINT16 nodeID, INT32 nodeStatus,
                             _pmdEDUCB *cb, SDB_DMSCB *pDmsCB,
                             SDB_DPSCB *pDpsCB, INT16 w ) ;

   /* Remove Node */
   INT32 catRemoveNodeStep ( const string &groupName,
                             UINT16 nodeID,
                             _pmdEDUCB *cb,
                             SDB_DMSCB *pDmsCB, SDB_DPSCB *pDpsCB,
                             INT16 w ) ;

   /* Transaction control */
   INT32 catTransBegin ( _pmdEDUCB *cb ) ;
   INT32 catTransEnd ( INT32 result, _pmdEDUCB *cb, SDB_DPSCB *pDpsCB ) ;

   /* Catalog group sync control */
   void  catSetSyncW ( INT16 w ) ;
   INT16 catGetSyncW () ;

   /* AutoIncrement */
   INT32 catValidSequenceOption( const BSONObj &option ) ;
   INT32 catCreateAutoIncSequences( const catCollectionInfo &clInfo,
                                    _pmdEDUCB *cb, INT16 w ) ;
   INT32 catDropAutoIncSequences( const BSONObj &boCollection,
                                  _pmdEDUCB *cb,
                                  INT16 w ) ;
   BSONObj catBuildSequenceOptions( const BSONObj &autoIncOpt,
                                    utilSequenceID ID = UTIL_SEQUENCEID_NULL,
                                    UINT32 fieldMask = UTIL_ARG_FIELD_ALL ) ;
   INT32  catBuildCatalogAutoIncField( _pmdEDUCB *cb,
                                       catCollectionInfo &clInfo,
                                       const BSONObj &obj,
                                       utilCLUniqueID clUniqueID ,
                                       INT16 w ) ;
   string catGetSeqName4AutoIncFld( const utilCLUniqueID id,
                                    const CHAR* fldName ) ;
}

#endif //CAT_COMMON_HPP__

