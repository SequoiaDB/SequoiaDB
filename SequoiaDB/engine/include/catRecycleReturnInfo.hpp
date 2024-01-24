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

   Source File Name = catRecycleReturnInfo.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_RECYCLE_RETURN_INFO_HPP__
#define CAT_RECYCLE_RETURN_INFO_HPP__

#include "oss.hpp"
#include "catDef.hpp"
#include "catLevelLock.hpp"
#include "utilRecycleReturnInfo.hpp"
#include "pmdEDU.hpp"

namespace engine
{

   // pre-define
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;

   /*
      _catCheckCLInfo define
    */
   class _catCheckCLInfo : public _utilPooledObject
   {
   public:
      _catCheckCLInfo()
      : _uniqueID( UTIL_UNIQUEID_NULL ),
        _originName(),
        _returnName(),
        _renameMask( UTIL_RETURN_NO_RENAME )
      {
      }

      _catCheckCLInfo( utilCLUniqueID uniqueID,
                       utilReturnNameInfo nameInfo )
      : _uniqueID( uniqueID ),
        _originName( nameInfo.getOriginName() ),
        _returnName( nameInfo.getReturnName() ),
        _renameMask( nameInfo.getRenameMask() )
      {
      }

      _catCheckCLInfo( const _catCheckCLInfo &info )
      : _uniqueID( info._uniqueID ),
        _originName( info._originName ),
        _returnName( info._returnName ),
        _renameMask( info._renameMask )
      {
      }

      ~_catCheckCLInfo() {}

      utilCLUniqueID getUniqueID() const
      {
         return _uniqueID ;
      }

      const ossPoolString &getOriginName() const
      {
         return _originName ;
      }

      const ossPoolString &getReturnName() const
      {
         return _returnName ;
      }

      void setUniqueID( utilCLUniqueID uniqueID )
      {
         _uniqueID = uniqueID ;
      }

      UINT8 getRenameMask() const
      {
         return _renameMask ;
      }

      BOOLEAN isCLRenamed() const
      {
         return OSS_BIT_TEST( _renameMask, UTIL_RETURN_RENAMED_CL ) ;
      }

      BOOLEAN isRenamed() const
      {
         return UTIL_RETURN_NO_RENAME != _renameMask ;
      }

      _catCheckCLInfo &operator =( const _catCheckCLInfo &info )
      {
         _uniqueID = info._uniqueID ;
         _originName = info._originName ;
         _returnName = info._returnName ;
         _renameMask = info._renameMask ;
         return ( *this ) ;
      }

   protected:
      utilCLUniqueID _uniqueID ;
      ossPoolString  _originName ;
      ossPoolString  _returnName ;
      UINT8          _renameMask ;
   } ;

   typedef class _catCheckCLInfo catCheckCLInfo ;

   /*
      _catCheckSeqInfo define
    */
   class _catCheckSeqInfo : public _utilPooledObject
   {
   public:
      _catCheckSeqInfo()
      : _uniqueID( UTIL_UNIQUEID_NULL ),
        _name()
      {
      }

      _catCheckSeqInfo( utilSequenceID uniqueID,
                        const CHAR *name,
                        const CHAR *fieldName )
      : _uniqueID( uniqueID ),
        _name( name ),
        _fieldName( fieldName )
      {
      }

      _catCheckSeqInfo( const _catCheckSeqInfo &info )
      : _uniqueID( info._uniqueID ),
        _name( info._name )
      {
      }

      ~_catCheckSeqInfo() {}

      utilSequenceID getUniqueID() const
      {
         return _uniqueID ;
      }

      const ossPoolString &getName() const
      {
         return _name ;
      }

      const ossPoolString &getFieldName() const
      {
         return _fieldName ;
      }

      void setUniqueID( utilCLUniqueID uniqueID )
      {
         _uniqueID = uniqueID ;
      }

