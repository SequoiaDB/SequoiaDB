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

   Source File Name = rtnExtDataProcessor.hpp

   Descriptive Name = External data processor for rtn.

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
#ifndef RTN_EXTDATAPROCESSOR_HPP__
#define RTN_EXTDATAPROCESSOR_HPP__

#include "ixm.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdEDU.hpp"
#include "utilConcurrentMap.hpp"
#include "rtnExtOprDef.hpp"
#include "rtnExtOprData.hpp"
#include "dmsStorageUnit.hpp"

#define RTN_EXT_PROCESSOR_INVALID_ID   (-1)
#define RTN_EXT_PROCESSOR_MAX_NUM       64

namespace engine
{
   // Metadata of the processor.
   struct _rtnExtProcessorMeta
   {
      CHAR     _csName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      CHAR     _clName[ DMS_COLLECTION_NAME_SZ + 1 ] ;
      CHAR     _idxName[ IXM_INDEX_NAME_SIZE + 1 ] ;
      CHAR     _targetName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      BSONObj  _idxKeyDef ;

      _rtnExtProcessorMeta()
      {
         reset() ;
      }

      ~_rtnExtProcessorMeta() {}

      INT32 init( const CHAR *csName, const CHAR *clName,
                  const CHAR *idxName, const CHAR *targetName,
                  const BSONObj &idxKeyDef )
      {
         SDB_ASSERT( csName, "cs name is NULL" ) ;
         SDB_ASSERT( clName, "cl name is NULL" ) ;
         SDB_ASSERT( idxName, "index name is NULL" ) ;
         SDB_ASSERT( targetName, "target name is NULL" ) ;

         ossStrncpy( _csName, csName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         ossStrncpy( _clName, clName, DMS_COLLECTION_NAME_SZ ) ;
         ossStrncpy( _idxName, idxName, IXM_INDEX_NAME_SIZE ) ;
         ossStrncpy( _targetName, targetName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
         _idxKeyDef = idxKeyDef.copy() ;
         return SDB_OK ;
      }

      void reset()
      {
         ossMemset( _csName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
         ossMemset( _clName, 0, DMS_COLLECTION_NAME_SZ + 1 ) ;
         ossMemset( _idxName, 0, IXM_INDEX_NAME_SIZE + 1 ) ;
         ossMemset( _targetName, 0, DMS_COLLECTION_SPACE_NAME_SZ + 1 ) ;
         _idxKeyDef = BSONObj() ;
      }
   } ;
   typedef _rtnExtProcessorMeta rtnExtProcessorMeta ;

   enum _rtnExtProcessorStat
   {
      RTN_EXT_PROCESSOR_NORMAL = 0,
      RTN_EXT_PROCESSOR_CREATING,
      RTN_EXT_PROCESSOR_INVALID
   };
   typedef _rtnExtProcessorStat rtnExtProcessorStat ;

   /*
    * One processor is according to one text index. It handles operations on the
    * relative capped collection. All processors are created and  managed by
    * the processor manager.
    * Processor will be created when a text index is created, or when open a
    * collection space, which contains text indices.
    */
   class _rtnExtDataProcessor : public SDBObject
   {
   public:
      _rtnExtDataProcessor() ;
      ~_rtnExtDataProcessor() ;

      INT32 init( INT32 id, const CHAR *csName, const CHAR *clName,
                  const CHAR *idxName, const CHAR *targetName,
                  const BSONObj &idxKeyDef ) ;
      void  reset() ;

      INT32 getID() const;

      rtnExtProcessorStat stat() const ;

      INT32 active() ;

      BOOLEAN isActive() const ;

      const rtnExtProcessorMeta* meta() const
      {
         return &_meta ;
      }

      void updateMeta( const CHAR *csName, const CHAR *clName = NULL,
                       const CHAR *idxName = NULL ) ;

      INT32 setTargetNames( const CHAR *extName ) ;

      INT32 prepare( DMS_EXTOPR_TYPE oprType, rtnExtOprData *oprData ) ;

      INT32 processDML( BSONObj &oprRecord, pmdEDUCB *cb,
                        BOOLEAN isRollback = FALSE, SDB_DPSCB *dpsCB = NULL ) ;

      INT32 processTruncate( pmdEDUCB *cb, BOOLEAN needChangeCLID,
                             SDB_DPSCB *dpsCB = NULL ) ;

      INT32 doDropP1( pmdEDUCB *cb, SDB_DPSCB *dpsCB = NULL ) ;
      INT32 doDropP1Cancel( pmdEDUCB *cb, SDB_DPSCB *dpsCB = NULL ) ;
      INT32 doDropP2( pmdEDUCB *cb, SDB_DPSCB *dpsCB = NULL ) ;
      INT32 doLoad() ;
      INT32 doUnload( _pmdEDUCB *cb ) ;
      INT32 doRebuild( pmdEDUCB *cb, SDB_DPSCB *dpsCB = NULL ) ;

      INT32 done( pmdEDUCB *cb ) ;
      INT32 abort() ;

      BOOLEAN isOwnedBy( const CHAR *csName, const CHAR *clName = NULL,
                         const CHAR *idxName = NULL ) const ;

      BOOLEAN isOwnedByExt( const CHAR *targetName ) const ;

   private:
      INT32 _prepareCSAndCL( const CHAR *csName, const CHAR *clName,
                             _pmdEDUCB *cb, SDB_DPSCB *dpsCB ) ;

      /*
       * Why prepare and done? The commit LSN should be the same on primary and
       * slaves. If we write before the log DPS, the commit LSN will be newer
       * than that on the slave.
       */
      INT32 _prepareInsert( rtnExtOprData *oprData ) ;
      INT32 _prepareDelete( rtnExtOprData *oprData ) ;
      INT32 _prepareUpdate( rtnExtOprData *oprData ) ;

      INT32 _genRecordByKeySet( const BSONObjSet &keySet, BSONObj &record ) ;

      INT32 _prepareRecord( _rtnExtOprType oprType,
                            BSONObj &recordObj,
                            const BSONObjSet *keySet,
                            const BSONElement &idEle,
                            const BSONElement *newIdEle = NULL ) ;

      const CHAR* _getExtCLShortName() const ;

      // Whether the corresponding capped collection has enough free space.
      INT32 _spaceCheck( UINT32 size ) ;

      INT32 _updateSpaceInfo( UINT64 &freeSpace ) ;

      INT32 _addRebuildRecord( pmdEDUCB *cb, SDB_DPSCB *dpsCB ) ;

   private:
      rtnExtProcessorStat  _stat ;
      dmsStorageUnit      *_su ;
      INT32                _id ;
      rtnExtProcessorMeta  _meta ;
      CHAR                 _cappedCSName[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;
      CHAR                 _cappedCLName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      BOOLEAN              _needUpdateLSN ;
      UINT64               _freeSpace ;
   } ;
   typedef _rtnExtDataProcessor rtnExtDataProcessor ;

   class _rtnExtDataProcessorMgr : public SDBObject
   {
   public:
      _rtnExtDataProcessorMgr() ;
      ~_rtnExtDataProcessorMgr () ;

      // activate: Once activated, the processor can be used by other threads.
      INT32 createProcessor( const CHAR *csName, const CHAR *clName,
                             const CHAR *idxName, const CHAR *extName,
                             const BSONObj &idxKeyDef,
                             rtnExtDataProcessor *&processor,
                             BOOLEAN activate = TRUE ) ;

      INT32 activateProcessor( INT32 id, BOOLEAN inProtection ) ;

      void destroyProcessor( rtnExtDataProcessor *processor,
                             INT32 lockType = -1 ) ;

      void destroyProcessors( vector<rtnExtDataProcessor*> &processors,
                              INT32 lockType = -1 ) ;

      UINT32 number()  ;

      INT32 getProcessorsByCS( const CHAR *csName, INT32 lockType,
                               vector<rtnExtDataProcessor *> &processors ) ;

      INT32 getProcessorsByCL( const CHAR *csName, const CHAR *clName,
                               INT32 lockType,
                               std::vector<rtnExtDataProcessor *> &processors ) ;

      INT32 getProcessorByIdx( const CHAR *csName, const CHAR *clName,
                               const CHAR *idxName, INT32 lockType,
                               rtnExtDataProcessor *&processor ) ;

      INT32 getProcessorByExtName( const CHAR *extName, INT32 lockType,
                                   rtnExtDataProcessor *&processor ) ;

      void unlockProcessors( std::vector<rtnExtDataProcessor *> &processors,
                             INT32 lockType ) ;

      INT32 renameCS( const CHAR *name, const CHAR *newName ) ;

      INT32 renameCL( const CHAR *csName, const CHAR *clName,
                      const CHAR *newCLName ) ;

   private:
      void _aquireMetaLock( OSS_LATCH_MODE lockType ) ;
      void _releaseMetaLock( OSS_LATCH_MODE lockType ) ;

   private:
      // Mutex to protect meta data change.
      ossSpinSLatch        _mutex ;
      UINT16               _number ;
      rtnExtDataProcessor  _processors[ RTN_EXT_PROCESSOR_MAX_NUM ] ;
      ossRWMutex           _processorLocks[ RTN_EXT_PROCESSOR_MAX_NUM ] ;
      rtnExtDataProcessor  *_pendingProcessor ;
   } ;
   typedef _rtnExtDataProcessorMgr rtnExtDataProcessorMgr ;

  rtnExtDataProcessorMgr* rtnGetExtDataProcessorMgr() ;
}

#endif /* RTN_EXTDATAPROCESSOR_HPP__ */

