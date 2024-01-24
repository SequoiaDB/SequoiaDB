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

   Source File Name = clsFreezingWindow.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          22/11/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_FREEZING_WINDOW_HPP_
#define CLS_FREEZING_WINDOW_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "pmdEDUMgr.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "ossEvent.hpp"
#include "dpsUtil.hpp"
#include "utilCSKeyName.hpp"
#include "rtnContext.hpp"

namespace engine
{

   #define CLS_BLOCKWRITE_TIMES ( 30 )

   /*
      _clsFreezingItem define
    */
   class _clsFreezingItem : public utilPooledObject
   {
   public:
      _clsFreezingItem()
      : _blockID( 0 )
      {
      }

      _clsFreezingItem( UINT64 blockID )
      : _blockID( blockID )
      {
      }

      _clsFreezingItem( const _clsFreezingItem &item )
      : _blockID( item._blockID ),
        _transWhiteList( item._transWhiteList ),
        _ctxWhiteList( item._ctxWhiteList )
      {
      }

      ~_clsFreezingItem()
      {
      }

      OSS_INLINE UINT64 getBlockID() const
      {
         return _blockID ;
      }

      // WARNING: may throw exception
      // NOTE: used as element in set
      OSS_INLINE void updateTransWhiteList( const DPS_TRANS_ID_SET &whiteList ) const
      {
         // union
         _transWhiteList.insert( whiteList.begin(), whiteList.end() ) ;
      }

      OSS_INLINE BOOLEAN isInTransWhiteList( const DPS_TRANS_ID &transID ) const
      {
         return ( ( DPS_INVALID_TRANS_ID != transID ) &&
                  ( _transWhiteList.end() !=
                                     _transWhiteList.find( transID ) ) ) ?
                                                               TRUE : FALSE ;
      }

      // WARNING: may throw exception
      // NOTE: used as element in set
      OSS_INLINE void updateCtxWhiteList( const RTN_CTX_ID_SET &whiteList ) const
      {
         // union
         _ctxWhiteList.insert( whiteList.begin(), whiteList.end() ) ;
      }

      OSS_INLINE BOOLEAN isInCtxWhiteList( const INT64 &contextID ) const
      {
         return ( ( -1 != contextID ) &&
                  ( _ctxWhiteList.end() !=
                                    _ctxWhiteList.find( contextID ) ) ) ?
                                                               TRUE : FALSE ;
      }

      _clsFreezingItem &operator =( const _clsFreezingItem &item )
      {
         _blockID = item._blockID ;
         _transWhiteList = item._transWhiteList ;
         _ctxWhiteList = item._ctxWhiteList ;
         return (*this) ;
      }

      bool operator ==( const _clsFreezingItem &item ) const
      {
         return _blockID == item._blockID ;
      }

      bool operator <( const _clsFreezingItem &item ) const
      {
         return _blockID < item._blockID ;
      }

   protected:
      // blocking ID
      UINT64            _blockID ;
      // white list for locked transactions
      mutable DPS_TRANS_ID_SET _transWhiteList ;
      mutable RTN_CTX_ID_SET   _ctxWhiteList ;
   } ;

   typedef class _clsFreezingItem clsFreezingItem ;

   /*
      _clsFreezingWindow define
   */
   class _clsFreezingWindow : public SDBObject
   {
      typedef ossPoolSet< clsFreezingItem >        OP_SET ;
      typedef ossPoolMap< ossPoolString, OP_SET >  MAP_WINDOW ;

      typedef ossPoolMap< utilCSKeyName, OP_SET >   MAP_CS_WINDOW ;

      public:
         _clsFreezingWindow() ;
         ~_clsFreezingWindow() ;

         INT32 registerCL ( const CHAR *pName, UINT64 &opID ) ;
         INT32 updateCLTransWhiteList( const CHAR *pName,
                                       UINT64 opID,
                                       const DPS_TRANS_ID_SET &whiteList ) ;
         INT32 updateCLCtxWhiteList( const CHAR *pName,
                                     UINT64 opID,
                                     const RTN_CTX_ID_SET &whiteList ) ;
         void  unregisterCL ( const CHAR *pName, UINT64 opID ) ;

         INT32 registerCS ( const CHAR *pName, UINT64 &opID ) ;
         INT32 updateCSTransWhiteList( const CHAR *pName,
                                       UINT64 opID,
                                       const DPS_TRANS_ID_SET &whiteList ) ;
         INT32 updateCSCtxWhiteList( const CHAR *pName,
                                     UINT64 opID,
                                     const RTN_CTX_ID_SET &whiteList ) ;
         void  unregisterCS ( const CHAR *pName, UINT64 opID ) ;