      void setName( const CHAR *name )
      {
         _name.assign( name ) ;
      }

      void setFieldName( const CHAR *fieldName )
      {
         _fieldName.assign( fieldName ) ;
      }

      _catCheckSeqInfo &operator =( const _catCheckSeqInfo &info )
      {
         _uniqueID = info._uniqueID ;
         _name = info._name ;
         _fieldName = info._fieldName ;
         return ( *this ) ;
      }

   protected:
      utilSequenceID _uniqueID ;
      ossPoolString  _name ;
      ossPoolString  _fieldName ;
   } ;

   typedef class _catCheckSeqInfo catCheckSeqInfo ;

   /*
      _catReturnConfig define
    */
   class _catReturnConfig : public _utilPooledObject
   {
   public:
      _catReturnConfig()
      : _isReturnToName( FALSE ),
        _isEnforced( FALSE ),
        _returnName( NULL )
      {
      }

      ~_catReturnConfig()
      {
      }

      BOOLEAN isReturnToName() const
      {
         return _isReturnToName ;
      }

      BOOLEAN isEnforced() const
      {
         return _isEnforced ;
      }

      const CHAR *getReturnName() const
      {
         return _returnName ;
      }

      void setReturnToName( BOOLEAN value )
      {
         _isReturnToName = value ;
      }

      void setEnforced( BOOLEAN value )
      {
         _isEnforced = value ;
      }

      void setReturnName( const CHAR *returnName )
      {
         _returnName = returnName ;
      }

      BOOLEAN isAllowRename() const
      {
         return _isReturnToName || _isEnforced ;
      }

   protected:
      BOOLEAN        _isReturnToName ;
      BOOLEAN        _isEnforced ;
      const CHAR *   _returnName ;
   } ;

   typedef class _catReturnConfig catReturnConfig ;

   /*
      _catRecycleReturnInfo define
    */
   class _catRecycleReturnInfo : public _utilRecycleReturnInfo
   {
   protected:
      typedef class _utilRecycleReturnInfo _BASE ;

   public:
      _catRecycleReturnInfo() ;
      ~_catRecycleReturnInfo() ;

      BOOLEAN isOnSiteReturn() const
      {
         return _isOnSite ;
      }

      void setOnSiteReturn( BOOLEAN isOnSite )
      {
         _isOnSite = isOnSite ;
      }

      BOOLEAN hasConflicts() const
      {
         return ( ( !( _conflictCSSet.empty() ) ) &&
                  ( !( _conflictNameCL.empty() ) ) &&
                  ( !( _conflictUIDCL.empty() ) ) ) ;
      }

      INT32 checkReturnCLToCS( const CHAR *clName,
                               utilCLUniqueID clUniqueID,
                               BOOLEAN allowRename,
                               pmdEDUCB *cb ) ;

      INT32 addConflictCS( const CHAR *csName ) ;
      INT32 addConflictNameCL( utilCLUniqueID clUniqueID,
                               const utilReturnNameInfo &clNameInfo ) ;
      INT32 addConflictUIDCL( utilCLUniqueID clUniqueID,
                              const CHAR *originName,
                              const CHAR *conflictName ) ;

      BOOLEAN isConflictNameCL( const CHAR *clName ) ;
      BOOLEAN isConflictUIDCL( utilCLUniqueID clUniqueID ) ;

      INT32 lockReplaceCS( catCtxLockMgr &lockMgr ) ;

      INT32 lockReplaceCL( catCtxLockMgr &lockMgr ) ;

      INT32 addRenameCSByRecyID( const CHAR *csName,
                                 utilRecycleID recycleID,
                                 catCtxLockMgr &lockMgr,
                                 pmdEDUCB *cb ) ;

      INT32 addRenameCLByRecyID( const catCheckCLInfo &checkInfo,
                                 utilRecycleID recycleID,
                                 catCtxLockMgr &lockMgr,
                                 pmdEDUCB *cb ) ;

