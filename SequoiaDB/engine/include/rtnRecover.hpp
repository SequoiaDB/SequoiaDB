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

   Source File Name = rtnRecover.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/08/2016  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_RECOVER_HPP__
#define RTN_RECOVER_HPP__

#include "dmsStorageUnit.hpp"
#include "pmdEDU.hpp"
#include <string>

#include "../bson/bson.h"
using namespace bson ;

namespace engine
{

   /*
      _rtnRUInfo define
   */
   struct _rtnRUInfo
   {
      UINT32      _dataCommitFlag ;
      UINT32      _idxCommitFlag ;
      UINT32      _lobCommitFlag ;
      UINT64      _dataCommitLSN ;
      UINT64      _idxCommitLSN ;
      UINT64      _lobCommitLSN ;

      CHAR        _clName[ DMS_COLLECTION_NAME_SZ + 1 ] ;

      _rtnRUInfo()
      {
         _dataCommitFlag = 0 ;
         _idxCommitFlag = 0 ;
         _lobCommitFlag = 0 ;
         _dataCommitLSN = 0 ;
         _idxCommitLSN = 0 ;
         _lobCommitLSN = 0 ;
         _clName[0] = 0 ;
      }

      BOOLEAN isAllValid() const
      {
         if ( 0 == _dataCommitFlag ||
              0 == _idxCommitFlag ||
              0 == _lobCommitFlag )
         {
            return FALSE ;
         }
         return TRUE ;
      }

      void  setAllInvalid()
      {
         _dataCommitFlag = 0 ;
         _idxCommitFlag = 0 ;
         _lobCommitFlag = 0 ;
      }

      void setAllValid()
      {
         _dataCommitFlag = 1 ;
         _idxCommitFlag = 1 ;
         _lobCommitFlag = 1 ;
      }

      UINT64 maxLSN() const
      {
         UINT64 lsn = _dataCommitLSN ;
         if ( DPS_INVALID_LSN_OFFSET != _idxCommitLSN &&
              _idxCommitLSN > lsn )
         {
            lsn = _idxCommitLSN ;
         }
         if ( DPS_INVALID_LSN_OFFSET != _lobCommitLSN &&
              _lobCommitLSN > lsn )
         {
            lsn = _lobCommitLSN ;
         }
         return lsn ;
      }

      UINT64 maxValidLSN() const
      {
         UINT64 lsn = DPS_INVALID_LSN_OFFSET ;
         if ( 1 == _dataCommitFlag && DPS_INVALID_LSN_OFFSET != _dataCommitLSN )
         {
            if ( DPS_INVALID_LSN_OFFSET == lsn || lsn < _dataCommitLSN )
            {
               lsn = _dataCommitLSN ;
            }
         }

         if ( 1 == _idxCommitFlag && DPS_INVALID_LSN_OFFSET != _idxCommitLSN )
         {
            if ( DPS_INVALID_LSN_OFFSET == lsn || lsn < _idxCommitLSN )
            {
               lsn = _idxCommitLSN ;
            }
         }

         if ( 1 == _lobCommitFlag && DPS_INVALID_LSN_OFFSET != _lobCommitLSN )
         {
            if ( DPS_INVALID_LSN_OFFSET == lsn || lsn < _lobCommitLSN )
            {
               lsn = _lobCommitLSN ;
            }
         }

         return lsn ;
      }
   } ;
   typedef _rtnRUInfo rtnRUInfo ;

   // Base class of collection rebuilder.
   class _rtnCLRebuilderBase : public SDBObject
   {
      public:
         _rtnCLRebuilderBase( dmsStorageUnit *pSU,
                              const CHAR *pCLShortName ) ;
         virtual ~_rtnCLRebuilderBase() ;

         virtual INT32 rebuild( pmdEDUCB *cb, rtnRUInfo *ruInfo ) ;
         virtual INT32 recover( pmdEDUCB *cb ) ;
         virtual INT32 reorg( pmdEDUCB *cb, const BSONObj &hint ) ;

      protected:
         virtual INT32 _doRebuild( dmsMBContext *context, pmdEDUCB *cb,
                                   rtnRUInfo *ruInfo ) = 0 ;
         virtual INT32 _onRebuildDone() = 0 ;
         virtual INT32 _doRecover( dmsMBContext *context, pmdEDUCB *cb ) = 0 ;
         virtual INT32 _onRecoverDone() = 0 ;
         virtual INT32 _doReorg( dmsMBContext *context, pmdEDUCB *cb,
                                 const BSONObj &hint ) = 0 ;
         virtual INT32 _onReorgDone() = 0 ;

      private:
         void _release() ;

      protected:
         dmsStorageUnit       *_pSU ;
         string               _clName ;
         string               _clFullName ;

         /// stat info
         UINT64               _totalRecord ;
         UINT64               _totalLob ;
         UINT32               _indexNum ;
   } ;
   typedef _rtnCLRebuilderBase rtnCLRebuilderBase ;

