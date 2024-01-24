/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = catCommand.hpp

   Descriptive Name = Catalogue commands.

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_COMMAND_HPP__
#define CAT_COMMAND_HPP__
#include "catCMDBase.hpp"
#include "catLevelLock.hpp"
#include "IDataSource.hpp"
#include "rtnContextBuff.hpp"
#include "clsTask.hpp"
#include "utilDataSource.hpp"

namespace engine
{
   /**
    * Data source information. Used when creating a data source, to generate
    * a data source metadata record.
    */
   struct _catDSInfo
   {
      UINT32      _id ;
      INT32       _version ;
      const CHAR *_dsName;
      const CHAR *_type ;     // Only support "SequoiaDB" for now.
      const CHAR *_dsVersion ;
      const CHAR *_addresses ;
      const CHAR *_user ;
      const CHAR *_password ;
      const CHAR *_errCtlLevel ;
      INT32       _accessMode ;
      INT32       _errFilterMask ;
      // Where degrade transactional operation to non-transactional operation
      // and send to data source.
      const CHAR *_transPropagateMode ;
      BOOLEAN     _inheritSessionAttr ;

      _catDSInfo()
      {
         reset() ;
      }

      void reset()
      {
         _id = 0 ;
         _version = 0 ;
         _dsName = NULL ;
         _type = NULL ;
         _dsVersion = NULL ;
         _addresses = NULL ;
         _user = NULL ;
         _password = NULL ;
         _errCtlLevel = VALUE_NAME_LOW ;
         _accessMode = DS_ACCESS_DEFAULT ;
         _errFilterMask = DS_ERR_FILTER_NONE ;
         _transPropagateMode = VALUE_NAME_NEVER ;
         _inheritSessionAttr = TRUE ;
      }

      BSONObj toBson()
      {
         try
         {
            BSONObjBuilder builder ;
            builder.append( FIELD_NAME_ID, _id );
            builder.append( FIELD_NAME_NAME, _dsName ) ;
            builder.append( FIELD_NAME_TYPE, _type ) ;
            builder.append( FIELD_NAME_VERSION, _version ) ;
            builder.append( FIELD_NAME_DSVERSION, _dsVersion ) ;
            builder.append( FIELD_NAME_ADDRESS, _addresses ) ;
            builder.append( FIELD_NAME_USER, _user ? _user : "" ) ;
            builder.append( FIELD_NAME_PASSWD, _password ? _password : "" ) ;
            builder.append( FIELD_NAME_ERRORCTLLEVEL, _errCtlLevel ) ;
            builder.append( FIELD_NAME_ACCESSMODE, _accessMode ) ;
            const CHAR *desc = NULL ;
            DS_ACCESS_MODE_2_DESC( _accessMode, desc ) ;
            builder.append( FIELD_NAME_ACCESSMODE_DESC, desc ) ;
            builder.append( FIELD_NAME_ERRORFILTERMASK, _errFilterMask ) ;
            DS_ERR_FILTER_2_DESC( _errFilterMask, desc ) ;
            builder.append( FIELD_NAME_ERRORFILTERMASK_DESC, desc ) ;
            builder.append( FIELD_NAME_TRANS_PROPAGATE_MODE,
                            _transPropagateMode ) ;
            builder.appendBool( FIELD_NAME_INHERIT_SESSION_ATTR,
                                _inheritSessionAttr ) ;
            return builder.obj() ;
         }
         catch ( std::exception &e )
         {
            return BSONObj() ;
         }
      }
   } ;
   typedef _catDSInfo catDSInfo ;

   /**
    * @brief Create data source command. Metadata of the data source will be
    *        added into the system collection SYSDATASOURCES.
    */
   class _catCMDCreateDataSource : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDCreateDataSource() ;
      virtual ~_catCMDCreateDataSource() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR* name() const ;

      virtual const CHAR *getProcessName() const
      {
         return _dsInfo._dsName ;
      }

