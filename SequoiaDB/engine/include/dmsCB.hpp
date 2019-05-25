/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dmsCB.hpp

   Descriptive Name = Data Management Service Control Block Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   data management control block, which is the metatdata information for DMS
   component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSCB_HPP_
#define DMSCB_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMem.hpp"
#include "dms.hpp"
#include "ossLatch.hpp"
#include "monDMS.hpp"
#include "dmsTempSUMgr.hpp"
#include "dmsStatSUMgr.hpp"
#include "ossAtomic.hpp"
#include "ossRWMutex.hpp"
#include "dpsLogWrapper.hpp"
#include "ossEvent.hpp"
#include "sdbInterface.hpp"
#include "dmsIxmKeySorter.hpp"
#include "utilMap.hpp"
#include "dmsStorageJob.hpp"
#include <map>
#include <set>


using namespace std ;

namespace engine
{
   class _pmdEDUCB ;
   class _dmsStorageUnit ;

   class _SDB_DMS_CSCB : public SDBObject
   {
   public:
      UINT32 _topSequence ;
      CHAR   _name [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      _dmsStorageUnit *_su ;
      _SDB_DMS_CSCB ( const CHAR *pName, UINT32 topSequence,
                      _dmsStorageUnit *su )
      {
         ossStrncpy ( _name, pName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         _name[DMS_COLLECTION_SPACE_NAME_SZ] = 0 ;
         _topSequence = topSequence ;
         _su = su ;
      }
      ~_SDB_DMS_CSCB () ;
   } ;
   typedef class _SDB_DMS_CSCB SDB_DMS_CSCB ;


   #define DMS_MAX_CS_NUM 4096
   #define DMS_INVALID_CS DMS_INVALID_SUID

   /*
      DMS_STATE DEFINE
   */
   #define DMS_STATE_NORMAL            0
   #define DMS_STATE_READONLY          1
   #define DMS_STATE_ONLINE_BACKUP     2
   #define DMS_STATE_FULLSYNC          3

   /*
      OTHER DEFINE
   */
   #define DMS_CHANGESTATE_WAIT_LOOP   100

   struct _dmsDictJob
   {
      dmsStorageUnitID _suID ;
      UINT32 _suLID ;
      UINT16 _clID ;
      UINT32 _clLID ;
      UINT64 _createTime ;

      _dmsDictJob()
      : _suID( DMS_INVALID_SUID ),
        _suLID( DMS_INVALID_SUID ),
        _clID( DMS_INVALID_CLID ),
        _clLID( DMS_INVALID_CLID )
      {
      }

      _dmsDictJob( dmsStorageUnitID suID, UINT32 suLID, UINT16 clID,
                   UINT32 clLID )
      : _suID( suID ),
        _suLID( suLID ),
        _clID( clID ),
        _clLID( clLID ),
        _createTime( 0 )
      {
      }
   } ;
   typedef _dmsDictJob dmsDictJob ;

   /*
      _SDB_DMSCB define
   */
   class _SDB_DMSCB : public _IControlBlock
   {
   private :
      ossSpinSLatch _mutex ;

      struct cmp_cscb
      {
         bool operator() (const char *a, const char *b)
         {
            return std::strcmp(a,b)<0 ;
         }
      } ;
      std::map<const CHAR*, dmsStorageUnitID, cmp_cscb> _cscbNameMap ;
      std::vector<SDB_DMS_CSCB*>          _cscbVec ;
      std::vector<SDB_DMS_CSCB*>          _delCscbVec ;
      std::vector<ossRWMutex*>            _latchVec ;
      std::vector<dmsStorageUnitID>       _freeList ;
      std::vector< ossSpinRecursiveXLatch* >  _vecCSMutex ;

#if defined (_WINDOWS)
      typedef std::map<const CHAR*,
                       dmsStorageUnitID,
                       cmp_cscb>::const_iterator CSCB_MAP_CONST_ITER ;
      typedef std::map<const CHAR*,
                       dmsStorageUnitID,
                       cmp_cscb>::iterator CSCB_MAP_ITER ;
#elif defined (_LINUX)
      typedef std::map<const CHAR*,
                       dmsStorageUnitID>::const_iterator CSCB_MAP_CONST_ITER ;
      typedef std::map<const CHAR*,
                       dmsStorageUnitID>::iterator CSCB_MAP_ITER ;
#endif

      /*
       * Queue of collections which are waitting for dictionaies creation.
       * Here we store the storage unit id and mb ID of the collection.
       * One concern here is the reuse of these two IDs, but that's ok. I'll
       * just check with these IDs to see if dictionary creation is needed for
       * the CURRENT corresponding collection. If they have been reused, then
       * the original collection has been dropped. So we just ignore that.
       * If the IDs have been reused, and added to the queue again, it's also
       * ok.
       * Everytime we are about to create a dictionary, we should check if a
       * dictionary is there already. If yes, just remove the item from the
       * queue.
       */
      ossQueue<dmsDictJob>    _dictWaitQue ;

      ossSpinXLatch           _stateMtx;
      ossEvent                _blockEvent ;
      SINT64                  _writeCounter;
      UINT8                   _dmsCBState;
      UINT32                  _logicalSUID ;

      dmsTempSUMgr            _tempSUMgr ;
      dmsStatSUMgr            _statSUMgr ;

      dmsIxmKeySorterCreator* _ixmKeySorterCreator ;

      dmsPageMappingDispatcher   _pageMapDispatcher ;

   private:
      void  _logCSCBNameMap () ;

      INT32 _CSCBNameInsert ( const CHAR *pName, UINT32 topSequence,
                              _dmsStorageUnit *su,
                              dmsStorageUnitID &suID ) ;

      INT32 _CSCBNameLookup ( const CHAR *pName,
                              SDB_DMS_CSCB **cscb,
                              dmsStorageUnitID *suID = NULL,
                              BOOLEAN exceptDeleting = TRUE ) ;

      INT32 _CSCBNameLookupAndLock ( const CHAR *pName,
                                     dmsStorageUnitID &suID,
                                     SDB_DMS_CSCB **cscb,
                                     OSS_LATCH_MODE lockType = SHARED,
                                     INT32 millisec = -1 ) ;
      void _CSCBRelease ( dmsStorageUnitID suID,
                          OSS_LATCH_MODE lockType = SHARED ) ;
      INT32 _CSCBNameRemove ( const CHAR *pName, _pmdEDUCB *cb,
                              SDB_DPSCB *dpsCB, BOOLEAN onlyEmpty,
                              SDB_DMS_CSCB *&pCSCB ) ;
      INT32 _CSCBNameRemoveP1 ( const CHAR *pName,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpsCB ) ;
      INT32 _CSCBNameRemoveP1Cancel ( const CHAR *pName,
                                      _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) ;
      INT32 _CSCBNameRemoveP2 ( const CHAR *pName,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpsCB,
                                SDB_DMS_CSCB *&pCSCB ) ;
      void _CSCBNameMapCleanup () ;

      INT32 _CSCBRename( const CHAR *pName,
                         const CHAR *pNewName,
                         _pmdEDUCB *cb,
                         SDB_DPSCB *dpsCB ) ;

      INT32 _delCollectionSpace ( const CHAR *pName, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB, BOOLEAN removeFile,
                                  BOOLEAN onlyEmpty ) ;

   public:
      _SDB_DMSCB() ;
      virtual ~_SDB_DMSCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_DMS ; }
      virtual const CHAR* cbName() const { return "DMSCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;
      virtual void   onConfigChange() ;

      INT32 nameToSUAndLock ( const CHAR *pName, dmsStorageUnitID &suID,
                              _dmsStorageUnit **su,
                              OSS_LATCH_MODE lockType = SHARED,
                              INT32 millisec = -1 ) ;

      INT32 verifySUAndLock ( const dmsEventSUItem *pSUItem,
                              _dmsStorageUnit **ppSU,
                              OSS_LATCH_MODE lockType = SHARED,
                              INT32 millisec = -1 ) ;

      _dmsStorageUnit *suLock ( dmsStorageUnitID suID ) ;
      void suUnlock ( dmsStorageUnitID suID,
                      OSS_LATCH_MODE lockType = SHARED ) ;

      INT32 addCollectionSpace ( const CHAR *pName, UINT32 topSequence,
                                 _dmsStorageUnit *su, _pmdEDUCB *cb,
                                 SDB_DPSCB *dpsCB, BOOLEAN isCreate ) ;
      INT32 dropCollectionSpace ( const CHAR *pName, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpsCB ) ;
      INT32 dropEmptyCollectionSpace( const CHAR *pName, _pmdEDUCB *cb,
                                      SDB_DPSCB *dpsCB ) ;
      INT32 unloadCollectonSpace( const CHAR *pName, _pmdEDUCB *cb ) ;

      INT32 renameCollectionSpace( const CHAR *pName,
                                   const CHAR *pNewName,
                                   _pmdEDUCB *cb,
                                   SDB_DPSCB *dpsCB ) ;

      void dumpInfo ( MON_CL_SIM_LIST &collectionList,
                      BOOLEAN sys = FALSE ) ;
      void dumpInfo ( MON_CS_SIM_LIST &csList,
                      BOOLEAN sys = FALSE,
                      BOOLEAN dumpCL = FALSE,
                      BOOLEAN dumpIdx = FALSE ) ;

      void dumpInfo ( MON_CL_LIST &collectionList,
                      BOOLEAN sys = FALSE ) ;
      void dumpInfo ( MON_CS_LIST &csList,
                      BOOLEAN sys = FALSE ) ;
      void dumpInfo ( MON_SU_LIST &storageUnitList,
                      BOOLEAN sys = FALSE ) ;

      void dumpInfo ( INT64 &totalFileSize );

      void dumpPageMapCSInfo( MON_CSNAME_VEC &vecCS ) ;

      dmsTempSUMgr *getTempSUMgr () ;

      dmsStatSUMgr *getStatSUMgr () ;

      void clearSUCaches ( UINT32 mask ) ;

      void clearSUCaches ( const MON_CS_SIM_LIST &monCSList, UINT32 mask ) ;

      void changeSUCaches ( UINT32 mask ) ;

      void changeSUCaches ( const MON_CS_SIM_LIST &monCSList, UINT32 mask ) ;

      INT32 dropCollectionSpaceP1 ( const CHAR *pName, _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB );

      INT32 dropCollectionSpaceP1Cancel ( const CHAR *pName, _pmdEDUCB *cb,
                                          SDB_DPSCB *dpsCB );

      INT32 dropCollectionSpaceP2 ( const CHAR *pName, _pmdEDUCB *cb,
                                    SDB_DPSCB *dpsCB ) ;

      BOOLEAN dispatchDictJob( dmsDictJob &job ) ;
      void pushDictJob( dmsDictJob job ) ;

      void setIxmKeySorterCreator( dmsIxmKeySorterCreator* creator ) ;
      dmsIxmKeySorterCreator* getIxmKeySorterCreator() ;
      dmsIxmKeySorter* createIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer ) ;
      void releaseIxmKeySorter( dmsIxmKeySorter* sorter ) ;

   public:
      typedef std::vector<SDB_DMS_CSCB*>::iterator CSCB_ITERATOR;

      OSS_INLINE CSCB_ITERATOR begin()
      {
         return _cscbVec.begin();
      }

      OSS_INLINE CSCB_ITERATOR end()
      {
         return _cscbVec.end();
      }

      INT32 writable( _pmdEDUCB * cb ) ;
      void  writeDown( _pmdEDUCB * cb ) ;

      INT32 blockWrite( _pmdEDUCB *cb,
                        SDB_DB_STATUS byStatus = SDB_DB_NORMAL ) ;
      void  unblockWrite( _pmdEDUCB *cb ) ;

      INT32 registerBackup( _pmdEDUCB *cb, BOOLEAN offline = TRUE ) ;
      void  backupDown( _pmdEDUCB *cb ) ;

      INT32 registerRebuild( _pmdEDUCB *cb ) ;
      void  rebuildDown( _pmdEDUCB *cb ) ;

      INT32 registerFullSync( _pmdEDUCB *cb ) ;
      void  fullSyncDown( _pmdEDUCB *cb ) ;

      OSS_INLINE UINT8 getCBState () const
      {
         return _dmsCBState ;
      }

      void  aquireCSMutex( const CHAR *pCSName ) ;
      void  releaseCSMutex( const CHAR *pCSName ) ;
   } ;
   typedef class _SDB_DMSCB SDB_DMSCB ;

   /*
      _dmsCSMutexScope define
   */
   class _dmsCSMutexScope
   {
      public:
         _dmsCSMutexScope( SDB_DMSCB *pDMSCB, const CHAR *pName ) ;
         ~_dmsCSMutexScope() ;

      private:
         SDB_DMSCB            *_pDMSCB ;
         const CHAR           *_pName ;
   } ;
   typedef _dmsCSMutexScope dmsCSMutexScope ;

   /*
      get global SDB_DMSCB
   */
   SDB_DMSCB* sdbGetDMSCB () ;
}

#endif //DMSCB_HPP_