   /*
      _rtnCLRebuilder define
   */
   class _rtnCLRebuilder : public rtnCLRebuilderBase
   {
      public:
         _rtnCLRebuilder( dmsStorageUnit *pSU,
                          const CHAR *pCLShortName ) ;
         ~_rtnCLRebuilder() ;

      protected:
         INT32    _rebuild( pmdEDUCB *cb,
                            dmsMBContext *mbContext,
                            rtnRUInfo *ruInfo ) ;

         INT32    _recover( pmdEDUCB *cb,
                            dmsMBContext *mbContext ) ;

         /*
            Rebuild data records
         */
         INT32    _rebuildData( pmdEDUCB *cb,
                                dmsMBContext *mbContext ) ;

         /*
            Reubild index
         */
         INT32    _rebuildIndex( pmdEDUCB *cb,
                                 dmsMBContext *mbContext ) ;
         /*
            Rebuild lob
         */
         INT32    _rebuildLob( pmdEDUCB *cb,
                               dmsMBContext *mbContext ) ;

         /*
            Reorg data records
         */
         INT32    _reorgData( pmdEDUCB *cb,
                              dmsMBContext *mbContext,
                              const BSONObj &hint ) ;
      private:
         virtual INT32 _doRebuild( dmsMBContext *context, pmdEDUCB *cb,
                                   rtnRUInfo *ruInfo ) ;
         virtual INT32 _onRebuildDone() ;
         virtual INT32 _doRecover( dmsMBContext *context, pmdEDUCB *cb ) ;
         virtual INT32 _onRecoverDone() ;
         virtual INT32 _doReorg( dmsMBContext *context, pmdEDUCB *cb,
                                 const BSONObj &hint ) ;
         virtual INT32 _onReorgDone() ;
   } ;
   typedef _rtnCLRebuilder rtnCLRebuilder ;

   class _rtnCappedCLRebuilder : public rtnCLRebuilderBase
   {
      public:
         _rtnCappedCLRebuilder( dmsStorageUnit *pSU,
                                const CHAR *pCLShortName ) ;
         ~_rtnCappedCLRebuilder() ;

      private:
         virtual INT32 _doRebuild( dmsMBContext *context, pmdEDUCB *cb,
                                   rtnRUInfo *ruInfo ) ;
         virtual INT32 _onRebuildDone() ;
         virtual INT32 _doRecover( dmsMBContext *context, pmdEDUCB *cb ) ;
         virtual INT32 _onRecoverDone() ;
         virtual INT32 _doReorg( dmsMBContext *context, pmdEDUCB *cb,
                                 const BSONObj &hint ) ;
         virtual INT32 _onReorgDone() ;

         INT32 _rebuildData( dmsMBContext *context, pmdEDUCB *cb ) ;
         INT32 _rebuildLob( dmsMBContext *context, pmdEDUCB *cb ) ;
   } ;
   typedef _rtnCappedCLRebuilder rtnCappedCLRebuilder ;

   class _rtnCLRebuilderFactory : public SDBObject
   {
      public:
         _rtnCLRebuilderFactory() ;
         ~_rtnCLRebuilderFactory() ;

         INT32 create( dmsStorageUnit *pSU,
                       const CHAR *pCLShortName,
                       rtnCLRebuilderBase *&rebuilder) ;
         void release( rtnCLRebuilderBase *rebuilder ) ;
   } ;
   typedef _rtnCLRebuilderFactory rtnCLRebuilderFactory ;

   rtnCLRebuilderFactory* rtnGetCLRebuilderFactory() ;

   /*
      Type define
   */
   typedef map< string, rtnRUInfo >          MAP_SU_STATUS ;

   /*
      _rtnRecoverUnit
   */
   class _rtnRecoverUnit : public SDBObject
   {
      public:
         _rtnRecoverUnit() ;
         ~_rtnRecoverUnit() ;

         INT32       init( dmsStorageUnit *pSu ) ;

         void        release() ;

         /*
            Drop the invalid collections
            If all collection is invalid, will drop the collectionspace
         */
         INT32       cleanup( pmdEDUCB *cb ) ;
         /*
            Rebuild the invlaid collections
         */
         INT32       rebuild( pmdEDUCB *cb ) ;
         /*
            Restore by Journal
         */
         INT32       restore( pmdEDUCB *cb ) ;

         /*
            Is all the collectionspace valid
         */
         BOOLEAN     isAllValid() const ;
         BOOLEAN     isAllInvalid() const ;

         /*
            Get all valid collection's status map
         */
         UINT32      getValidCLItem( MAP_SU_STATUS &items ) ;

         /*
            Get all invalid collection's status map
         */
         UINT32      getInvalidCLItem( MAP_SU_STATUS &items ) ;

         /*
            Get all collection's status map
         */
         UINT32      getCLItems( MAP_SU_STATUS &item ) ;

         rtnRUInfo*  getItem( const string &name ) ;

         dmsStorageUnit* getSU() { return _pSU ; }

         void        setAllInvalid() ;