      INT32 addChangeUIDCL( const ossPoolString &clName,
                            utilCLUniqueID clUniqueID,
                            catCtxLockMgr &lockMgr,
                            pmdEDUCB *cb,
                            INT16 w ) ;

      INT32 addMissingSubCL( const CHAR *subCLName,
                             const CHAR *mainCLName ) ;
      INT32 processMissingSubCL( pmdEDUCB *cb,
                                 _SDB_DMSCB *dmsCB,
                                 _dpsLogWrapper *dpsCB,
                                 INT16 w ) ;

      INT32 addMissingMainCL( const CHAR *mainCLName,
                              const CHAR *subCLName ) ;
      INT32 processMissingMainCL( pmdEDUCB *cb,
                                  _SDB_DMSCB *dmsCB,
                                  _dpsLogWrapper *dpsCB,
                                  INT16 w ) ;

      INT32 checkConflicts( const utilRecycleItem &item,
                            BOOLEAN isEnforced,
                            BOOLEAN isReturnToName,
                            catCtxLockMgr &lockMgr,
                            pmdEDUCB *cb,
                            INT16 w ) ;
      INT32 addConflictSeq( utilCLUniqueID clUniqueID,
                            utilSequenceID sequenceID,
                            const CHAR *seqName,
                            const CHAR *fieldName ) ;
      INT32 addRenameSeq( const CHAR *origSeqName,
                          const CHAR *renameSeqName ) ;
      BOOLEAN getRenameSeq( const CHAR *origSeqName,
                            const CHAR *&renameSeqName ) const ;
      INT32 addChangeUIDSeq( utilSequenceID origSeqUID,
                             utilSequenceID rtrnSeqUID ) ;
      BOOLEAN getChangeUIDSeq( utilSequenceID origSeqUID,
                               utilSequenceID &rtrnSeqUID ) const ;

      void resetExistDSSet() ;
      INT32 checkDSExist( UTIL_DS_UID dsUID, BOOLEAN &isExist, pmdEDUCB *cb ) ;

      BOOLEAN isMissingSubCL( const CHAR *subCLName ) const
      {
         return _isMissing( _missingSubCL, subCLName ) ;
      }

      BOOLEAN isMissingMainCL( const CHAR *mainCLName ) const
      {
         return _isMissing( _missingMainCL, mainCLName ) ;
      }

      BOOLEAN isMissingCL( const CHAR *clName ) const
      {
         return isMissingSubCL( clName ) || isMissingMainCL( clName ) ;
      }

      INT32 addRebuildIndex( const CHAR *clName,
                             const bson::BSONObj &options ) ;
      INT32 processRebuildIndex( pmdEDUCB *cb,
                                 _SDB_DMSCB *dmsCB,
                                 _dpsLogWrapper *dpsCB,
                                 INT16 w,
                                 ossPoolSet< UINT64 > &taskList ) ;

      INT32 checkCSDomain( utilCSUniqueID csUniqueID,
                           const bson::BSONObj &boSpace,
                           pmdEDUCB *cb ) ;
      INT32 addMissingDomain( const CHAR *domainName ) ;
      BOOLEAN isMissingDomain( const CHAR *domainName ) ;
      INT32 setDomainGroups( utilCSUniqueID csUniqueID,
                             const CHAR *domainName,
                             const CAT_GROUP_LIST &groups ) ;

      INT32 checkDomainGroup( utilCSUniqueID csUniqueID,
                              UINT32 groupID ) ;
      BOOLEAN isCSDomainChecked( utilCSUniqueID csUniqueID ) ;
      INT32 lockDomains( catCtxLockMgr &lockMgr ) ;

