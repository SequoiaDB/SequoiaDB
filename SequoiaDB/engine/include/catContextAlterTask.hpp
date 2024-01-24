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

   Source File Name = catContextAlterTask.hpp

   Descriptive Name = Alter-tasks for Catalog RunTime Context of Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context of Catalog.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef CATCONTEXTALTERTASK_HPP_
#define CATCONTEXTALTERTASK_HPP_

#include "catContextTask.hpp"
#include "rtnAlterJob.hpp"

using namespace bson ;

namespace engine
{
   class _SDB_DMSCB ;
   typedef _SDB_DMSCB SDB_DMSCB;
   /*
      _catCtxAlterTask define
    */
   class _catCtxAlterTask : public _catCtxDataTask
   {
      public :
         _catCtxAlterTask ( const std::string & dataName,
                            const rtnAlterTask * task ) ;
         virtual ~_catCtxAlterTask () ;

         OSS_INLINE const rtnAlterTask * getTask () const
         {
            return _task ;
         }

      protected :
         const rtnAlterTask * _task ;
   } ;

   /*
      _catCtxAlterCLTask define
    */
   class _catCtxAlterCLTask : public _catCtxAlterTask
   {
      public :
         _catCtxAlterCLTask ( const std::string & collection,
                              const rtnAlterTask * task ) ;
         virtual ~_catCtxAlterCLTask () ;

         INT32 startPostTasks ( _pmdEDUCB * cb,
                                SDB_DMSCB * pDmsCB,
                                SDB_DPSCB * pDpsCB,
                                INT16 w ) ;
         INT32 clearPostTasks ( _pmdEDUCB * cb, INT16 w ) ;

         OSS_INLINE const ossPoolList< UINT64 > & getPostTasks () const
         {
            return _postTasks ;
         }
         OSS_INLINE void setSubCLFlag ()
         {
            _subCLOFMainCL = TRUE ;
         }
         OSS_INLINE const ossPoolList<BSONObj>& getIndexes () const
         {
            return _createIdxList ;
         }

      protected :
         virtual INT32 _checkInternal ( _pmdEDUCB * cb,
                                        catCtxLockMgr & lockMgr ) ;

         virtual INT32 _executeInternal ( _pmdEDUCB * cb,
                                          SDB_DMSCB * pDmsCB,
                                          SDB_DPSCB * pDpsCB,
                                          INT16 w ) ;

         virtual INT32 _rollbackInternal ( _pmdEDUCB * cb,
                                           SDB_DMSCB * pDmsCB,
                                           SDB_DPSCB * pDpsCB,
                                           INT16 w ) ;

         INT32 _buildFields ( clsCatalogSet & cataSet,
                              bson::BSONObj & setObject,
                              bson::BSONObj & unsetObject ) ;

         INT32 _buildRollbackFields ( clsCatalogSet & cataSet,
                                      bson::BSONObj & setObject,
                                      bson::BSONObj & unsetObject ) ;

         INT32 _checkCreateIDIndex ( const clsCatalogSet & cataSet,
                                     _pmdEDUCB * cb,
                                     catCtxLockMgr & lockMgr ) ;

         INT32 _checkDropIDIndex ( const clsCatalogSet & cataSet,
                                   _pmdEDUCB * cb,
                                   catCtxLockMgr & lockMgr ) ;

         INT32 _checkEnableShard ( const clsCatalogSet & cataSet,
                                   const rtnCLShardingArgument & argument,
                                   _pmdEDUCB * cb,
                                   catCtxLockMgr & lockMgr ) ;

         INT32 _checkDisableShard ( const clsCatalogSet & cataSet,
                                    _pmdEDUCB * cb,
                                    catCtxLockMgr & lockMgr ) ;

         INT32 _checkEnableCompress ( const clsCatalogSet & cataSet,
                                      _pmdEDUCB * cb,
                                      catCtxLockMgr & lockMgr ) ;

         INT32 _checkDisableCompress ( const clsCatalogSet & cataSet,
                                       _pmdEDUCB * cb,
                                       catCtxLockMgr & lockMgr ) ;

         INT32 _checkSetAttributes ( const clsCatalogSet & cataSet,
                                     _pmdEDUCB * cb,
                                     catCtxLockMgr & lockMgr ) ;
         INT32 _checkCreateAutoIncField ( const clsCatalogSet & cataSet,
                                          _pmdEDUCB * cb,
                                          catCtxLockMgr & lockMgr ) ;
         INT32 _checkDropAutoIncField ( const clsCatalogSet & cataSet,
                                       _pmdEDUCB * cb,
                                       catCtxLockMgr & lockMgr ) ;

