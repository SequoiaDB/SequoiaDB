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
#include "rtnResultSetFilter.hpp"
#include "optAccessPlanRuntime.hpp"
#include "rtnScanner.hpp"

namespace engine
{

   class _rtnTBScanner ;
   class _rtnIXScanner ;
   class _dpsITransLockCallback ;

   typedef struct _rtnAdvanceSection
   {
      INT32   prefixNum ;
      BOOLEAN startIncluded ;
      BOOLEAN endIncluded ;
      BSONObj startKey ;
      BSONObj endKey ;
   } rtnAdvanceSection ;

   class _rtnCmpSection
   {
   public:
      _rtnCmpSection( const BSONObj &order ) : _orderBy( order ) { }
      bool operator()( const rtnAdvanceSection &l, const rtnAdvanceSection &r )
      {
         INT32 cmpStart = 0 ;
         INT32 cmpEnd   = 0 ;
         INT32 minNum   = l.prefixNum > r.prefixNum ?
                                        r.prefixNum : l.prefixNum ;

         // Compare the start key of minimun prefix num.
         cmpStart = woNCompare( l.startKey, r.startKey, minNum, _orderBy ) ;

         if ( cmpStart < 0 )
         {
            return TRUE ;
         }
         else if ( 0 == cmpStart )
         {
            /*
              There are two cases of unequal prefix num, the sorted section is:
                left section => right section
              1. left prefix num less than right prefix num and left start
                 include is true. eg:
                   left section:
                   {
                     prefix num : 2,
                     start key : { "1" : 3, "2": 1999 },
                     start include : true
                   }
                   right section:
                   {
                     prefix num : 3,
                     start key : { "1" : 3, "2": 1999 , "3": 18},
                     start include : -
                   }
              2. left prefix num greater than right prefix num and right start
                 include is false. eg:
                   left section:
                   {
                     prefix num : 3,
                     start key : { "1" : 3, "2": 1999 , "3": 18},
                     start include : -
                   }
                   right section:
                   {
                     prefix num : 2,
                     start key : { "1" : 3, "2": 1999 },
                     start include : false
                   }
            */
            if ( l.prefixNum < r.prefixNum )
            {
               if ( l.startIncluded )
               {
                  return TRUE;      
               }
            }
            else if ( l.prefixNum > r.prefixNum )
            {
               if ( !r.startIncluded )
               {
                  return TRUE;
               }
            }
            else
            {
               if ( l.startIncluded && !r.startIncluded )
               {
                  return TRUE ;
               }
               else if ( l.startIncluded == r.startIncluded )
               {
                  cmpEnd = woNCompare( l.endKey, r.endKey, minNum, _orderBy ) ;
                  if ( cmpEnd > 0 )
                  {
                     return TRUE ;
                  }
                  else if ( 0 == cmpEnd )
                  {
                     if( l.endIncluded && !r.endIncluded )
                     {
                        return TRUE ;
                     }
                  }
               }
            }
         }
         return FALSE ;
      }

      INT32 woNCompare( const BSONObj &l, const BSONObj &r,
                        UINT32 keyNum, const BSONObj &orderBy) const ;

   private:
      const BSONObj _orderBy ;
   } ;
   typedef class _rtnCmpSection rtnCmpSection ;

