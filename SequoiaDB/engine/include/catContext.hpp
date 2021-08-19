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

   Source File Name = catContext.hpp

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

#ifndef CATCONTEXT_HPP_
#define CATCONTEXT_HPP_

#include "rtnContext.hpp"
#include "catLevelLock.hpp"
#include "catalogueCB.hpp"
#include "catContextTask.hpp"
#include "catContextAlterTask.hpp"

namespace engine
{
   class sdbCatalogueCB ;

   /*
    * _catContextBase define
    */
   class _catContextBase : public _rtnContextBase
   {
   public:
      enum CAT_CONTEXT_STATUS
      {
         CAT_CONTEXT_NEW = 1,
         CAT_CONTEXT_LOCKING,
         CAT_CONTEXT_READY,
         CAT_CONTEXT_PREEXECUTED,
         CAT_CONTEXT_CAT_DONE,
         CAT_CONTEXT_CAT_ERROR,
         CAT_CONTEXT_DATA_DONE,
         CAT_CONTEXT_DATA_ERROR,
         CAT_CONTEXT_CLEANING,
         CAT_CONTEXT_END
      } ;

      typedef std::vector<UINT32> _catGroupList ;
      typedef std::vector<_catCtxTaskBase *> _catSubTasks ;

   public:
      _catContextBase ( INT64 contextID, UINT64 eduID ) ;
      virtual ~_catContextBase () ;

   public:
      // Override functions
      virtual _dmsStorageUnit* getSU () { return NULL ; }

      // Catalog context functions
      CAT_CONTEXT_STATUS getStatus () const { return _status ; }

      virtual INT32 open ( const NET_HANDLE &handle,
                           MsgHeader *pMsg,
                           const CHAR *pQuery,
                           rtnContextBuf &buffObj,
                           _pmdEDUCB *cb ) ;

   protected:
      virtual INT32 _prepareData ( _pmdEDUCB *cb ) ;

   protected:
      virtual void _setStatus ( CAT_CONTEXT_STATUS status ) ;

      virtual INT32 _parseQuery (_pmdEDUCB *cb ) = 0 ;

      virtual INT32 _checkContext ( _pmdEDUCB *cb ) ;

      virtual INT32 _checkInternal ( _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _preExecute ( _pmdEDUCB *cb ) ;

      virtual INT32 _preExecuteInternal ( _pmdEDUCB *cb, INT16 w ) = 0 ;

      virtual INT32 _execute ( _pmdEDUCB *cb ) ;

      virtual INT32 _executeInternal ( _pmdEDUCB *cb, INT16 w ) = 0 ;

      virtual INT32 _commit ( _pmdEDUCB *cb ) ;

      virtual INT32 _rollback ( _pmdEDUCB *cb ) ;

      virtual INT32 _rollbackInternal ( _pmdEDUCB *cb, INT16 w ) = 0 ;

      virtual INT32 _makeReply ( rtnContextBuf &buffObj ) = 0 ;

      virtual INT32 _initQuery ( const NET_HANDLE &handle,
                                 MsgHeader *pMsg,
                                 const CHAR *pQuery,
                                 _pmdEDUCB *cb ) ;
      virtual INT32 _clear ( _pmdEDUCB *cb ) ;
      virtual INT32 _clearInternal(  _pmdEDUCB *cb, INT16 w  ) = 0 ;

      // Must be called in destruction function
      virtual INT32 _onCtxDelete () ;

      void _changeStatusOnError () ;

      virtual void _toString ( stringstream &ss ) ;

   protected:
      _SDB_DMSCB *_pDmsCB ;
      sdbCatalogueCB *_pCatCB ;

      MSG_TYPE _cmdType ;
      INT32 _version ;
      BSONObj _boQuery ;

      CAT_CONTEXT_STATUS _status ;
      catCtxLockMgr _lockMgr ;

      // Flags to control process
      BOOLEAN _executeAfterLock ;
      BOOLEAN _commitAfterExecute ;
      BOOLEAN _needPreExecute ;
      BOOLEAN _needRollbackAlways ;
      BOOLEAN _needRollback ;
      BOOLEAN _needUpdate ;
      BOOLEAN _hasUpdated ;
      BOOLEAN _needClearAfterDone ;

      std::string _targetName ;
      BSONObj _boTarget ;
   } ;

   typedef class _catContextBase catContext ;
}

#endif //CATCONTEXT_HPP_