         INT32 _buildEnableShardFields ( clsCatalogSet & cataSet,
                                         const rtnCLShardingArgument & argument,
                                         BOOLEAN postAutoSplit,
                                         UINT32 & attribute,
                                         bson::BSONObjBuilder & setBuilder,
                                         bson::BSONObjBuilder & unsetBuilder ) ;

         INT32 _buildDisableShardFields ( clsCatalogSet & cataSet,
                                          UINT32 & attribute,
                                          bson::BSONObjBuilder & setBuilder,
                                          bson::BSONObjBuilder & unsetBuilder ) ;

         INT32 _buildEnableCompressFields ( clsCatalogSet & cataSet,
                                            const rtnCLCompressArgument & argument,
                                            UINT32 & attribute,
                                            bson::BSONObjBuilder & setBuilder,
                                            bson::BSONObjBuilder & unsetBuilder ) ;

         INT32 _buildDisableCompressFields ( clsCatalogSet & cataSet,
                                             UINT32 & attribute,
                                             bson::BSONObjBuilder & setBuilder,
                                             bson::BSONObjBuilder & unsetBuilder ) ;

         INT32 _buildExtOptionFields ( clsCatalogSet & cataSet,
                                       const rtnCLExtOptionArgument & argument,
                                       UINT32 & attribute,
                                       bson::BSONObjBuilder & setBuilder,
                                       bson::BSONObjBuilder & unsetBuilder ) ;

         INT32 _buildSetAttributeFields ( clsCatalogSet & cataSet,
                                          UINT32 & attribute,
                                          bson::BSONObjBuilder & setBuilder,
                                          bson::BSONObjBuilder & unsetBuilder ) ;
         INT32 _buildSetAutoincFields ( clsCatalogSet & cataSet,
                                        autoIncFieldsList &fieldList,
                                        BSONObjBuilder & setBuilder,
                                        BSONObjBuilder & unsetBuilder,
                                        BOOLEAN addRbk ) ;

         INT32 _buildCreateAutoincFields ( clsCatalogSet & cataSet,
                                           autoIncFieldsList & fieldList,
                                           BSONObjBuilder & setBuilder,
                                           BSONObjBuilder & unsetBuilder ) ;
         INT32 _buildDropAutoincFields ( clsCatalogSet & cataSet,
                                         autoIncFieldsList & fieldList,
                                         BSONObjBuilder & setBuilder,
                                         BSONObjBuilder & unsetBuilder,
                                         BOOLEAN addRbk ) ;

         INT32 _fillShardingArgument ( clsCatalogSet & cataSet,
                                       rtnCLShardingArgument & argument ) ;

         INT32 _buildPostTasks ( clsCatalogSet & cataSet,
                                 _pmdEDUCB * cb,
                                 SDB_DMSCB * pDmsCB,
                                 SDB_DPSCB * pDpsCB,
                                 INT16 w ) ;

         INT32 _buildAutoHashSplit ( clsCatalogSet & cataSet,
                                     _pmdEDUCB * cb,
                                     SDB_DMSCB * pDmsCB,
                                     SDB_DPSCB * pDpsCB,
                                     INT16 w ) ;
         INT32 _buildSequenceNames ( clsCatalogSet & cataSet,
                                    autoIncFieldsList &fldList,
                                    _pmdEDUCB * cb,
                                    INT16 w ) ;

         // Helper functions
         INT32 _checkAutoSplit ( const clsCatalogSet & cataSet,
                                 const rtnCLShardingArgument & argument,
                                 _pmdEDUCB * cb,
                                 catCtxLockMgr & lockMgr ) ;

         INT32 _checkSysIndex( const clsCatalogSet& cataSet,
                               _pmdEDUCB* cb ) ;

         INT32 _buildSysIndexInfo( const CHAR* collection,
                                   const CHAR* indexName,
                                   const BSONObj& indexDef,
                                   _pmdEDUCB * cb ) ;

         INT32 _checkTaskConflict( const CHAR *collection,
                                   const BSONObj &indexDef,
                                   _pmdEDUCB *cb ) ;

         INT32 _executeSysIndex( const CHAR* collection,
                                 _pmdEDUCB* cb,
                                 INT16 w ) ;

      protected :
         ossPoolList< UINT64 >   _postTasks ;
         BOOLEAN                 _postAutoSplit ;

         ossPoolList< BSONObj >  _createIdxList ;
         BOOLEAN                 _dropIdIdx ;
         BOOLEAN                 _dropShardIdx ;