         UINT64      getMaxLsn() const { return _maxLsn ; }

         DPS_LSN_OFFSET getMaxValidLsn() { return _maxValidLsn ; }

      protected:

      private:
         MAP_SU_STATUS              _clStatus ;

         dmsStorageUnit             *_pSU ;
         UINT64                     _maxLsn ;
         DPS_LSN_OFFSET             _maxValidLsn ;

   } ;
   typedef _rtnRecoverUnit rtnRecoverUnit ;

   /*
      _rtnDBOprBase define
   */
   class _rtnDBOprBase : public SDBObject
   {
      public:
         _rtnDBOprBase() {}
         virtual ~_rtnDBOprBase() {}

         INT32       doOpr( pmdEDUCB *cb ) ;

      public:
         virtual const CHAR*  oprName() const = 0 ;

      protected:
         virtual BOOLEAN   _lockDMS() const { return TRUE ; }
         virtual BOOLEAN   _cleanDPS() const { return TRUE ; }

         virtual INT32     _onBegin( pmdEDUCB *cb ) { return SDB_OK ; }

         virtual INT32     _doOpr( pmdEDUCB *cb,
                                   rtnRecoverUnit *pUnit,
                                   dmsStorageUnitID &suID ) = 0 ;

         virtual void      _onSucceed( pmdEDUCB *cb ) {}

      private:
         INT32             _rewriteCommitLSN( _SDB_DMSCB *dmsCB,
                                              MON_CS_SIM_LIST &csList,
                                              DPS_LSN_OFFSET dpsMaxLSN ) ;

         INT32             _rewriteCLCommitLSN( _SDB_DMSCB *dmsCB,
                                                dmsStorageUnit *su,
                                                MAP_SU_STATUS &validCLs,
                                                DPS_LSN_OFFSET dpsMaxLSN ) ;

   } ;

   /*
      _rtnDBRebuilder define
   */
   class _rtnDBRebuilder : public _rtnDBOprBase
   {
      public:
         _rtnDBRebuilder() {}
         virtual ~_rtnDBRebuilder() {}

      public:
         virtual const CHAR*  oprName() const { return "Rebuild" ; }

      protected:
         virtual BOOLEAN   _lockDMS() const { return TRUE ; }
         virtual BOOLEAN   _cleanDPS() const { return TRUE ; }

         virtual INT32     _onBegin( pmdEDUCB *cb ) { return SDB_OK ; }

         virtual INT32     _doOpr( pmdEDUCB *cb,
                                   rtnRecoverUnit *pUnit,
                                   dmsStorageUnitID &suID ) ;

         virtual void      _onSucceed( pmdEDUCB *cb ) ;

   } ;
   typedef _rtnDBRebuilder rtnDBRebuilder ;

   /*
      _rtnDBCleaner define
   */
   class _rtnDBCleaner : public _rtnDBOprBase
   {
      public:
         _rtnDBCleaner() { _useUDF = FALSE ; }
         virtual ~_rtnDBCleaner() {}

         void     setUDFValidCLs( const vector<string> &vecValidCLs ) ;
         const vector<string>& getUDFValidCLs() const { return _udfValidCLs ; }

      public:
         virtual const CHAR*  oprName() const { return "Cleanup" ; }

      protected:
         virtual BOOLEAN   _lockDMS() const { return FALSE ; }
         virtual BOOLEAN   _cleanDPS() const { return FALSE ; }

         virtual INT32     _onBegin( pmdEDUCB *cb ) { return SDB_OK ; }

         virtual INT32     _doOpr( pmdEDUCB *cb,
                                   rtnRecoverUnit *pUnit,
                                   dmsStorageUnitID &suID ) ;

         virtual void      _onSucceed( pmdEDUCB *cb ) {}

         void              _removeCLsByCS( const CHAR *csName ) ;

      private:
         vector<string>             _udfValidCLs ;
         BOOLEAN                    _useUDF ;
   } ;
   typedef _rtnDBCleaner rtnDBCleaner ;

   /*
      _rtnDBFSPostCleaner define
   */
   class _rtnDBFSPostCleaner : public _rtnDBOprBase
   {
      public:
         _rtnDBFSPostCleaner() {}
         virtual ~_rtnDBFSPostCleaner() {}

      public:
         virtual const CHAR*  oprName() const { return "Repaire" ; }

      protected:
         virtual BOOLEAN   _lockDMS() const { return FALSE ; }
         virtual BOOLEAN   _cleanDPS() const { return FALSE ; }

         virtual INT32     _onBegin( pmdEDUCB *cb ) { return SDB_OK ; }

         virtual INT32     _doOpr( pmdEDUCB *cb,
                                   rtnRecoverUnit *pUnit,
                                   dmsStorageUnitID &suID ) ;

         virtual void      _onSucceed( pmdEDUCB *cb ) ;
   } ;
   typedef _rtnDBFSPostCleaner rtnDBFSPostCleaner ;

}

#endif /* RTN_RECOVER_HPP__ */

