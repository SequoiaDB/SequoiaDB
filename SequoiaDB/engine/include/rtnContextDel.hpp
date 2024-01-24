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

   Source File Name = rtnContextDel.hpp

   Descriptive Name = RunTime Delete Operation Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_DEL_HPP_
#define RTN_CONTEXT_DEL_HPP_

#include "rtnContext.hpp"
#include "utilRenameLogger.hpp"
#include "rtnLocalTaskFactory.hpp"
#include "clsCatalogAgent.hpp"
#include "dmsEventHandler.hpp"
#include "utilRecycleItem.hpp"

namespace engine
{

   // forward declare
   class _clsCatalogAgent ;
   class _clsFreezingWindow ;
   class dpsTransCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   typedef ossPoolMap<std::string, SINT64>  SUBCL_CONTEXT_LIST ;

   /*
      _rtnContextDelCS define
   */
   class _rtnContextDelCS : public _rtnContextBase
   {
      enum delCSPhase
      {
         DELCSPHASE_0 = 0,
         DELCSPHASE_1
      };
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextDelCS )
   public:
      _rtnContextDelCS( SINT64 contextID, UINT64 eduID ) ;
      ~_rtnContextDelCS();
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual UINT32 getSULogicalID () const
      {
         // must return invalid, to avoid killing self in preDelContext
         return DMS_INVALID_LOGICCSID ;
      }
      virtual BOOLEAN          isWrite() const { return TRUE ; }

      virtual const CHAR *     getProcessName() const
      {
         return _name ;
      }

      INT32 open( const CHAR *pCollectionName,
                  const utilRecycleItem *recycleItem,
                  _pmdEDUCB *cb );

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _tryLock( const CHAR *pCollectionSpaceName,
                      const utilRecycleItem *recycleItem,
                      _pmdEDUCB *cb );

      INT32 _releaseLock( _pmdEDUCB *cb );

      void _clean( _pmdEDUCB *cb );

   private:
      delCSPhase           _status;
      _SDB_DMSCB            *_pDmsCB;
      dpsTransCB           *_pTransCB;
      _clsCatalogAgent     *_pCatAgent;
      CHAR                 _name[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ];
      BOOLEAN              _gotDmsCBWrite;
      BOOLEAN              _gotTransLock ;

