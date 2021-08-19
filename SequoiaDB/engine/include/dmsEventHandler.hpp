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
#include "dpsLogWrapper.hpp"
#include "dmsSUCache.hpp"

namespace engine
{

   class _IDmsEventHolder ;
   typedef class _IDmsEventHolder IDmsEventHolder ;

   typedef class _IUtilSUCacheHolder<DMS_MME_SLOTS> IDmsSUCacheHolder ;

   #define DMS_EVENT_MASK_ALL    0xFFFFFFFF
   #define DMS_EVENT_MASK_STAT   0x00000001
   #define DMS_EVENT_MASK_PLAN   0x00000002

   /*
      _dmsEventSUItem define
    */
   typedef struct _dmsEventSUItem
   {
      _dmsEventSUItem ()
      : _pCSName( NULL ),
        _suID( DMS_INVALID_SUID ),
        _suLID( DMS_INVALID_LOGICCSID )
      {
      }

      _dmsEventSUItem( const CHAR *pCSName, dmsStorageUnitID suID,
                       UINT32 suLID )
      : _pCSName( pCSName ),
        _suID( suID ),
        _suLID( suLID )
      {
      }

      const CHAR *      _pCSName ;
      dmsStorageUnitID  _suID ;
      UINT32            _suLID ;
   } dmsEventSUItem ;

   /*
      _dmsEventCLItem define
    */
   typedef struct _dmsEventCLItem
   {
      _dmsEventCLItem ()
      : _pCLName( NULL ),
        _mbID( DMS_INVALID_MBID ),
        _clLID( DMS_INVALID_CLID )
      {
      }

      _dmsEventCLItem ( const CHAR *pCLName, UINT16 mbID, UINT32 clLID )
      : _pCLName( pCLName ),
        _mbID( mbID ),
        _clLID( clLID )
      {
      }

      const CHAR *   _pCLName ;
      UINT16         _mbID ;
      UINT32         _clLID ;
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

         OSS_INLINE virtual INT32 onDropCS ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
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

         OSS_INLINE virtual INT32 onTruncateCL ( IDmsEventHolder *pEventHolder,
                                                 IDmsSUCacheHolder *pCacheHolder,
                                                 const dmsEventCLItem &clItem,
                                                 UINT32 newCLLID,
                                                 pmdEDUCB *cb,
                                                 SDB_DPSCB *dpsCB )
         {
            return SDB_OK ;
         }

         OSS_INLINE virtual INT32 onDropCL ( IDmsEventHolder *pEventHolder,
                                             IDmsSUCacheHolder *pCacheHolder,
                                             const dmsEventCLItem &clItem,
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

         virtual UINT32 getMask () = 0 ;
   } ;

   /*
      _IDmsEventHolder
    */
   class _IDmsEventHolder
   {
      public :
         _IDmsEventHolder () {}

         virtual ~_IDmsEventHolder () {}

         virtual void regHandler ( _IDmsEventHandler *pHandler ) = 0 ;

         virtual void unregHandler ( _IDmsEventHandler *pHandler ) = 0 ;

         virtual void unregAllHandlers () = 0 ;

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

         virtual INT32 onDropCS ( UINT32 mask,
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

         virtual INT32 onTruncateCL ( UINT32 mask,
                                      const dmsEventCLItem &clItem,
                                      UINT32 newCLLID,
                                      pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) = 0 ;

         virtual INT32 onDropCL ( UINT32 mask,
                                  const dmsEventCLItem &clItem,
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

