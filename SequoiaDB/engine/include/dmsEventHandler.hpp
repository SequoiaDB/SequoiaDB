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

   Source File Name = dmsEventHandler.hpp

   Descriptive Name = Data Management Service Event Handler Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS event handler.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef DMS_EVENTHANDLER_HPP_
#define DMS_EVENTHANDLER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "pmd.hpp"
#include "dmsOprtOptions.hpp"
#include "dpsLogWrapper.hpp"
#include "dmsSUCache.hpp"
#include "utilUniqueID.hpp"

namespace engine
{

   // forward define
   class _dmsStorageUnit ;
   typedef class _dmsStorageUnit dmsStorageUnit ;

   class _dmsMBContext ;
   typedef class _dmsMBContext dmsMBContext ;

   class _IDmsEventHolder ;
   typedef class _IDmsEventHolder IDmsEventHolder ;

   typedef class _IUtilSUCacheHolder<DMS_MME_SLOTS> IDmsSUCacheHolder ;

   #define DMS_EVENT_MASK_ALL    0xFFFFFFFF
   #define DMS_EVENT_MASK_STAT   0x00000001
   #define DMS_EVENT_MASK_PLAN   0x00000002
   #define DMS_EVENT_MASK_RECY   0x00000004

   /*
      _dmsEventSUItem define
    */
   typedef struct _dmsEventSUItem
   {
      _dmsEventSUItem ()
      : _pCSName( NULL ),
        _suID( DMS_INVALID_SUID ),
        _suLID( DMS_INVALID_LOGICCSID ),
        _csUniqueID( UTIL_UNIQUEID_NULL )
      {
      }

      _dmsEventSUItem( const CHAR *pCSName, dmsStorageUnitID suID,
                       UINT32 suLID )
      : _pCSName( pCSName ),
        _suID( suID ),
        _suLID( suLID ),
        _csUniqueID( UTIL_UNIQUEID_NULL )
      {
      }

      void init( const CHAR *pCSName,
                 dmsStorageUnitID suID,
                 UINT32 suLID,
                 utilCSUniqueID csUniqueID )
      {
         _pCSName = pCSName ;
         _suID = suID ;
         _suLID = suLID ;
         _csUniqueID = csUniqueID ;
      }

      BOOLEAN isValid() const
      {
         return DMS_INVALID_SUID != _suID ;
      }

      const CHAR *      _pCSName ;
      dmsStorageUnitID  _suID ;
      UINT32            _suLID ;
      utilCSUniqueID    _csUniqueID ;
   } dmsEventSUItem ;

   /*
      _dmsEventCLItem define
    */
   typedef struct _dmsEventCLItem
   {
      _dmsEventCLItem ()
      : _pCLName( NULL ),
        _logicCSID( DMS_INVALID_LOGICCSID ),
        _mbID( DMS_INVALID_MBID ),
        _clLID( DMS_INVALID_CLID ),
        _mbContext( NULL )
      {
      }

      _dmsEventCLItem ( const CHAR *pCLName, UINT16 mbID, UINT32 clLID )
      : _pCLName( pCLName ),
        _logicCSID( DMS_INVALID_LOGICCSID ),
        _mbID( mbID ),
        _clLID( clLID ),
        _mbContext( NULL )
      {
      }

      void init( const CHAR *pCLName,
                 UINT32 logicCSID,
                 UINT16 mbID,
                 UINT32 clLID,
                 _dmsMBContext *mbContext )
      {
         SDB_ASSERT( NULL != pCLName, "collection name is invalid" ) ;
         SDB_ASSERT( NULL != mbContext, "meta block context is invalid" ) ;
         _pCLName = pCLName ;
         _logicCSID = logicCSID ;
         _mbID = mbID ;
         _clLID = clLID ;
         _mbContext = mbContext ;
      }

      BOOLEAN isValid() const
      {
         return DMS_INVALID_MBID != _mbID ;
      }

      const CHAR *   _pCLName ;
      UINT32         _logicCSID ;
      UINT16         _mbID ;
      UINT32         _clLID ;
      _dmsMBContext * _mbContext ;
   } dmsEventCLItem ;