         rtnCLShardingArgument   _rollbackShardArgument ;

         BOOLEAN                 _subCLOFMainCL ;
         autoIncFieldsList       _rollbackAutoIncFields ;
   } ;

   typedef class _catCtxAlterCLTask catCtxAlterCLTask ;

   class _catCtxSequenceTask : public _catCtxTaskBase
   {
   public :
      _catCtxSequenceTask ( const std::string & collection,
                                 const rtnAlterTask * task )
      : _collection( collection ) ,
        _seqTask( task )
      {}
      virtual ~_catCtxSequenceTask () {} ;

      OSS_INLINE const rtnAlterTask * getSeqTask () const
      {
         return _seqTask ;
      }
      OSS_INLINE vector<BSONObj>&  getRollbackObj ()
      {
         return _rollbackObj ;
      }
      virtual INT32 _executeInternal ( _pmdEDUCB * cb,
                                       SDB_DMSCB * pDmsCB,
                                       SDB_DPSCB * pDpsCB,
                                       INT16 w ) = 0 ;
      INT32 getCLUniqueID(_pmdEDUCB *cb, utilCLUniqueID *clUniqueID) ;
   protected :
      std::string _collection ;
      const rtnAlterTask * _seqTask ;
      std::vector<BSONObj> _rollbackObj ;
   };
   typedef class _catCtxSequenceTask catCtxSequenceTask ;

   class _catCtxAlterSequenceTask : public _catCtxAlterTask
   {
   public :
      _catCtxAlterSequenceTask ( const std::string & dataName,
                                       const rtnAlterTask * task )
      : _catCtxAlterTask( dataName, task )
      {}
      ~_catCtxAlterSequenceTask () {} ;
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;
      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;
      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb,
                                        SDB_DMSCB *pDmsCB,
                                        SDB_DPSCB *pDpsCB,
                                        INT16 w ) ;
      OSS_INLINE vector< rtnCLAutoincFieldArgument > & getRollbackObj ()
      {
         return _rollbackObj ;
      }

      INT32 getCLUniqueID(_pmdEDUCB *cb, utilCLUniqueID *clUniqueID) ;