   protected:
      typedef ossPoolSet< utilCSUniqueID > _CAT_UID_CS_SET ;
      typedef ossPoolMap< ossPoolString, catCheckCLInfo > _CAT_NAME_CL_MAP ;
      typedef ossPoolMap< utilCLUniqueID, catCheckCLInfo > _CAT_UID_CL_MAP ;
      typedef ossPoolMap< ossPoolString, ossPoolString > _CAT_SUB_MAIN_MAP ;
      typedef ossPoolMultiMap< ossPoolString, ossPoolString > _CAT_MAIN_SUB_MAP ;
      typedef ossPoolMultiMap< utilCLUniqueID, catCheckSeqInfo > _CAT_CL_SEQ_MAP ;
      typedef ossPoolSet< UTIL_DS_UID > _CAT_DS_UID_SET ;
      typedef ossPoolMultiMap< ossPoolString, bson::BSONObj > _CAT_IDX_MAP ;
      typedef std::pair< ossPoolString, CAT_GROUP_SET > _CAT_DOMAIN_GROUPS ;
      typedef ossPoolMap< utilCSUniqueID, _CAT_DOMAIN_GROUPS > _CAT_CS_GROUP_SET ;
      typedef ossPoolMap< utilSequenceID, utilSequenceID > _CAT_RETURN_SEQ_MAP ;

      INT32 _checkConflictCS( const utilRecycleItem &item,
                              BOOLEAN isEnforced,
                              catCtxLockMgr &lockMgr,
                              pmdEDUCB *cb ) ;
      INT32 _checkConflictNameCL( const utilRecycleItem &item,
                                  BOOLEAN isEnforced,
                                  BOOLEAN isReturnToName,
                                  catCtxLockMgr &lockMgr,
                                  pmdEDUCB *cb ) ;
      INT32 _checkConflictUIDCL( const utilRecycleItem &item,
                                 BOOLEAN isEnforced,
                                 BOOLEAN isReturnToName,
                                 catCtxLockMgr &lockMgr,
                                 pmdEDUCB *cb,
                                 INT16 w ) ;

      BOOLEAN _isMissing( const _CAT_MAIN_SUB_MAP &missingMap,
                          const CHAR *mainCLName ) const ;

      BOOLEAN _isMissing( const _CAT_SUB_MAIN_MAP &missingMap,
                          const CHAR *subCLName ) const ;

   protected:
      // indicates return in the origin place
      // e.g. for truncated collection without version and data changes
      BOOLEAN              _isOnSite ;

      // return info for collection spaces
      _CAT_UID_CS_SET      _checkCSSet ;

      // conflict collection spaces
      UTIL_RETURN_NAME_SET _conflictCSSet ;

      // conflict collections by name
      _CAT_NAME_CL_MAP     _conflictNameCL ;
      // conflict collections by unique ID
      _CAT_UID_CL_MAP      _conflictUIDCL ;

      // missing sub-collections
      // NOTE: origin names, NOT return names
      _CAT_SUB_MAIN_MAP    _missingSubCL ;
      // missing main-collections
      // NOTE: origin names, NOT return names
      _CAT_MAIN_SUB_MAP    _missingMainCL ;

      // conflict sequences caused by conflict collections
      _CAT_CL_SEQ_MAP      _conflictSeq ;
      // sequences need to rename
      UTIL_RETURN_NAME_MAP _renameSeq ;
      // sequences need to change unique ID
      _CAT_RETURN_SEQ_MAP  _changeUIDSeq ;

      _CAT_DS_UID_SET      _existDSSet ;
      _CAT_DS_UID_SET      _missingDSSet ;

      _CAT_IDX_MAP         _rebuildIndexMap ;

      // collection space unique ID -> ( domain name, domain groups )
      // Note: "" for system domain
      _CAT_CS_GROUP_SET    _csDomainGroups ;
      // domain is missing
      UTIL_RETURN_NAME_SET _missingDomains ;
   } ;

   typedef class _catRecycleReturnInfo catRecycleReturnInfo ;

}

#endif // CAT_RECYCLE_RETURN_INFO_HPP__
