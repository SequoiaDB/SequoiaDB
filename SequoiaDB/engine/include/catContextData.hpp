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

   Source File Name = catContextData.hpp

   Descriptive Name = RunTime Context of Catalog Header

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

#ifndef CATCONTEXTDATA_HPP_
#define CATCONTEXTDATA_HPP_

#include "catContext.hpp"

namespace engine
{
   /*
    * _catCtxDataBase define
    */
   class _catCtxDataBase : public _catContextBase
   {
   public :
      _catCtxDataBase ( INT64 contextID, UINT64 eduID ) ;
      virtual ~_catCtxDataBase () ;

   protected :
      virtual INT32 _makeReply ( rtnContextBuf &buffObj ) ;

      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb, INT16 w )
      { return SDB_OK ; }

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w )
      { return SDB_OK ; }
      virtual INT32 _clearInternal(  _pmdEDUCB *cb, INT16 w  )
      { return SDB_OK ; }

   protected :
      std::vector<UINT32> _groupList ;
   } ;

   /*
    * _catCtxDataMultiTaskBase define
    */
   class _catCtxDataMultiTaskBase : public _catCtxDataBase
   {
   public :
      _catCtxDataMultiTaskBase ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxDataMultiTaskBase () ;

   protected :
      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w ) ;

      void _addTask ( _catCtxTaskBase *pCtx, BOOLEAN pushExec ) ;

      INT32 _pushExecTask ( _catCtxTaskBase *pCtx ) ;

   protected :
      _catSubTasks _subTasks ;
      _catSubTasks _execTasks ;
   } ;

   /*
    * _catCtxCLMultiTask define
    */
   class _catCtxCLMultiTask : public _catCtxDataMultiTaskBase
   {
   public :
      _catCtxCLMultiTask ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxCLMultiTask () {}

   protected :
      INT32 _addDropCLTask ( const std::string &clName,
                             INT32 version,
                             _catCtxDropCLTask **ppCtx,
                             BOOLEAN pushExec = TRUE ) ;
   } ;

   /*
    * _catCtxIndexMultiTask define
    */
   class _catCtxIndexMultiTask : public _catCtxDataMultiTaskBase
   {
   public :
      _catCtxIndexMultiTask ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxIndexMultiTask () {}

   protected :
      INT32 _addCreateIdxTask ( const std::string &clName,
                                const std::string &idxName,
                                const BSONObj &boIdx,
                                _catCtxCreateIdxTask **ppCtx,
                                BOOLEAN pushExec = TRUE ) ;

      INT32 _addDropIdxTask ( const std::string &clName,
                              const std::string &idxName,
                              _catCtxDropIdxTask **pCtx,
                              BOOLEAN pushExec = TRUE ) ;

      INT32 _addCreateIdxSubTasks ( _catCtxCreateIdxTask *pCreateIdxTask,
                                    catCtxLockMgr &lockMgr,
                                    _pmdEDUCB *cb ) ;

      INT32 _addCreateIdxTasks ( const std::string &clName,
                                 const std::string &idxName,
                                 const BSONObj &boIdx,
                                 BOOLEAN uniqueCheck,
                                 _pmdEDUCB *cb ) ;

      INT32 _addDropIdxSubTasks ( _catCtxDropIdxTask *pDropIdxTask,
                                  catCtxLockMgr &lockMgr,
                                  _pmdEDUCB *cb ) ;

      INT32 _addDropIdxTasks ( const std::string &clName,
                               const std::string &idxName,
                               _pmdEDUCB *cb ) ;
   } ;

   /*
    * _catCtxDropCS define
    */
   class _catCtxDropCS : public _catCtxCLMultiTask
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public :
      _catCtxDropCS ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxDropCS () ;

      virtual const CHAR* name() const
      {
         return "CAT_DROP_CS" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_DROP_CS ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      INT32 _addDropCSTask ( const std::string &csName,
                             _catCtxDropCSTask **ppCtx,
                             BOOLEAN pushExec = TRUE ) ;

      INT32 _addDropCSSubTasks ( _catCtxDropCSTask *pDropCSTask,
                                 _pmdEDUCB *cb ) ;

      INT32 _addDropCLSubTasks ( _catCtxDropCLTask *pDropCLTask,
                                 _pmdEDUCB *cb,
                                 std::set<std::string> &externalMainCL ) ;

      INT32 _addUnlinkCSTask ( const std::string &csName,
                               _catCtxUnlinkCSTask **ppCtx,
                               BOOLEAN pushExec = TRUE ) ;

      INT32 _addUnlinkSubCLTask ( const std::string &mainCLName,
                                  const std::string &subCLName,
                                  _catCtxUnlinkSubCLTask **ppCtx,
                                  BOOLEAN pushExec = TRUE ) ;
   } ;

   typedef class _catCtxDropCS catCtxDropCS ;

   /*
      _catCtxRenameCS define
    */
   class _catCtxRenameCS : public _catCtxDataBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()

      public :
         _catCtxRenameCS ( INT64 contextID, UINT64 eduID ) ;

         virtual ~_catCtxRenameCS () ;

         virtual const CHAR* name () const
         {
            return "CAT_RENAME_CS" ;
         }

         virtual RTN_CONTEXT_TYPE getType () const
         {
            return RTN_CONTEXT_CAT_RENAME_CS ;
         }

      protected :
         virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

         virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

         virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      protected :
         std::string _newCSName ;
         BSONObj _boCollectionspace ;
   } ;

   typedef class _catCtxRenameCS catCtxRenameCS ;

   /*
      _catCtxAlterCS define
    */
   class _catCtxAlterCS : public _catCtxDataMultiTaskBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()

      public :
         _catCtxAlterCS ( INT64 contextID, UINT64 eduID ) ;

         virtual ~_catCtxAlterCS () ;

         virtual const CHAR* name () const
         {
            return "CAT_ALTER_CS" ;
         }

         virtual RTN_CONTEXT_TYPE getType () const
         {
            return RTN_CONTEXT_CAT_ALTER_CS ;
         }

      protected :
         INT32 _parseQuery ( _pmdEDUCB * cb ) ;
         INT32 _checkInternal ( _pmdEDUCB * cb ) ;
         INT32 _addAlterTask ( const string & collectionSpace,
                               const rtnAlterTask * task,
                               catCtxAlterCSTask ** catTask,
                               BOOLEAN pushExec ) ;

         INT32 _checkAlterTask ( const rtnAlterTask * task, _pmdEDUCB * cb ) ;

      protected :
         rtnAlterJob _alterJob ;
   } ;

   /*
    * _catCtxCreateCL define
    */
   class _catCtxCreateCL : public _catCtxDataBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public :
      _catCtxCreateCL ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxCreateCL () ;

      virtual const CHAR* name() const
      {
         return "CAT_CREATE_CL" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_CREATE_CL ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w ) ;

   protected :
      INT32 _combineOptions ( const BSONObj &boDomain,
                              const BSONObj &boSpace,
                              UINT32 &fieldMask,
                              catCollectionInfo &clInfo ) ;

      INT32 _chooseGroupOfCl ( const BSONObj &domainObj,
                               const BSONObj &csObj,
                               const catCollectionInfo &clInfo,
                               _pmdEDUCB *cb,
                               std::vector<UINT32> &groupIDList,
                               std::map<std::string, UINT32> &splitRange ) ;

      INT32 _getBoundFromClObj ( const BSONObj &clObj, UINT32 &totalBound ) ;

   private :
      INT32 _chooseCLGroupBySpec ( const CHAR * groupName,
                                   const BSONObj & domainObj,
                                   _pmdEDUCB * cb,
                                   std::vector<UINT32> & groupIDList ) ;

      INT32 _chooseCLGroupAutoSplit ( const BSONObj & domainObj,
                                      std::vector<UINT32> & groupIDList,
                                      std::map<std::string, UINT32> & splitRange ) ;

      INT32 _chooseCLGroupDefault ( const BSONObj & domainObj,
                                    const BSONObj & csObj,
                                    INT32 assignType,
                                    _pmdEDUCB * cb,
                                    std::vector<UINT32> & groupIDList ) ;

   private :
      utilCLUniqueID                _clUniqueID ;
      catCollectionInfo             _clInfo ;
      std::map<std::string, UINT32> _splitList ;
      UINT32                        _fieldMask ;
      vector<BSONObj>               _autoIncOptArr ;
   } ;

   typedef class _catCtxCreateCL catCtxCreateCL ;

   /*
    * _catCtxDropCL define
    */
   class _catCtxDropCL : public _catCtxCLMultiTask
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public :
      _catCtxDropCL ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxDropCL () ;

      virtual const CHAR* name() const
      {
         return "CAT_DROP_CL" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_DROP_CL ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _makeReply ( rtnContextBuf &buffObj ) ;

      INT32 _addDropCLSubTasks ( _catCtxDropCLTask *pDropCLTask,
                                 _pmdEDUCB *cb ) ;

      INT32 _addUnlinkMainCLTask ( const std::string &mainCLName,
                                   const std::string &subCLName,
                                   _catCtxUnlinkMainCLTask **ppCtx,
                                   BOOLEAN pushExec = TRUE ) ;

      INT32 _addDelCLsFromCSTask ( _catCtxDelCLsFromCSTask **ppCtx,
                                   BOOLEAN pushExec = TRUE ) ;

   protected :
      INT32 _needUpdateCoord ;
   } ;

   typedef class _catCtxDropCL catCtxDropCL ;

   /*
      _catCtxRenameCL define
    */
   class _catCtxRenameCL : public _catCtxDataBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()

      public :
         _catCtxRenameCL ( INT64 contextID, UINT64 eduID ) ;

         virtual ~_catCtxRenameCL () ;

         virtual const CHAR* name () const
         {
            return "CAT_RENAME_CL" ;
         }

         virtual RTN_CONTEXT_TYPE getType () const
         {
            return RTN_CONTEXT_CAT_RENAME_CL ;
         }

      protected :
         virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

         virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

         virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      protected :
         std::string _newCLFullName ;
   } ;

   typedef class _catCtxRenameCL catCtxRenameCL ;

   /*
    * _catCtxAlterCL define
    */
   class _catCtxAlterCL : public _catCtxIndexMultiTask
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public :
      _catCtxAlterCL ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxAlterCL () ;

      virtual const CHAR* name() const
      {
         return "CAT_ALTER_CL" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_ALTER_CL ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;
      virtual INT32 _clearInternal(_pmdEDUCB * cb,INT16 w) ;

   protected :
      INT32 _checkAlterTask ( const rtnAlterTask * task,
                              _pmdEDUCB * cb ) ;
      INT32 _executeAlterTask ( const rtnAlterTask * task,
                                _pmdEDUCB * cb,
                                INT16 w ) ;
      INT32 _clearAlterTask ( const rtnAlterTask * task,
                              _pmdEDUCB * cb,
                              INT16 w ) ;
      INT32 _addAlterTask ( const string & collection,
                            const rtnAlterTask * task,
                            catCtxAlterCLTask ** catTask,
                            BOOLEAN pushExec ) ;
      INT32 _addAlterSubCLTask ( catCtxAlterCLTask * catTask,
                                 pmdEDUCB * cb,
                                 catCtxLockMgr & lockMgr,
                                 std::set< std::string > & collectionSet,
                                 std::vector< UINT32 > & groupList ) ;
      INT32 _addSequenceTask( const string & collection,
                              const rtnAlterTask * task,
                              catCtxTaskBase ** catAutoIncTask ) ;
      INT32 _addCreateSeqenceTask( const string & collection,
                                   const rtnAlterTask * task,
                                   catCtxTaskBase ** catAutoIncTask ) ;
      INT32 _addDropSeqenceTask( const string & collection,
                                 const rtnAlterTask * task,
                                 catCtxTaskBase ** catAutoIncTask ) ;
      INT32 _addAlterSeqenceTask( const string & collection,
                                  const rtnAlterTask * task,
                                  catCtxTaskBase ** catAutoIncTask ) ;

      virtual INT32 _makeReply ( rtnContextBuf &buffObj ) ;

   protected :
      rtnAlterJob _alterJob ;
   } ;

   typedef class _catCtxAlterCL catCtxAlterCL ;

   /*
    * _catCtxLinkCL define
    */
   class _catCtxLinkCL : public _catCtxDataBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _catCtxLinkCL ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxLinkCL () ;

      virtual const CHAR* name() const
      {
         return "CAT_LINK_CL" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_LINK_CL ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w ) ;

   protected :
      BOOLEAN _needUpdateSubCL ;
      std::string _subCLName ;
      BSONObj _boSubCL ;
      BSONObj _lowBound ;
      BSONObj _upBound ;
   } ;

   typedef class _catCtxLinkCL catCtxLinkCL ;

   /*
    * _catCtxUnlinkCL define
    */
   class _catCtxUnlinkCL : public _catCtxDataBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _catCtxUnlinkCL ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxUnlinkCL () ;

      virtual const CHAR* name() const
      {
         return "CAT_UNLINK_CL" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_UNLINK_CL ;
      }

   protected :
      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w ) ;

   protected :
      BOOLEAN _needUpdateSubCL ;
      std::string _subCLName ;
      BSONObj _boSubCL ;
      BSONObj _lowBound ;
      BSONObj _upBound ;
   } ;

   typedef class _catCtxUnlinkCL catCtxUnlinkCL ;

   /*
    * _catCtxCreateIdx define
    */
   class _catCtxCreateIdx : public _catCtxIndexMultiTask
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public :
      _catCtxCreateIdx ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxCreateIdx () ;

      virtual const CHAR* name() const
      {
         return "CAT_CREATE_IDX" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_CREATE_IDX ;
      }

      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

   protected :
      std::string _idxName ;
      BSONObj _boIdx ;
   } ;

   typedef class _catCtxCreateIdx catCtxCreateIdx ;

   /*
    * _catCtxDropIdx define
    */
   class _catCtxDropIdx : public _catCtxIndexMultiTask
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public :
      _catCtxDropIdx ( INT64 contextID, UINT64 eduID ) ;

      virtual ~_catCtxDropIdx () ;

      virtual const CHAR* name() const
      {
         return "CAT_DROP_IDX" ;
      }

      virtual RTN_CONTEXT_TYPE getType () const
      {
         return RTN_CONTEXT_CAT_DROP_IDX ;
      }

      virtual INT32 _parseQuery ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) ;

   protected :
      std::string _idxName ;
   } ;

   typedef class _catCtxDropIdx catCtxDropIdx ;
}

#endif //CATCONTEXTDATA_HPP_