         INT32 registerWhole( UINT64 &opID ) ;
         INT32 updateWholeTransWhiteList( UINT64 opID,
                                          const DPS_TRANS_ID_SET &whiteList ) ;
         INT32 updateWholeCtxWhiteList( UINT64 opID,
                                        const RTN_CTX_ID_SET &whiteList ) ;
         void  unregisterWhole( UINT64 opID ) ;

         void  unregisterAll() ;

         INT32 waitForOpr( const CHAR *pName,
                           _pmdEDUCB *cb,
                           BOOLEAN isWrite ) ;

         BOOLEAN needBlockOpr( const ossPoolString &name,
                               UINT64 testOpID,
                               _pmdEDUCB *cb ) ;

      private :
         INT32 _registerCLInternal ( const ossPoolString &name, UINT64 opID ) ;
         void  _unregisterCLInternal ( const ossPoolString &name, UINT64 opID ) ;
         INT32 _registerCSInternal( const ossPoolString &name, UINT64 opID ) ;
         void  _unregisterCSInternal( const ossPoolString &name, UINT64 opID ) ;
         INT32 _regWholeInternal( UINT64 opID ) ;
         void  _unregWholeInternal( UINT64 opID ) ;

         void  _blockCheck( const OP_SET &setID,
                            UINT64 testOPID,
                            _pmdEDUCB *cb,
                            BOOLEAN &result,
                            BOOLEAN &forceEnd ) ;

         void  _unregisterCLByIter ( const CHAR * pName, UINT64 opID ) ;
         void  _unregisterCSByIter ( const CHAR * pName, UINT64 opID ) ;

      private:
         volatile UINT32   _clCount ;
         MAP_WINDOW        _mapWindow ;
         MAP_CS_WINDOW     _mapCSWindow ;
         OP_SET            _setWholeID ;
         ossSpinXLatch     _latch ;
         ossEvent          _event ;

   } ;
   typedef _clsFreezingWindow clsFreezingWindow ;

   /*
      freezing window checker define
    */
   #define CLS_FREEZING_CHECKER_MASK_EDU     ( 0x01 )
   #define CLS_FREEZING_CHECKER_MASK_CTX     ( 0x02 )
   #define CLS_FREEZING_CHECKER_MASK_TRANS   ( 0x04 )

   enum CLS_FREEZING_CHECKER_STEP
   {
      CLS_FREEZING_CHECKER_EDU,
      CLS_FREEZING_CHECKER_CTX,
      CLS_FREEZING_CHECKER_TRANS,
      CLS_FREEZING_CHECKER_DONE
   } ;

   /*
      _clsFreezingCheckResult define
    */
   class _clsFreezingCheckResult : public SDBObject
   {
   public:
      _clsFreezingCheckResult()
      : _isPassed( FALSE ),
        _step( CLS_FREEZING_CHECKER_EDU ),
        _blockID( 0 ),
        _blockEDUID( PMD_INVALID_EDUID ),
        _blockCtxID( -1 ),
        _blockCtxNum( 0 ),
        _blockTransID(),
        _blockTransNum( 0 )
      {
      }

      ~_clsFreezingCheckResult()
      {
      }

   public:
      BOOLEAN                    _isPassed ;
      CLS_FREEZING_CHECKER_STEP  _step ;
      UINT64                     _blockID ;
      EDUID                      _blockEDUID ;
      INT64                      _blockCtxID ;
      UINT32                     _blockCtxNum ;
      DPS_TRANS_ID               _blockTransID ;
      UINT32                     _blockTransNum ;
   } ;

   typedef class _clsFreezingCheckResult clsFreezingCheckResult ;

   /*
      _clsFreezingChecker
    */

   class _clsFreezingChecker : public SDBObject
   {
   public:
      _clsFreezingChecker( clsFreezingWindow *freezingWindow,
                           UINT64 blockID,
                           const CHAR *objName ) ;
      virtual ~_clsFreezingChecker() ;

      INT32 enableEDUCheck( pmdEDUCB *cb,
                            EDU_BLOCK_TYPE excludeBlockType ) ;
      INT32 enableCtxCheck( pmdEDUCB *cb ) ;
      INT32 enableTransCheck( pmdEDUCB *cb,
                              UINT32 logicalCSID,
                              UINT16 collectionID,
                              DPS_TRANSLOCK_TYPE transLockType,
                              BOOLEAN canSelfIncomp ) ;

