/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnContextData.hpp

   Descriptive Name = RunTime Data Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_DATA_HPP_
#define RTN_CONTEXT_DATA_HPP_

#include "rtnContext.hpp"
#include "rtnQueryOptions.hpp"
#include "rtnQueryModifier.hpp"
#include "optAccessPlanRuntime.hpp"

namespace engine
{
   class _rtnIXScanner ;

   /*
      _rtnContextData define
   */
   class _rtnContextData : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
      public:
         _rtnContextData ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextData () ;

         _rtnIXScanner*    getIXScanner () { return _scanner ; }
         optScanType       scanType () const { return _scanType ; }
         _dmsMBContext*    getMBContext () { return _mbContext ; }

         dmsExtentID       lastExtLID () const { return _lastExtLID ; }

         virtual INT32 open( _dmsStorageUnit *su, _dmsMBContext *mbContext,
                             _pmdEDUCB *cb, const rtnReturnOptions &returnOptions,
                             const BSONObj *blockObj = NULL,
                             INT32 direction = 1 ) ;

         INT32 openTraversal( _dmsStorageUnit *su,
                              _dmsMBContext *mbContext,
                              _rtnIXScanner *scanner,
                              _pmdEDUCB *cb, const BSONObj &selector,
                              INT64 numToReturn = -1, INT64 numToSkip = 0 ) ;

         void setQueryModifier ( rtnQueryModifier* modifier ) ;

         OSS_INLINE virtual optAccessPlanRuntime * getPlanRuntime ()
         {
            return &_planRuntime ;
         }

         OSS_INLINE virtual const optAccessPlanRuntime * getPlanRuntime () const
         {
            return &_planRuntime ;
         }

         virtual void setQueryActivity ( BOOLEAN hitEnd ) ;

         OSS_INLINE const rtnReturnOptions & getReturnOptions () const
         {
            return _returnOptions ;
         }

      public:
         virtual std::string      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () { return _su ; }
         virtual BOOLEAN          isWrite() const ;

      protected:
         INT32 _queryModify( _pmdEDUCB* eduCB,
                             const dmsRecordID& recordID,
                             ossValuePtr recordDataPtr,
                             BSONObj& obj ) ;
         virtual INT32     _prepareData( _pmdEDUCB *cb ) ;
         virtual BOOLEAN   _canPrefetch () const
         {
            return ( _queryModifier ? FALSE : TRUE ) ;
         }
         virtual void      _toString( stringstream &ss ) ;

      protected:

         INT32    _prepareByTBScan( _pmdEDUCB *cb,
                                    DMS_ACCESS_TYPE accessType,
                                    vector<INT64>* dollarList ) ;
         INT32    _prepareByIXScan( _pmdEDUCB *cb,
                                    DMS_ACCESS_TYPE accessType,
                                    vector<INT64>* dollarList ) ;

         INT32    _parseSegments( const BSONObj &obj,
                                  std::vector< dmsExtentID > &segments ) ;
         INT32    _parseIndexBlocks( const BSONObj &obj,
                                    std::vector< BSONObj > &indexBlocks,
                                    std::vector< dmsRecordID > &indexRIDs ) ;
         INT32    _parseRID( const BSONElement &ele, dmsRecordID &rid ) ;

         INT32    _openTBScan ( _dmsStorageUnit *su,
                                _dmsMBContext *mbContext,
                                _pmdEDUCB *cb,
                                const BSONObj *blockObj ) ;
         INT32    _openIXScan ( _dmsStorageUnit *su,
                                _dmsMBContext *mbContext,
                                _pmdEDUCB *cb,
                                const BSONObj *blockObj,
                                INT32 direction ) ;

         INT32    _innerAppend( mthSelector *selector,
                                _mthRecordGenerator &generator ) ;
         INT32    _selectAndAppend( mthSelector *selector, BSONObj &obj ) ;

      protected:
         _SDB_DMSCB                 *_dmsCB ;
         _dmsStorageUnit            *_su ;
         _dmsMBContext              *_mbContext ;
         optAccessPlanRuntime       _planRuntime ;
         optScanType                _scanType ;

         SINT64                     _numToReturn ;
         SINT64                     _numToSkip ;
         rtnReturnOptions           _returnOptions ;

         dmsExtentID                _extentID ;
         dmsExtentID                _lastExtLID ;
         BOOLEAN                    _segmentScan ;
         std::vector< dmsExtentID > _segments ;
         _rtnIXScanner              *_scanner ;
         std::vector< BSONObj >     _indexBlocks ;
         std::vector< dmsRecordID > _indexRIDs ;
         BOOLEAN                    _indexBlockScan ;
         INT32                      _direction ;

         rtnQueryModifier*          _queryModifier ;
   } ;

   typedef _rtnContextData rtnContextData ;

   /*
      _rtnContextParaData define
   */
   class _rtnContextParaData : public _rtnContextData
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
      public:
         _rtnContextParaData( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextParaData () ;

         virtual INT32 open( _dmsStorageUnit *su, _dmsMBContext *mbContext,
                             _pmdEDUCB *cb,
                             const rtnReturnOptions &returnOptions,
                             const BSONObj *blockObj = NULL,
                             INT32 direction = 1 ) ;

      public:
         virtual std::string      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;

      protected:
         virtual INT32     _prepareData( _pmdEDUCB *cb ) ;
         virtual BOOLEAN   _canPrefetch () const { return FALSE ; }

         const BSONObj* _nextBlockObj () ;
         INT32          _checkAndPrefetch () ;
         INT32          _getSubContextData( _pmdEDUCB *cb ) ;
         INT32          _openSubContext( _pmdEDUCB *cb,
                                         const rtnReturnOptions &subReturnOptions,
                                         const BSONObj *blockObj ) ;
         void           _removeSubContext( rtnContextData *pContext ) ;
         INT32          _getSubCtxWithData ( rtnContextData **ppContext,
                                             _pmdEDUCB *cb ) ;

      protected:
         std::vector< _rtnContextData* >           _vecContext ;
         BOOLEAN                                   _isParalled ;
         BSONObj                                   _blockObj ;
         UINT32                                    _curIndex ;
         UINT32                                    _step ;
         rtnPrefWatcher                            _prefWather ;

   } ;
   typedef _rtnContextParaData rtnContextParaData ;

   /*
      _rtnContextTemp define
   */
   class _rtnContextTemp : public _rtnContextData
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
      public:
         _rtnContextTemp ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextTemp ();

      public:
         virtual std::string      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;

   } ;
   typedef _rtnContextTemp rtnContextTemp ;

}

#endif /* RTN_CONTEXT_DATA_HPP_ */

