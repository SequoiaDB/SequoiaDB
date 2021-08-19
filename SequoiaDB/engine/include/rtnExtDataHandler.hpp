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

   Source File Name = rtnExtDataHandler.hpp

   Descriptive Name = External data process handler for rtn.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_EXTDATAHANDLER_HPP__
#define RTN_EXTDATAHANDLER_HPP__

#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "dmsExtDataHandler.hpp"
#include "rtnExtDataProcessor.hpp"
#include "rtnExtContext.hpp"

namespace engine
{
   #define RTN_INVALID_LOCK_HANDLE   NULL

   typedef void*     LOCK_HANDLE ;

   class _rtnExtDataHandler : public _IDmsExtDataHandler
   {
      typedef ossPoolMap<rtnExtDataProcessor*, INT32> LOCK_INFO_MAP ;
      typedef LOCK_INFO_MAP::iterator LOCK_INFO_MAP_ITR ;
   public:
      _rtnExtDataHandler( rtnExtDataProcessorMgr *edpMgr ) ;
      virtual ~_rtnExtDataHandler() ;

   public:

      void enable() ;

      INT32 disable( INT32 timeout ) ;


      virtual INT32 prepare( DMS_EXTOPR_TYPE type, const CHAR *csName,
                             const CHAR *clName, const CHAR *idxName,
                             const BSONObj *object, const BSONObj *objNew,
                             pmdEDUCB *cb ) ;

      virtual INT32 onOpenTextIdx( const CHAR *csName, const CHAR *clName,
                                   ixmIndexCB &indexCB ) ;

      // Always called after the cs name have been removed from cscbVec.(In 2P
      // dropping, it may be in the delCscbVec. Any way, other sessions are not
      // able to see this cs now.
      virtual INT32 onDelCS( const CHAR *csName, pmdEDUCB *cb,
                             BOOLEAN removeFiles, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onDelCL( const CHAR *csName, const CHAR *clName,
                             pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onCrtTextIdx( _dmsMBContext *context, const CHAR *csName,
                                  ixmIndexCB &indexCB, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onDropTextIdx( const CHAR *extName, _pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onRebuildTextIdx( const CHAR *csName, const CHAR *clName,
                                      const CHAR *idxName, const CHAR *extName,
                                      const BSONObj &indexDef, _pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onInsert( const CHAR *extName, const BSONObj &object,
                              _pmdEDUCB* cb, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onDelete( const CHAR *extName, const BSONObj &object,
                              _pmdEDUCB* cb, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onUpdate( const CHAR *extName, const BSONObj &orignalObj,
                              const BSONObj &newObj, _pmdEDUCB* cb,
                              BOOLEAN isRollback = FALSE,
                              SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onTruncateCL( const CHAR *csName, const CHAR *clName,
                                  _pmdEDUCB *cb, BOOLEAN needChangeCLID = TRUE,
                                  SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onRenameCS( const CHAR *oldCSName, const CHAR *newCSName,
                                _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 onRenameCL( const CHAR *csName, const CHAR *oldCLName,
                                const CHAR *newCLName, _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 done( DMS_EXTOPR_TYPE type, _pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) ;

      virtual INT32 abortOperation( DMS_EXTOPR_TYPE type, _pmdEDUCB *cb ) ;

      INT32 acquireLock( const CHAR *extName, INT32 lockType,
                         LOCK_HANDLE &handle ) ;

      void releaseLock( LOCK_HANDLE handle ) ;

   private:
      BOOLEAN _hasExtName( const ixmIndexCB &indexCB ) ;

      INT32 _extendIndexDef( const CHAR *csName, const CHAR *clName,
                             ixmIndexCB &indexCB ) ;

      OSS_INLINE INT32 _hold() ;

      OSS_INLINE void _release() ;

      OSS_INLINE BOOLEAN _holdingType( DMS_EXTOPR_TYPE type ) const ;

      INT32 _prepare( rtnExtDataOprCtx *context, const CHAR *csName,
                      const CHAR *clName, const CHAR *idxName ) ;

      INT32 _getContext( DMS_EXTOPR_TYPE type, rtnExtContextBase *&ctx,
                         pmdEDUCB *cb ) ;

      INT32 _getExtDataName( utilCLUniqueID clUniqID,
                             const CHAR *idxName,
                             CHAR *extName,
                             UINT32 buffSize ) ;

      // For compatibility with version 3.0. The external name rule is
      // different. This is used during upgrading from version 3.0, to append
      // 'ExtDataName' to indexCB.
      INT32 _getExtDataNameV1( const CHAR *csName, const CHAR *clName,
                               const CHAR *idxName, CHAR *extName,
                               UINT32 buffSize ) ;
   private:
      BOOLEAN                 _enabled ;
      ossAtomic32             _refCount ;
      rtnExtDataProcessorMgr  *_edpMgr ;
      rtnExtContextMgr        _contextMgr ;
      ossSpinXLatch           _latch ;
      LOCK_INFO_MAP           _lockInfo ;
   } ;
   typedef _rtnExtDataHandler rtnExtDataHandler ;

   OSS_INLINE INT32 _rtnExtDataHandler::_hold()
   {
      INT32 rc = SDB_OK ;

      if ( !_enabled )
      {
         rc = SDB_CLS_FULL_SYNC ;
         PD_LOG( PDERROR, "External data handler is not enabled. Maybe some "
                          "full sync is in progress and this node is source") ;
         goto error ;
      }
      _refCount.inc() ;

      // Double check if it has been disabled by someone else.
      if ( !_enabled )
      {
         _refCount.dec() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE void _rtnExtDataHandler::_release()
   {
      SDB_ASSERT( _refCount.fetch() > 0, "External handler reference "
                                         "is not greater than 0" ) ;
      _refCount.dec() ;
   }

   OSS_INLINE BOOLEAN _rtnExtDataHandler::_holdingType( DMS_EXTOPR_TYPE type ) const
   {
      return ( ( DMS_EXTOPR_TYPE_INSERT == type ) ||
               ( DMS_EXTOPR_TYPE_DELETE == type ) ||
               ( DMS_EXTOPR_TYPE_UPDATE == type ) ||
               ( DMS_EXTOPR_TYPE_TRUNCATE == type ) ) ;
   }

   rtnExtDataHandler* rtnGetExtDataHandler() ;
}

#endif /* RTN_EXTDATAHANDLER_HPP__ */