      INT32 check( pmdEDUCB *cb,
                   clsFreezingCheckResult &result ) ;
      INT32 loopCheck( pmdEDUCB *cb,
                       UINT32 maxTimes = CLS_BLOCKWRITE_TIMES,
                       IContext *context = NULL ) ;

      CLS_FREEZING_CHECKER_STEP getStep() const
      {
         return _step ;
      }

   protected:
      INT32 _checkEDUs( pmdEDUCB *cb,
                        clsFreezingCheckResult &result ) ;
      INT32 _checkContexts( pmdEDUCB *cb,
                            clsFreezingCheckResult &result ) ;
      INT32 _checkTransactions( pmdEDUCB *cb,
                                clsFreezingCheckResult &result ) ;

      INT32 _getCtxBlockList( pmdEDUCB *cb,
                              clsFreezingCheckResult *result ) ;
      INT32 _getTransBlockList( pmdEDUCB *cb,
                                clsFreezingCheckResult *result ) ;

      virtual INT32 _isRelated( pmdEDUCB *cb,
                                const CHAR *objName,
                                BOOLEAN &isRelated )
      {
         isRelated = FALSE ;
         return SDB_OK ;
      }

      virtual INT32 _updateTransWhiteList( const DPS_TRANS_ID_SET &whiteList )
      {
         return SDB_OK ;
      }

      virtual INT32 _updateCtxWhiteList( const RTN_CTX_ID_SET &whiteList )
      {
         return SDB_OK ;
      }

   protected:
      clsFreezingWindow *  _freezingWindow ;
      UINT8                _checkMask ;
      UINT8                _enableMask ;
      CLS_FREEZING_CHECKER_STEP _step ;
      UINT64               _blockID ;
      const CHAR *         _objName ;

      // EDU check related
      EDU_BLOCK_TYPE       _excludeBlockType ;

      // transaction check related
      dpsTransLockId       _transLockID ;
      DPS_TRANSLOCK_TYPE   _transLockType ;
      BOOLEAN              _canSelfIncomp ;

      // context check related
      EDUID                _selfEDUID ;
   } ;

   typedef class _clsFreezingChecker clsFreezingChecker ;

   /*
      _clsFreezingCLChecker define
    */
   class  _clsFreezingCLChecker : public _clsFreezingChecker
   {
   public:
      _clsFreezingCLChecker( clsFreezingWindow *freezingWindow,
                             UINT64 blockID,
                             const CHAR *clName,
                             const CHAR *mainCLName ) ;
      virtual ~_clsFreezingCLChecker() ;

   protected:
      virtual INT32 _isRelated( pmdEDUCB *cb,
                                const CHAR *objName,
                                BOOLEAN &isRelated ) ;

      virtual INT32 _updateTransWhiteList( const DPS_TRANS_ID_SET &whiteList )
      {
         return _freezingWindow->updateCLTransWhiteList( _objName,
                                                         _blockID,
                                                         whiteList ) ;
      }

      virtual INT32 _updateCtxWhiteList( const RTN_CTX_ID_SET &whiteList )
      {
         return _freezingWindow->updateCLCtxWhiteList( _objName,
                                                       _blockID,
                                                       whiteList ) ;
      }

      BOOLEAN _isRelated( const CHAR *selfName, const CHAR *objName ) ;

   protected:
      const CHAR * _mainCLName ;
   } ;

   typedef class _clsFreezingCLChecker clsFreezingCLChecker ;

   /*
      _clsFreezingCSChecker define
    */
   class  _clsFreezingCSChecker : public _clsFreezingChecker
   {
   public:
      _clsFreezingCSChecker( clsFreezingWindow *freezingWindow,
                             UINT64 blockID,
                             const CHAR *csName ) ;
      virtual ~_clsFreezingCSChecker() ;

   protected:
      virtual INT32 _isRelated( pmdEDUCB *cb,
                                const CHAR *objName,
                                BOOLEAN &isRelated ) ;

      virtual INT32 _updateTransWhiteList( const DPS_TRANS_ID_SET &whiteList )
      {
         return _freezingWindow->updateCSTransWhiteList( _objName,
                                                         _blockID,
                                                         whiteList ) ;
      }

      virtual INT32 _updateCtxWhiteList( const RTN_CTX_ID_SET &whiteList )
      {
         return _freezingWindow->updateCSCtxWhiteList( _objName,
                                                       _blockID,
                                                       whiteList ) ;
      }

      BOOLEAN _isRelated( const CHAR *selfName, const CHAR *objName ) ;
   } ;

   typedef class _clsFreezingCSChecker clsFreezingCSChecker ;

}

#endif // CLS_FREEZING_WINDOW_HPP_