   protected:
      std::vector< rtnCLAutoincFieldArgument > _rollbackObj ;

   };
   typedef class _catCtxAlterSequenceTask catCtxAlterSequenceTask ;

   class _catCtxCreateSequenceTask : public _catCtxSequenceTask
   {
   public :
      _catCtxCreateSequenceTask ( const string & collection,
                                          const rtnAlterTask * task )
      : _catCtxSequenceTask( collection, task )
      {}
      virtual ~_catCtxCreateSequenceTask () {} ;
      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb,
                                          SDB_DMSCB *pDmsCB,
                                          SDB_DPSCB *pDpsCB,
                                          INT16 w )
      {
         return SDB_OK;
      }
      virtual INT32 _executeInternal ( _pmdEDUCB * cb,
                                       SDB_DMSCB * pDmsCB,
                                       SDB_DPSCB * pDpsCB,
                                       INT16 w ) ;
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb,
                                        SDB_DMSCB *pDmsCB,
                                        SDB_DPSCB *pDpsCB,
                                        INT16 w ) ;

   };
   typedef class _catCtxCreateSequenceTask catCtxCreateSequenceTask ;

   class _catCtxDropSequenceTask : public _catCtxSequenceTask
   {
   public :
      _catCtxDropSequenceTask ( const string & collection,
                                       const rtnAlterTask * task )
      : _catCtxSequenceTask( collection, task )
      {}
      virtual ~_catCtxDropSequenceTask () {} ;
      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb,
                                          SDB_DMSCB *pDmsCB,
                                          SDB_DPSCB *pDpsCB,
                                          INT16 w )
      {
         return SDB_OK ;
      }
      virtual INT32 _checkInternal ( _pmdEDUCB *cb, catCtxLockMgr &lockMgr ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb,
                                       SDB_DMSCB *pDmsCB,
                                       SDB_DPSCB *pDpsCB,
                                       INT16 w ) ;
      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb,
                                        SDB_DMSCB *pDmsCB,
                                        SDB_DPSCB *pDpsCB,
                                        INT16 w ) ;

   };
   typedef class _catCtxDropSequenceTask catCtxDropSequenceTask ;

   /*
      _catCtxAlterCSTask define
    */
   class _catCtxAlterCSTask : public _catCtxAlterTask
   {
      public :
         _catCtxAlterCSTask ( const std::string & collectionSpace,
                              const rtnAlterTask * task ) ;
         virtual ~_catCtxAlterCSTask () ;

         OSS_INLINE const CAT_GROUP_LIST & getGroups () const
         {
            return _groups ;
         }

      protected :
         virtual INT32 _checkInternal ( _pmdEDUCB * cb,
                                        catCtxLockMgr & lockMgr ) ;

         virtual INT32 _executeInternal ( _pmdEDUCB * cb,
                                          SDB_DMSCB * pDmsCB,
                                          SDB_DPSCB * pDpsCB,
                                          INT16 w ) ;

         INT32 _checkSetDomain ( _pmdEDUCB * cb,
                                 catCtxLockMgr & lockMgr ) ;

         INT32 _checkRemoveDomain ( _pmdEDUCB * cb,
                                    catCtxLockMgr & lockMgr ) ;

         INT32 _checkEnableCapped ( _pmdEDUCB * cb,
                                    catCtxLockMgr & lockMgr ) ;

         INT32 _checkDisableCapped ( _pmdEDUCB * cb,
                                     catCtxLockMgr & lockMgr ) ;

         INT32 _checkSetAttributes ( _pmdEDUCB * cb,
                                     catCtxLockMgr & lockMgr ) ;

         INT32 _checkDomainGroups ( const CHAR * domain,
                                    _pmdEDUCB * cb,
                                    catCtxLockMgr & lockMgr ) ;

         INT32 _checkGroups ( _pmdEDUCB * cb,
                              catCtxLockMgr & lockMgr ) ;

         INT32 _checkEmptyCollectionSpace ( _pmdEDUCB * cb ) ;

         virtual INT32 _buildSetFields ( BSONObj & setObject ) ;

         virtual INT32 _buildUnsetFields ( BSONObj & unsetObject ) ;

      protected :
         CAT_GROUP_LIST _groups ;
   } ;

   typedef class _catCtxAlterCSTask catCtxAlterCSTask ;

   /*
      _catCtxAlterDomainTask define
    */
   typedef map< std::string, UINT32 >  CAT_DOMAIN_GROUP_MAP ;

   class _catCtxAlterDomainTask : public _catCtxAlterTask
   {
      public :
         _catCtxAlterDomainTask ( const std::string & domain,
                                  const rtnAlterTask * task ) ;
         virtual ~_catCtxAlterDomainTask () ;

      public:
         OSS_INLINE const CAT_DOMAIN_GROUP_MAP& getGroupMap() const
         {
            return _groupMap ;
         }

         OSS_INLINE const CAT_GROUP_LIST& getFailedGroupList() const
         {
            return _failedGroupLst ;
         }

         virtual INT32 buildDomainGroups ( BSONObjBuilder & builder,
                                           const CHAR* arrayName = CAT_GROUPS_NAME ) ;

      protected :
         virtual INT32 _checkInternal ( _pmdEDUCB * cb,
                                        catCtxLockMgr & lockMgr ) ;

         virtual INT32 _executeInternal ( _pmdEDUCB * cb,
                                          SDB_DMSCB * pDmsCB,
                                          SDB_DPSCB * pDpsCB,
                                          INT16 w ) ;

         virtual INT32 _checkAddGroupTask ( _pmdEDUCB * cb ) ;

         virtual INT32 _checkRemoveGroupTask ( _pmdEDUCB * cb ) ;

         virtual INT32 _checkSetGroupTask ( _pmdEDUCB * cb ) ;

         virtual INT32 _checkSetAttrTask ( _pmdEDUCB * cb ) ;

         virtual INT32 _checkAddingGroups ( const RTN_DOMAIN_GROUP_LIST & groups,
                                            _pmdEDUCB * cb ) ;

         virtual INT32 _checkRemovingGroups ( const RTN_DOMAIN_GROUP_LIST & groups,
                                              _pmdEDUCB * cb ) ;

         virtual void _extractGroups ( const RTN_DOMAIN_GROUP_LIST & groups,
                                       RTN_DOMAIN_GROUP_LIST & addingGroups,
                                       RTN_DOMAIN_GROUP_LIST & removingGroups ) ;

         INT32 _executeSetActiveLocationTask( _pmdEDUCB * cb ) ;

         INT32 _executeSetLocationTask( _pmdEDUCB * cb ) ;

      protected :
         CAT_DOMAIN_GROUP_MAP    _groupMap ;
         CAT_GROUP_LIST          _failedGroupLst ;
   } ;

   typedef class _catCtxAlterDomainTask catCtxAlterDomainTask ;

}

#endif //CATCONTEXTBASEALTERTASK_HPP_