   /*
      _dmsEventIdxItem define
    */
   typedef struct _dmsEventIdxItem
   {
      _dmsEventIdxItem ()
      : _pIXName( NULL ),
        _idxLID( DMS_INVALID_EXTENT )
      {
      }

      _dmsEventIdxItem ( const CHAR *pIXName, dmsExtentID idxLID,
                         const BSONObj &boDefine )
      : _pIXName( pIXName ),
        _idxLID( idxLID ),
        _boDefine( boDefine )
      {
      }

      const CHAR *   _pIXName ;
      dmsExtentID    _idxLID ;
      BSONObj        _boDefine ;
   } dmsEventIdxItem ;

   /*
      _IDmsEventHandler
    */
   class _IDmsEventHandler
   {
      public :
         _IDmsEventHandler () {}

         virtual ~_IDmsEventHandler () {}

         OSS_INLINE virtual INT32 onCreateCS ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onLoadCS ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onUnloadCS ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onRenameCS ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const CHAR *pOldCSName,
                                               const CHAR *pNewCSName,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         // drop collection space callbacks
         OSS_INLINE virtual INT32 onCheckDropCS( IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventSUItem &suItem,
                                                 dmsDropCSOptions *options,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onDropCS ( SDB_EVENT_OCCUR_TYPE type,
                                             IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             const dmsEventSUItem &suItem,
                                             dmsDropCSOptions *options,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onCleanDropCS( IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventSUItem &suItem,
                                                 dmsDropCSOptions *options,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onCreateCL ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onRenameCL ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               const CHAR *pNewCLName,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         // truncate collection callbacks
         OSS_INLINE virtual INT32 onCheckTruncCL( IDmsEventHolder *pEventHolder,
                                                  IDmsSUCacheHolder *pCacheHolder,
                                                  const dmsEventCLItem &clItem,
                                                  dmsTruncCLOptions *options,
                                                  pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onTruncateCL ( SDB_EVENT_OCCUR_TYPE type,
                                                 IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventCLItem &clItem,
                                                 dmsTruncCLOptions *options,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onCleanTruncCL( IDmsEventHolder *pEventHolder,
                                                  IDmsSUCacheHolder *pCacheHolder,
                                                  const dmsEventCLItem &clItem,
                                                  dmsTruncCLOptions *options,
                                                  pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         // drop collection callbacks
         OSS_INLINE virtual INT32 onCheckDropCL( IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventCLItem &clItem,
                                                 dmsDropCLOptions *options,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onDropCL ( SDB_EVENT_OCCUR_TYPE type,
                                             IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             const dmsEventCLItem &clItem,
                                             dmsDropCLOptions *options,
                                             pmdEDUCB *cb,
                                             SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onCleanDropCL( IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventCLItem &clItem,
                                                 dmsDropCLOptions *options,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onCreateIndex ( IDmsEventHolder *pEventHolder,
                                                  IDmsSUCacheHolder *pCacheHolder,
                                                  const dmsEventCLItem &clItem,
                                                  const dmsEventIdxItem &idxItem,
                                                  pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onRebuildIndex ( IDmsEventHolder *pEventHolder,
                                                   IDmsSUCacheHolder *pCacheHolder,
                                                   const dmsEventCLItem &clItem,
                                                   const dmsEventIdxItem &idxItem,
                                                   pmdEDUCB *cb,
                                                   SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onDropIndex ( IDmsEventHolder *pEventHolder,
                                                IDmsSUCacheHolder *pCacheHolder,
                                                const dmsEventCLItem &clItem,
                                                const dmsEventIdxItem &idxItem,
                                                pmdEDUCB *cb,
                                                SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onLinkCL ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             const dmsEventCLItem &clItem,
                                             const CHAR *pMainCLName,
                                             pmdEDUCB *cb, SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onUnlinkCL ( IDmsEventHolder *pEventHolder,
                                               IDmsSUCacheHolder *pCacheHolder,
                                               const dmsEventCLItem &clItem,
                                               const CHAR *pMainCLName,
                                               pmdEDUCB *cb,
                                               SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onClearSUCaches ( IDmsEventHolder *pEventHolder,
                                                    IDmsSUCacheHolder *pCacheHolder )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onClearCLCaches ( IDmsEventHolder *pEventHolder,
                                                    IDmsSUCacheHolder *pCacheHolder,
                                                    const dmsEventCLItem &clItem )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onChangeSUCaches ( IDmsEventHolder *pEventHolder,
                                                     IDmsSUCacheHolder *pCacheHolder )
         {
            return SDB_OK ;
         }

         virtual UINT32 getMask () const = 0 ;

         virtual const CHAR *getName() const = 0 ;
   } ;

   typedef ossPoolList<_IDmsEventHandler *> DMS_HANDLER_LIST ;

   /*
      _IDmsEventHolder
    */
   class _IDmsEventHolder
   {
      public :
         _IDmsEventHolder () {}

         virtual ~_IDmsEventHolder () {}

         virtual void setHandlers ( DMS_HANDLER_LIST *handlers ) = 0 ;

         virtual void unsetHandlers () = 0 ;

         virtual INT32 onCreateCS ( UINT32 mask,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onLoadCS ( UINT32 mask,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onUnloadCS ( UINT32 mask,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onRenameCS ( UINT32 mask,
                                    const CHAR *pOldCSName,
                                    const CHAR *pNewCSName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) = 0 ;

         // drop collection space callbacks
         virtual INT32 onCheckDropCS( UINT32 mask,
                                      const dmsEventSUItem &suItem,
                                      dmsDropCSOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onDropCS ( UINT32 mask,
                                  SDB_EVENT_OCCUR_TYPE type,
                                  const dmsEventSUItem &suItem,
                                  dmsDropCSOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onCleanDropCS( UINT32 mask,
                                      const dmsEventSUItem &suItem,
                                      dmsDropCSOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onCreateCL ( UINT32 mask,
                                    const dmsEventCLItem &clItem,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onRenameCL ( UINT32 mask,
                                    const dmsEventCLItem &clItem,
                                    const CHAR *pNewCLName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) = 0 ;

         // truncate collection callbacks
         virtual INT32 onCheckTruncCL( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       dmsTruncCLOptions *options,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onTruncateCL ( UINT32 mask,
                                      SDB_EVENT_OCCUR_TYPE type,
                                      const dmsEventCLItem &clItem,
                                      dmsTruncCLOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onCleanTruncCL( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       dmsTruncCLOptions *options,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) = 0 ;

         // drop collection callbacks
         virtual INT32 onCheckDropCL( UINT32 mask,
                                      const dmsEventCLItem &clItem,
                                      dmsDropCLOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onDropCL ( UINT32 mask,
                                  SDB_EVENT_OCCUR_TYPE type,
                                  const dmsEventCLItem &clItem,
                                  dmsDropCLOptions *options,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onCleanDropCL( UINT32 mask,
                                      const dmsEventCLItem &clItem,
                                      dmsDropCLOptions *options,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onCreateIndex ( UINT32 mask,
                                       const dmsEventCLItem &clItem,
                                       const dmsEventIdxItem &idxItem,
                                       pmdEDUCB *cb,
                                       SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onRebuildIndex ( UINT32 mask,
                                        const dmsEventCLItem &clItem,
                                        const dmsEventIdxItem &idxItem,
                                        pmdEDUCB *cb,
                                        SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onDropIndex ( UINT32 mask,
                                     const dmsEventCLItem &clItem,
                                     const dmsEventIdxItem &idxItem,
                                     pmdEDUCB *cb,
                                     SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onLinkCL ( UINT32 mask,
                                  const dmsEventCLItem &clItem,
                                  const CHAR *pMainCLName,
                                  pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onUnlinkCL ( UINT32 mask,
                                    const dmsEventCLItem &clItem,
                                    const CHAR *pMainCLName,
                                    pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onClearSUCaches ( UINT32 mask ) = 0 ;

         virtual INT32 onClearCLCaches ( UINT32 mask,
                                         const dmsEventCLItem &clItem ) = 0 ;

         virtual INT32 onChangeSUCaches ( UINT32 mask ) = 0 ;

         virtual const CHAR *getCSName () const = 0 ;

         virtual UINT32 getSUID () const = 0 ;

         virtual UINT32 getSULID () const = 0 ;
   } ;

}

#endif //DMS_EVENTHANDLER_HPP_