   /*
      _rtnContextData define
   */
   class _rtnContextData : public _rtnContextBase, public _IMthMatchEventHandler
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextData )

      typedef ossPoolVector< dmsExtentID >      SEGMENT_VEC ;
      typedef SEGMENT_VEC::const_iterator       SEGMENT_VEC_CITR ;

      public:
         _rtnContextData ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextData () ;

         _rtnTBScanner*    getTBScanner () ;
         _rtnIXScanner*    getIXScanner () ;
         optScanType       scanType () const { return _scanType ; }
         _dmsMBContext*    getMBContext () { return _mbContext ; }

         const dmsRecordID &lastRID () const { return _recordID ; }

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

         OSS_INLINE BOOLEAN isIndexCover() const
         {
            return _indexCover ;
         }

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () { return _su ; }
         virtual BOOLEAN          isWrite() const ;
         virtual BOOLEAN          needRollback() const ;
         virtual const CHAR *     getProcessName() const
         {
            return _planRuntime.getCLFullName() ?
                   _planRuntime.getCLFullName() : "" ;
         }

         virtual UINT32 getSULogicalID() const
         {
            return _suLogicalID ;
         }

         virtual void setResultSetFilter( rtnResultSetFilter *rsFilter,
                                          BOOLEAN appendMode = TRUE ) ;

         INT32   setAdvanceSection ( const BSONObj &arg ) ;

         virtual INT32 validate ( const BSONObj &record ) ;
         virtual INT32 onMatchDone( const bson::BSONObj &record,
                                    BOOLEAN isMatched )
         {
            return ( isMatched ) ? ( SDB_OK ) : ( validate( record ) ) ;
         }

      protected:
         INT32 _queryModify( _pmdEDUCB* eduCB,
                             const dmsRecordID& recordID,
                             ossValuePtr recordDataPtr,
                             BSONObj& obj,
                             IDmsOprHandler* pHandler,
                             const dmsTransRecordInfo *pInfo ) ;
         virtual INT32     _prepareData( _pmdEDUCB *cb ) ;
         virtual BOOLEAN   _canPrefetch () const
         {
            // If contain modifier, do not use prefetch
            return ( _scanner && _scanner->canPrefetch() ) && ( _queryModifier ? FALSE : TRUE ) ;
         }
         virtual void      _toString( stringstream &ss ) ;

         virtual INT32     _doAdvance( INT32 type,
                                       INT32 prefixNum,
                                       const BSONObj &keyVal,
                                       const BSONObj &orderby,
                                       const BSONObj &arg,
                                       BOOLEAN isLocate,
                                       _pmdEDUCB *cb ) ;

         virtual INT32     _getAdvanceOrderby( BSONObj &orderby,
                                       BOOLEAN isRange = FALSE ) const ;

         virtual INT32     _prepareDoAdvance ( _pmdEDUCB *cb ) ;

      protected:

         INT32    _prepareByTBScan( _rtnTBScanner *tbScanner,
                                    _pmdEDUCB *cb,
                                    DMS_ACCESS_TYPE accessType,
                                    vector<INT64>* dollarList ) ;
         INT32    _prepareByIXScan( _rtnIXScanner *ixScanner,
                                    _pmdEDUCB *cb,
                                    DMS_ACCESS_TYPE accessType,
                                    vector<INT64>* dollarList ) ;

         INT32    _parseSegments( const BSONObj &obj,
                                  SEGMENT_VEC &segments ) ;
         INT32    _parseIndexBlocks( const BSONObj &obj,
                                    std::vector< BSONObj > &indexBlocks,
                                    std::vector< dmsRecordID > &indexRIDs ) ;
         INT32    _parseRID( const BSONElement &ele, dmsRecordID &rid ) ;

         INT32    _openTBScan ( _dmsStorageUnit *su,
                                _dmsMBContext *mbContext,
                                _pmdEDUCB *cb,
                                const rtnReturnOptions &returnOptions,
                                const BSONObj *blockObj ) ;
         INT32    _openIXScan ( _dmsStorageUnit *su,
                                _dmsMBContext *mbContext,
                                _pmdEDUCB *cb,
                                const rtnReturnOptions &returnOptions,
                                const BSONObj *blockObj,
                                INT32 direction ) ;

         INT32    _innerAppend( mthSelector *selector,
                                _mthRecordGenerator &generator ) ;
         INT32    _selectAndAppend( mthSelector *selector, BSONObj &obj ) ;

         INT32    _evalIndexCover( IXM_FIELD_NAME_SET &selectSet ) ;

         BSONObj  _buildNextValueObj( const BSONObj &keyPattern,
                                      const BSONObj &srcVal,
                                      UINT32 keepNum,
                                      INT32 type ) const ;
         void     _buildNextRID( INT32 type, dmsRecordID &rid ) const ;
         BOOLEAN  _compareFieldName( const BSONObj &orderby,
                                     const BSONObj &keyPattern,
                                     UINT32 prefixNum ) const ;

         INT32    _extractAllEqualSec( INT32 indexFieldNum,
                                       const BSONElement &eNum,
                                       const BSONElement &eVal );

         INT32    _extractRangeSec( INT32 indexFieldNum,
                                    const BSONElement &eNum,
                                    const BSONElement &eVal,
                                    const BSONElement &eIndexValueInc );

         BOOLEAN  _needValidate() const
         {
            return !( _advanceSectionList.empty() ) ;
         }

      protected:
         _SDB_DMSCB                 *_dmsCB ;
         _dmsStorageUnit            *_su ;
         UINT32                     _suLogicalID ;
         _dmsMBContext              *_mbContext ;
         optAccessPlanRuntime       _planRuntime ;
         optScanType                _scanType ;

         // rest number of records to expect, -1 means select all
         SINT64                     _numToReturn ;
         // rest number of records need to skip
         SINT64                     _numToSkip ;
         // Original return options, number of skip, etc.
         rtnReturnOptions           _returnOptions ;

         _rtnScanner               *_scanner ;

         // TBSCAN
         dmsRecordID                _recordID ;
         BOOLEAN                    _segmentScan ;
         SEGMENT_VEC                _segments ;
         // Index scan
         std::vector< BSONObj >     _indexBlocks ;
         std::vector< dmsRecordID > _indexRIDs ;
         BOOLEAN                    _indexBlockScan ;
         INT32                      _direction ;

         rtnResultSetFilter         *_rsFilter ;
         BOOLEAN                    _appendRIDFilter ;

         // query modify
         rtnQueryModifier*          _queryModifier ;

         BOOLEAN                    _indexCover ;

         BOOLEAN                    _isPrevSec ;

         BSONObj                    _orderBy ;
         ixmIndexKeyGen             _keyGen ;

         ossPoolList< rtnAdvanceSection > _advanceSectionList ;
         ossPoolList< rtnAdvanceSection >::iterator _nextAdvanceSecIt ;
   } ;

   typedef _rtnContextData rtnContextData ;

   /*
      _rtnContextParaData define
   */
   class _rtnContextParaData : public _rtnContextData
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextParaData )
      public:
         _rtnContextParaData( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextParaData () ;

         virtual INT32 open( _dmsStorageUnit *su, _dmsMBContext *mbContext,
                             _pmdEDUCB *cb,
                             const rtnReturnOptions &returnOptions,
                             const BSONObj *blockObj = NULL,
                             INT32 direction = 1 ) ;

      public:
         virtual const CHAR*      name() const ;
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
         virtual INT32     _doAdvance( INT32 type,
                                       INT32 prefixNum,
                                       const BSONObj &keyVal,
                                       const BSONObj &orderby,
                                       const BSONObj &arg,
                                       BOOLEAN isLocate,
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
   class _rtnContextTemp : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextTemp )
      public:
         _rtnContextTemp ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextTemp ();

         INT32 open( UINT32 suLID,
                     UINT32 mbLID,
                     const CHAR *csName,
                     const CHAR *clShortName,
                     const CHAR *optrDesc ) ;

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;

         virtual _dmsStorageUnit *getSU()
         {
            return NULL ;
         }

         virtual UINT32 getSULogicalID() const
         {
            return _suLID ;
         }

         virtual const CHAR *getProcessName() const
         {
            return _processName.c_str() ;
         }

      protected:
         virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
         virtual void _toString( stringstream &ss ) ;

      protected:
         // logical ID of collection space which is being processed
         UINT32 _suLID ;
         // logical ID of collection which is being processed
         UINT32 _mbLID ;
         // name of the collection or collection space which is being processed
         ossPoolString _processName ;
         // description of operation which is processing the collection or
         // collection space
         ossPoolString _optrDesc ;
   } ;
   typedef _rtnContextTemp rtnContextTemp ;

}

#endif /* RTN_CONTEXT_DATA_HPP_ */
