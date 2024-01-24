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

   Source File Name = rtnContextRecycle.hpp

   Descriptive Name = RunTime Recycle Operation Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/01/2021  HGM initial version

   Last Changed =

*******************************************************************************/

#ifndef RTN_CONTEXT_RECY_HPP_
#define RTN_CONTEXT_RECY_HPP_

#include "rtnContextDel.hpp"
#include "clsRecycleBinManager.hpp"
#include "utilRecycleItem.hpp"
#include "utilRecycleReturnInfo.hpp"

namespace engine
{

   /*
      _rtnCtxReturnHelper define
    */
   class _rtnCtxReturnHelper
   {
   protected:
      _rtnCtxReturnHelper() ;
      ~_rtnCtxReturnHelper() ;

      INT32 _deleteItem( pmdEDUCB *cb ) ;

   protected:
      dmsReturnOptions        _returnOptions ;
      clsRecycleBinManager *  _recycleBinMgr ;
   } ;

   typedef class _rtnCtxReturnHelper rtnCtxReturnHelper ;

   /*
      _rtnContextReturnCL define
    */
   class _rtnContextReturnCL : public _rtnContextRenameCL,
                               public _rtnCtxReturnHelper
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextReturnCL )

   protected:
      typedef class _rtnContextRenameCL _BASE ;

   public:
      _rtnContextReturnCL( SINT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextReturnCL() ;

      INT32 open( const utilRecycleItem &recycleItem,
                  const utilRecycleReturnInfo &returnInfo,
                  _pmdEDUCB *cb,
                  INT16 w = 1 ) ;

      virtual const CHAR *name() const
      {
         return "RETURNCL" ;
      }

      virtual RTN_CONTEXT_TYPE getType() const
      {
         return RTN_CONTEXT_RETURNCL ;
      }

   protected:
      virtual void  _toString( stringstream &ss ) ;

      virtual INT32 _tryLock( const CHAR *csName,
                              _pmdEDUCB *cb ) ;
      virtual INT32 _releaseLock( _pmdEDUCB *cb ) ;
      virtual INT32 _doRename( _pmdEDUCB *cb ) ;
      virtual INT32 _initLocalTask( rtnLocalTaskPtr &taskPtr,
                                    const CHAR *oldName,
                                    const CHAR *newName ) ;

      virtual RTN_LOCAL_TASK_TYPE _getLocakTaskType() const
      {
         return RTN_LOCAL_TASK_RETURNCL ;
      }

      virtual BOOLEAN _flagAllowOldSYS() const
      {
         return TRUE ;
      }

      virtual BOOLEAN _flagLockExclusive() const
      {
         return TRUE ;
      }

      virtual BOOLEAN _flagAllowNewExist() const
      {
         return _returnOptions._recycleItem.isTruncate() ;
      }

   protected:
      UINT64 _returnBlockID ;
   } ;

   typedef class _rtnContextReturnCL rtnContextReturnCL ;

   /*
      _rtnContextReturnCS define
    */
   class _rtnContextReturnCS : public _rtnContextRenameCS,
                               public _rtnCtxReturnHelper
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextReturnCS )

   protected:
      typedef class _rtnContextRenameCS _BASE ;

   public:
      _rtnContextReturnCS( SINT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextReturnCS() ;

      INT32 open( const utilRecycleItem &recycleItem,
                  const utilRecycleReturnInfo &returnInfo,
                  _pmdEDUCB *cb,
                  INT16 w = 1 ) ;

      virtual const CHAR *name() const
      {
         return "RETURNCS" ;
      }

      virtual RTN_CONTEXT_TYPE getType() const
      {
         return RTN_CONTEXT_RETURNCS ;
      }

   protected:
      virtual void  _toString( stringstream &ss ) ;
      virtual INT32 _tryLock( const CHAR *csName,
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
         return RTN_LOCAL_TASK_RETURNCS ;
      }

      virtual BOOLEAN _flagAllowOldSYS() const
      {
         return TRUE ;
      }

      virtual BOOLEAN _flagLockExclusive() const
      {
         return TRUE ;
      }

   protected:
      UINT64 _returnBlockID ;
   } ;

   typedef class _rtnContextReturnCS rtnContextReturnCS ;

   /*
      _rtnContextReturnMainCL define
    */
   class _rtnContextReturnMainCL : public _rtnContextBase,
                                   public _rtnCtxReturnHelper
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextReturnMainCL )
   public:
      _rtnContextReturnMainCL( SINT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextReturnMainCL() ;

      virtual const CHAR *name() const
      {
         return "RETURNMAINCL" ;
      }

      virtual RTN_CONTEXT_TYPE getType() const
      {
         return RTN_CONTEXT_RETURNMAINCL ;
      }

      virtual _dmsStorageUnit *getSU()
      {
         return NULL ;
      }

      virtual BOOLEAN isWrite() const
      {
         return TRUE ;
      }

      virtual const CHAR *getProcessName() const
      {
         return _recycleFullName ;
      }

      INT32 open( const utilRecycleItem &recycleItem,
                  const utilRecycleReturnInfo &returnInfo,
                  _pmdEDUCB *cb,
                  INT16 w ) ;

   protected:
      virtual void  _toString( stringstream &ss ) ;
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;

      INT32 _openSubContext( const utilRecycleItem &subItem,
                             const utilRecycleReturnInfo &returnInfo,
                             _pmdEDUCB *cb,
                             INT32 w ) ;
      void _clean( _pmdEDUCB *cb ) ;

   protected:
      _SDB_DMSCB *         _dmsCB ;
      _SDB_RTNCB *         _rtnCB ;
      SUBCL_CONTEXT_LIST   _subContextList ;
      BOOLEAN              _lockDMS ;
      CHAR                 _recycleFullName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
   } ;

   typedef class _rtnContextReturnMainCL rtnContextReturnMainCL ;

}

#endif // RTN_CONTEXT_RECY_HPP_