      dmsEventSUItem       _eventItem ;
      dmsDropCSOptions     _options ;
   };
   typedef class _rtnContextDelCS rtnContextDelCS;

   /*
      _rtnContextDelCL define
   */
   class _rtnContextDelCL : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextDelCL )
   public:
      _rtnContextDelCL( SINT64 contextID, UINT64 eduID );
      ~_rtnContextDelCL();
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }
      virtual const CHAR *     getProcessName() const
      {
         return _collectionName ;
      }

      INT32 open( const CHAR *pCollectionName,
                  const utilRecycleItem *recycleItem,
                  _pmdEDUCB *cb,
                  INT16 w ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _tryLock( const CHAR *pCollectionName,
                      const utilRecycleItem *recycleItem,
                      _pmdEDUCB *cb );

      INT32 _releaseLock( _pmdEDUCB *cb );

      void _clean( _pmdEDUCB *cb );

   private:
      _SDB_DMSCB           *_pDmsCB;
      _clsCatalogAgent     *_pCatAgent;
      dpsTransCB           *_pTransCB;
      CHAR                 _collectionName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      const CHAR           *_clShortName ;
      BOOLEAN              _gotDmsCBWrite ;
      BOOLEAN              _hasLock ;
      BOOLEAN              _hasDropped ;

      _dmsStorageUnit      *_su ;
      _dmsMBContext        *_mbContext ;

      dmsEventCLItem       _eventItem ;
      dmsDropCLOptions     _options ;
   };
   typedef class _rtnContextDelCL rtnContextDelCL;

   /*
      _rtnContextDelMainCL define
   */
   class _rtnContextDelMainCL : public _rtnContextBase
   {
      typedef ossPoolMap< std::string, SINT64>  SUBCL_CONTEXT_LIST ;
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextDelMainCL )
   public:
      _rtnContextDelMainCL( SINT64 contextID, UINT64 eduID );
      ~_rtnContextDelMainCL();
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }

      virtual const CHAR *     getProcessName() const
      {
         return _name ;
      }

      INT32 open( const CHAR *pCollectionName,
                  CLS_SUBCL_LIST &subCLList,
                  const utilRecycleItem *recycleItem,
                  _pmdEDUCB *cb,
                  INT16 w ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      void _clean( _pmdEDUCB *cb );

   private:
      _clsCatalogAgent           *_pCatAgent;
      _SDB_RTNCB                 *_pRtncb;
      CHAR                       _name[ DMS_COLLECTION_FULL_NAME_SZ + 1 ];
      SUBCL_CONTEXT_LIST         _subContextList ;
      BOOLEAN                    _lockDms ;

   };
   typedef class _rtnContextDelMainCL rtnContextDelMainCL ;

   #define RTN_RENAME_BLOCKWRITE_INTERAL ( 0.1 * OSS_ONE_SEC )
   #define RTN_RENAME_BLOCKWRITE_TIMES   ( 30 )

   class _rtnLocalTaskMgr ;
   /*
      _rtnContextRenameCS define
   */
   class _rtnContextRenameCS : public _rtnContextBase
   {
   protected:
      enum renameCSPhase
      {
         RENAMECSPHASE_0 = 0,
         RENAMECSPHASE_1
      } ;
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextRenameCS )
   public:
      _rtnContextRenameCS( SINT64 contextID, UINT64 eduID ) ;
      ~_rtnContextRenameCS();
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }
      virtual const CHAR *     getProcessName() const
      {
         return _oldName ;
      }

      INT32 open( const CHAR *pCSName, const CHAR *pNewCSName,
                  _pmdEDUCB *cb, BOOLEAN useLocalTask = TRUE,
                  BOOLEAN allowOldSYS = FALSE,
                  BOOLEAN allowNewSYS = FALSE );

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   protected:
      virtual INT32 _tryLock( const CHAR *pCSName,
                              _pmdEDUCB *cb ) ;
      virtual INT32 _releaseLock( _pmdEDUCB *cb ) ;
      virtual INT32 _doRenameP1( _pmdEDUCB *cb ) ;
      virtual INT32 _doRename( _pmdEDUCB *cb ) ;
      virtual INT32 _cancelRename( _pmdEDUCB *cb ) ;
      virtual INT32 _initLocalTask( rtnLocalTaskPtr &taskPtr,
                                    const CHAR *oldName,
                                    const CHAR *newName ) ;

      virtual RTN_LOCAL_TASK_TYPE _getLocakTaskType() const
      {
         return RTN_LOCAL_TASK_RENAMECS ;
      }

      virtual BOOLEAN _flagAllowNewSYS() const
      {
         return FALSE ;
      }

      virtual BOOLEAN _flagAllowOldSYS() const
      {
         return FALSE ;
      }

      virtual BOOLEAN _flagLockExclusive() const
      {
         return FALSE ;
      }

   protected:
      _SDB_DMSCB           *_pDmsCB ;
      dpsTransCB           *_pTransCB ;
      _clsCatalogAgent     *_pCatAgent ;
      _clsFreezingWindow   *_pFreezingWnd ;
      _rtnLocalTaskMgr     *_pLTMgr ;
      CHAR                 _oldName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      CHAR                 _newName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      UINT64               _blockID ;
      BOOLEAN              _lockDms ;
      UINT32               _logicCSID ;
      renameCSPhase        _status ;
      utilRenameLogger     _logger ;
      BOOLEAN              _skipGetMore ;
      rtnLocalTaskPtr      _taskPtr ;
   };
   typedef class _rtnContextRenameCS rtnContextRenameCS ;

   /*
      _rtnContextRenameCL define
   */
   class _rtnContextRenameCL : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextRenameCL )
   public:
      _rtnContextRenameCL( SINT64 contextID, UINT64 eduID ) ;
      ~_rtnContextRenameCL();
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }
      virtual const CHAR *     getProcessName() const
      {
         return _clFullName ;
      }

      INT32 open( const CHAR *csName, const CHAR *clShortName,
                  const CHAR *newCLShortName,
                  _pmdEDUCB *cb, INT16 w = 1,
                  BOOLEAN useLocalTask = TRUE ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   protected:
      virtual INT32 _tryLock( const CHAR *pCSName,
                              _pmdEDUCB *cb ) ;
      virtual INT32 _releaseLock( _pmdEDUCB *cb ) ;
      virtual INT32 _doRename( _pmdEDUCB *cb ) ;
      virtual INT32 _initLocalTask( rtnLocalTaskPtr &taskPtr,
                                    const CHAR *oldName,
                                    const CHAR *newName ) ;

      virtual RTN_LOCAL_TASK_TYPE _getLocakTaskType() const
      {
         return RTN_LOCAL_TASK_RENAMECL ;
      }

      virtual BOOLEAN _flagAllowNewSYS() const
      {
         return FALSE ;
      }

      virtual BOOLEAN _flagAllowOldSYS() const
      {
         return FALSE ;
      }

      virtual BOOLEAN _flagLockExclusive() const
      {
         return FALSE ;
      }

      virtual BOOLEAN _flagAllowNewExist() const
      {
         return FALSE ;
      }

   protected:
      _SDB_DMSCB           *_pDmsCB ;
      _clsCatalogAgent     *_pCatAgent ;
      _clsFreezingWindow   *_pFreezingWnd ;
      dpsTransCB           *_pTransCB ;
      _rtnLocalTaskMgr     *_pLTMgr ;

      CHAR                 _clFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      CHAR                 _newCLFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      CHAR                 _clShortName[ DMS_COLLECTION_NAME_SZ + 1 ] ;
      CHAR                 _newCLShortName[ DMS_COLLECTION_NAME_SZ + 1 ] ;

      UINT64               _blockID ;
      BOOLEAN              _lockDms ;
      _dmsStorageUnit      *_su ;
      UINT16               _mbID ;
      BOOLEAN              _skipGetMore ;
      rtnLocalTaskPtr      _taskPtr ;
   };
   typedef class _rtnContextRenameCL rtnContextRenameCL ;

   /*
      _rtnContextRenameMainCL define
   */
   class _rtnContextRenameMainCL : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextRenameMainCL )
   public:
      _rtnContextRenameMainCL( SINT64 contextID, UINT64 eduID );
      ~_rtnContextRenameMainCL();
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }
      virtual const CHAR *     getProcessName() const { return _name ; }

      INT32 open( const CHAR *pCollectionName, _pmdEDUCB *cb, INT16 w ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      void _clean( _pmdEDUCB *cb );

   private:
      _SDB_DMSCB                 *_pDmsCB ;
      _clsCatalogAgent           *_pCatAgent;
      CHAR                       _name[ DMS_COLLECTION_FULL_NAME_SZ + 1 ];
      BOOLEAN                    _lockDms ;

   };
   typedef class _rtnContextRenameMainCL rtnContextRenameMainCL ;

   /*
      rtnContextTruncateCL define
   */
   class _rtnContextTruncateCL : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextTruncateCL ) ;
   public:
      _rtnContextTruncateCL( SINT64 contextID, UINT64 eduID ) ;
      ~_rtnContextTruncateCL() ;
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const ;
      virtual _dmsStorageUnit* getSU () { return _su ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }
      virtual const CHAR *     getProcessName() const
      {
         return _collectionName ;
      }

      INT32 open( const CHAR *pCollectionName,
                  const utilRecycleItem *recycleItem,
                  _pmdEDUCB *cb,
                  INT16 w ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _tryLock( const CHAR *pCollectionName,
                      const utilRecycleItem *recycleItem,
                      _pmdEDUCB *cb ) ;
      INT32 _releaseLock( _pmdEDUCB *cb ) ;
      void _clean( _pmdEDUCB *cb ) ;

   private:
      _SDB_DMSCB           *_pDmsCB;
      _clsCatalogAgent     *_pCatAgent;
      dpsTransCB           *_pTransCB;
      CHAR                 _collectionName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      const CHAR           *_clShortName ;
      BOOLEAN              _gotDmsCBWrite ;
      BOOLEAN              _hasTransLockCL ;

      _dmsStorageUnit      *_su ;
      _dmsMBContext        *_mbContext ;

      dmsEventCLItem       _eventItem ;
      dmsTruncCLOptions    _options ;
   };
   typedef class _rtnContextTruncateCL rtnContextTruncateCL ;

   /*
      _rtnContextTruncMainCL define
   */
   class _rtnContextTruncMainCL : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextTruncMainCL )
   public:
      _rtnContextTruncMainCL( SINT64 contextID, UINT64 eduID ) ;
      ~_rtnContextTruncMainCL() ;
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const ;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }
      virtual const CHAR *     getProcessName() const
      {
         return _name ;
      }

      INT32 open( const CHAR *pCollectionName,
                  CLS_SUBCL_LIST &subCLList,
                  const utilRecycleItem *recycleItem,
                  _pmdEDUCB *cb,
                  INT16 w ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      void _clean( _pmdEDUCB *cb ) ;

   private:
      _clsCatalogAgent *   _cataAgent ;
      _SDB_RTNCB *         _rtnCB ;
      CHAR                 _name[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      SUBCL_CONTEXT_LIST   _subContextList ;
      BOOLEAN              _lockDms ;

   };
   typedef class _rtnContextTruncMainCL rtnContextTruncMainCL ;

}

#endif /* RTN_CONTEXT_DEL_HPP_ */