   private:
      catDSInfo _dsInfo ;
   } ;
   typedef _catCMDCreateDataSource catCMDCreateDataSource ;

   class _catCMDDropDataSource : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDDropDataSource() ;
      virtual ~_catCMDDropDataSource() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR* name() const ;

      virtual const CHAR *getProcessName() const
      {
         return _name ;
      }

   private:
      /**
       * Check if the data source is being used by any collection space or
       * collection.
       * @param used
       * @return
       */
      INT32 _checkUsage( BOOLEAN &used, pmdEDUCB *cb ) ;

   private:
      const CHAR *_name ;
      UTIL_DS_UID _dsID ;
      catCtxLockMgr _lockMgr ;
   } ;
   typedef _catCMDDropDataSource catCMDDropDataSource ;

   class _catCMDAlterDataSource : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDAlterDataSource() ;
      virtual ~_catCMDAlterDataSource() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR* name() const ;

      virtual const CHAR *getProcessName() const
      {
         return _dsName ;
      }

   private:
      INT32 _getDataSourceMeta( const CHAR *name, BSONObj &record ) ;

   private:
      const CHAR * _dsName ;
      UTIL_DS_UID _dsID ;
      BSONObjBuilder _optionBuilder ;
   } ;
   typedef _catCMDAlterDataSource catCMDAlterDataSource ;

   class _catCMDTestCollection : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDTestCollection() ;
      virtual ~_catCMDTestCollection() ;

      INT32 init( const CHAR *pQuery,
                  const CHAR *pSelector = NULL,
                  const CHAR *pOrderBy = NULL,
                  const CHAR *pHint = NULL,
                  INT32 flags = 0,
                  INT64 numToSkip = 0,
                  INT64 numToReturn = -1 ) ;

      INT32 doit( _pmdEDUCB *cb,
                  rtnContextBuf &ctxBuf,
                  INT64 &contextID ) ;

      const CHAR *name() const ;

      virtual const CHAR *getProcessName() const
      {
         return _name ;
      }

   private:
      const CHAR *_name ;

   } ;
   typedef _catCMDTestCollection catCMDTestCollection ;

   struct _catCSInfo
   {
      const CHAR*       _pCSName ;
      utilCSUniqueID    _csUniqueID ;
      utilCLUniqueID    _clUniqueHWM ;
      utilIdxUniqueID   _idxUniqueHWM ;
      INT32             _pageSize ;
      const CHAR*       _domainName ;
      INT32             _lobPageSize ;
      DMS_STORAGE_TYPE  _type ;
      const CHAR*       _pDataSourceName ;
      const CHAR*       _pDataSourceMapping ;
      UTIL_DS_UID       _dsUID ;

      _catCSInfo()
      {
         reset() ;
      }

      void reset()
      {
         _pCSName = NULL ;
         _csUniqueID = UTIL_UNIQUEID_NULL ;
         _clUniqueHWM = UTIL_UNIQUEID_NULL ;
         _idxUniqueHWM = UTIL_UNIQUEID_NULL ;
         _pageSize = DMS_PAGE_SIZE_DFT ;
         _domainName = NULL ;
         _lobPageSize = DMS_DEFAULT_LOB_PAGE_SZ ;
         _type = DMS_STORAGE_NORMAL ;
         _pDataSourceName = NULL ;
         _pDataSourceMapping = NULL ;
         _dsUID = UTIL_INVALID_DS_UID ;
      }

      BSONObj toBson()
      {
         BSONObjBuilder builder ;
         builder.append( CAT_COLLECTION_SPACE_NAME, _pCSName ) ;
         builder.append( CAT_CS_UNIQUEID, _csUniqueID ) ;
         builder.append( CAT_CS_CLUNIQUEHWM, (INT64)_clUniqueHWM ) ;
         builder.append( FIELD_NAME_IDXUNIQUEHWM, (INT64)_idxUniqueHWM ) ;
         builder.append( CAT_PAGE_SIZE_NAME, _pageSize ) ;
         if ( _domainName )
         {
            builder.append( CAT_DOMAIN_NAME, _domainName ) ;
         }
         builder.append( CAT_LOB_PAGE_SZ_NAME, _lobPageSize ) ;
         builder.append( CAT_TYPE_NAME, _type ) ;
         if ( UTIL_INVALID_DS_UID != _dsUID )
         {
            builder.append( FIELD_NAME_DATASOURCE_ID, _dsUID ) ;
            SDB_ASSERT( _pDataSourceMapping, "Mapping is NULL" ) ;
            builder.append( FIELD_NAME_MAPPING, _pDataSourceMapping ) ;
         }
         return builder.obj() ;
      }
   } ;
   typedef _catCSInfo catCSInfo ;

   /*
      _catCMDCreateCS define
   */
   class _catCMDCreateCS : public _catCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDCreateCS() ;
      virtual ~_catCMDCreateCS() {} ;

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          rtnContextBuf &ctxBuf,
                          INT64 &contextID ) ;

      virtual const CHAR* name() const
      {
         return CMD_NAME_CREATE_COLLECTIONSPACE ;
      }

      virtual const CHAR *getProcessName() const
      {
         return _csInfo._pCSName ;
      }

      virtual BOOLEAN needCheckPrimary() const { return TRUE ; }
      virtual BOOLEAN needCheckDCStatus() const { return TRUE ; }

   private:
      catCSInfo _csInfo ;
   };
   typedef _catCMDCreateCS catCMDCreateCS ;

   /*
      catCMDIndex define
   */
   class _catCMDIndexHelper : public _catCMDBase
   {
   protected:
      typedef ossPoolVector<clsIdxTask*>           VEC_TASKS ;
      typedef ossPoolVector<clsIdxTask*>::iterator VEC_TASKS_IT ;

   public:
      _catCMDIndexHelper( BOOLEAN sysCall ) ;
      virtual ~_catCMDIndexHelper() ;

      virtual const CHAR *getProcessName() const
      {
         return _pCollection ;
      }

      const CHAR* collectionName() { return _pCollection ; }
      const CHAR* indexName()      { return _pIndexName ; }

      void disableLevelLock() { _needLevelLock = FALSE ; }

   protected:
      INT32 _makeReply( UINT64 taskID, rtnContextBuf &ctxBuf ) ;

      INT32 _checkTaskConflict( _pmdEDUCB *cb ) ;

      INT32 _dropGlobalIdxCL( const CHAR *clName,
                              _pmdEDUCB *cb ) ;

   private:
      INT32 _checkTaskConflict( const BSONObj &otherIdxObj ) ;
   protected:
      clsCatalogSet*  _pCataSet ;

      const CHAR*     _pCollection ;
      const CHAR*     _pIndexName ;

      ossPoolSet<ossPoolString>  _groupSet ;

      VEC_TASKS       _vecTasks ;

      BOOLEAN         _sysCall ;
      BOOLEAN         _needLevelLock ;
   };
   typedef _catCMDIndexHelper catCMDIndexHelper ;

   /*
      catCMDCreateIndex define
   */

   class _catCMDCreateIndex : public _catCMDIndexHelper
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
      friend class _catCMDCopyIndex ;
   public:
      _catCMDCreateIndex( BOOLEAN sysCall = FALSE ) ;
      virtual ~_catCMDCreateIndex() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          rtnContextBuf &ctxBuf,
                          INT64 &contextID ) ;
      virtual INT32 postDoit( const clsTask *pTask, _pmdEDUCB *cb ) ;

      virtual const CHAR* name() const { return CMD_NAME_CREATE_INDEX ; }
      virtual BOOLEAN needCheckPrimary() const { return TRUE ; }
      virtual BOOLEAN needCheckDCStatus() const { return TRUE ; }

   protected:
      INT32 _check( _pmdEDUCB *cb ) ;
      INT32 _execute( _pmdEDUCB *cb, rtnContextBuf &ctxBuf ) ;

   private:
      INT32 _buildMainCLTask( _pmdEDUCB *cb, BOOLEAN mainCLRedef ) ;
      INT32 _buildCLTask( _pmdEDUCB *cb,
                          const CHAR* collectionName,
                          UINT64 mainTaskID = CLS_INVALID_TASKID,
                          UINT64* pTaskID = NULL ) ;

      INT32 _checkGlobalIndex( _pmdEDUCB *cb ) ;

      INT32 _checkUniqueKey( _pmdEDUCB *cb ) ;
      INT32 _checkUniqueKey( const clsCatalogSet& cataSet,
                             std::set<UINT32>& checkedKeyIDs ) ;

      INT32 _createGlobalIdxCL( _pmdEDUCB *cb ) ;
      INT32 _addGlobalInfo2Task() ;

   private:
      BSONObj         _boIdx ;
      BSONObj         _key ;
      INT32           _sortBufSz ;
      BOOLEAN         _isPrepareStep ;

      // global index
      BOOLEAN         _isUnique ;
      BOOLEAN         _isEnforced ;
      BOOLEAN         _isGlobal ;
      CHAR            _globalIdxCSName[DMS_COLLECTION_SPACE_NAME_SZ+1] ;
      CHAR            _globalIdxCLName[DMS_COLLECTION_FULL_NAME_SZ+1] ;
      utilCLUniqueID  _globalIdxCLUniqID ;
      string          _domainName ;
   };
   typedef _catCMDCreateIndex catCMDCreateIndex ;

   /*
      catCMDDropIndex define
   */
   class _catCMDDropIndex : public _catCMDIndexHelper
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDDropIndex( BOOLEAN sysCall = FALSE ) ;
      virtual ~_catCMDDropIndex() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          rtnContextBuf &ctxBuf,
                          INT64 &contextID ) ;
      virtual INT32 postDoit( const clsTask *pTask, _pmdEDUCB *cb ) ;

      virtual const CHAR* name() const { return CMD_NAME_DROP_INDEX ; }
      virtual BOOLEAN needCheckPrimary() const { return TRUE ; }
      virtual BOOLEAN needCheckDCStatus() const { return TRUE ; }

   protected:
      virtual INT32 _check( _pmdEDUCB *cb ) ;
      virtual INT32 _execute( _pmdEDUCB *cb, rtnContextBuf &ctxBuf ) ;

   private:
      INT32 _buildMainCLTask( _pmdEDUCB *cb,
                              BOOLEAN mainCLIdxNotExist ) ;
      INT32 _buildCLTask( _pmdEDUCB *cb,
                          const CHAR* collectionName,
                          UINT64 mainTaskID = CLS_INVALID_TASKID,
                          UINT64* pTaskID = NULL ) ;
      INT32 _checkIndexExist( const CHAR* collection,
                              const CHAR* indexName,
                              _pmdEDUCB *cb ) ;

   private:
      BOOLEAN _ignoreIdxNotExist ;
   } ;
   typedef _catCMDDropIndex catCMDDropIndex ;

   /*
      catCMDCopyIndex define
   */
   class _catCMDCopyIndex : public _catCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   protected:
      typedef ossPoolVector<catCMDCreateIndex*>           VEC_CMD ;
      typedef ossPoolVector<catCMDCreateIndex*>::iterator VEC_CMD_IT ;

   public:
      _catCMDCopyIndex() ;
      virtual ~_catCMDCopyIndex() ;

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          rtnContextBuf &ctxBuf,
                          INT64 &contextID ) ;

      virtual const CHAR *name() const { return CMD_NAME_COPY_INDEX ; }
      virtual const CHAR *getProcessName() const { return _pCollection ; }
      virtual BOOLEAN needCheckPrimary() const { return TRUE ; }
      virtual BOOLEAN needCheckDCStatus() const { return TRUE ; }

   private:
      INT32 _check( _pmdEDUCB *cb ) ;
      INT32 _execute( _pmdEDUCB *cb ) ;
      INT32 _makeReply( UINT64 taskID, rtnContextBuf &ctxBuf ) ;

      INT32 _checkMainSubCL( _pmdEDUCB *cb,
                             BSONObj& boCollection ) ;
      INT32 _buildCommand( const CHAR *collectionName,
                           const BSONObj &indexDef,
                           _pmdEDUCB *cb ) ;
      INT32 _buildCommands( _pmdEDUCB *cb ) ;

   protected:
      clsCatalogSet*            _pCataSet ;

      const CHAR*               _pCollection ;
      const CHAR*               _pSubCollection ;
      const CHAR*               _pIndexName ;

      ossPoolSet<ossPoolString> _subCLSet ;
      ossPoolSet<ossPoolString> _indexSet ;
      ossPoolSet<ossPoolString> _groupSet ;

      clsCopyIdxTask*           _pMainTask ;
      VEC_CMD                   _commandList ;
      ossPoolVector<BSONObj>    _matcherList ;
   };
   typedef _catCMDCopyIndex catCMDCopyIndex ;

   /*
      catCMDReportTaskProgress define
   */
   class _catCMDReportTaskProgress : public _catCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDReportTaskProgress() {}
      virtual ~_catCMDReportTaskProgress() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 ) ;
      virtual INT32 doit( _pmdEDUCB *cb,
                          rtnContextBuf &ctxBuf,
                          INT64 &contextID ) ;

      virtual const CHAR* name() const
      {
         return CMD_NAME_REPORT_TASK_PROGRESS ;
      }

      virtual BOOLEAN needCheckPrimary() const  { return TRUE ; }
      virtual BOOLEAN needCheckDCStatus() const { return FALSE ; }

   private:
      INT32 _updateTaskProgress( UINT64 taskID,
                                 _pmdEDUCB *cb,
                                 CLS_TASK_STATUS &status ) ;
      INT32 _updateMainTaskProgress( clsTask *pSubTask,
                                     _pmdEDUCB *cb ) ;

   protected:
      BSONObj _query ;
   };
   typedef _catCMDReportTaskProgress catCMDReportTaskProgress ;

   /*
      _catCMDCreateRole define
   */
   class _catCMDCreateRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDCreateRole() {}
      virtual ~_catCMDCreateRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_CREATE_ROLE;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDCreateRole catCMDCreateRole;

   /*
      _catCMDDropRole define
   */
   class _catCMDDropRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDDropRole() {}
      virtual ~_catCMDDropRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_DROP_ROLE;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDDropRole catCMDDropRole;

   /*
      _catCMDGetRole define
   */
   class _catCMDGetRole : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDGetRole() {}
      virtual ~_catCMDGetRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_GET_ROLE;
      }

   protected:
      BSONObj _query;
   };

   /*
      _catCMDListRoles define
   */
   class _catCMDListRoles : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDListRoles() {}
      virtual ~_catCMDListRoles() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_LIST_ROLES;
      }

   protected:
      BSONObj _query;
   };

   /*
      _catCMDUpdateRole define
   */
   class _catCMDUpdateRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDUpdateRole() {}
      virtual ~_catCMDUpdateRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_UPDATE_ROLE;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDUpdateRole catCMDUpdateRole;

   /*
      _catCMDGrantPrivilegesToRole define
   */
   class _catCMDGrantPrivilegesToRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDGrantPrivilegesToRole() {}
      virtual ~_catCMDGrantPrivilegesToRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_GRANT_PRIVILEGES;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDGrantPrivilegesToRole catCMDGrantPrivilegesToRole;

   /*
      _catCMDRevokePrivilegesFromRole define
   */
   class _catCMDRevokePrivilegesFromRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDRevokePrivilegesFromRole() {}
      virtual ~_catCMDRevokePrivilegesFromRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_REVOKE_PRIVILEGES;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDRevokePrivilegesFromRole catCMDRevokePrivilegesFromRole;

   /*
      _catCMDGrantRolesToRole define
   */
   class _catCMDGrantRolesToRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDGrantRolesToRole() {}
      virtual ~_catCMDGrantRolesToRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_GRANT_ROLES_TO_ROLE;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDGrantRolesToRole catCMDGrantRolesToRole;

   /*
      _catCMDRevokeRolesFromRole define
   */
   class _catCMDRevokeRolesFromRole : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDRevokeRolesFromRole() {}
      virtual ~_catCMDRevokeRolesFromRole() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_REVOKE_ROLES_FROM_ROLE;
      }

   protected:
      BSONObj _query;
   };

   /*
      _catCMDGrantRolesToUser define
   */
   class _catCMDGrantRolesToUser : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDGrantRolesToUser() {}
      virtual ~_catCMDGrantRolesToUser() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_GRANT_ROLES_TO_USER;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDGrantRolesToUser catCMDGrantRolesToUser;

   /*
      _catCMDRevokeRolesFromUser define
   */
   class _catCMDRevokeRolesFromUser : public _catWriteCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDRevokeRolesFromUser() {}
      virtual ~_catCMDRevokeRolesFromUser() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_REVOKE_ROLES_FROM_USER;
      }

   protected:
      BSONObj _query;
   };
   typedef _catCMDRevokeRolesFromUser catCMDRevokeRolesFromUser;

   /*
      _catCMDGetUser define
   */
   class _catCMDGetUser : public _catReadCMDBase
   {
      CAT_DECLARE_CMD_AUTO_REGISTER()
   public:
      _catCMDGetUser() {}
      virtual ~_catCMDGetUser() {}

      virtual INT32 init( const CHAR *pQuery,
                          const CHAR *pSelector = NULL,
                          const CHAR *pOrderBy = NULL,
                          const CHAR *pHint = NULL,
                          INT32 flags = 0,
                          INT64 numToSkip = 0,
                          INT64 numToReturn = -1 );
      virtual INT32 doit( _pmdEDUCB *cb, rtnContextBuf &ctxBuf, INT64 &contextID );

      virtual const CHAR *name() const
      {
         return CMD_NAME_GET_USER;
      }

   protected:
      BSONObj _query;
   };
}

#endif /* CAT_COMMAND_HPP__ */


