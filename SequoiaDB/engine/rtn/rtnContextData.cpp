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

   Source File Name = rtnContextData.cpp

   Descriptive Name = RunTime Data Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.cpp

   Last Changed =

*******************************************************************************/
#include "rtnContextData.hpp"
#include "bps.hpp"
#include "ossUtil.hpp"
#include "rtn.hpp"
#include "pmd.hpp"
#include "rtnScannerFactory.hpp"
#include "rtnIXScanner.hpp"
#include "rtnTBScanner.hpp"
#include "dpsTransLockCallback.hpp"
#include "dmsScanner.hpp"
#include "dmsCB.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsStorageDataCommon.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "pmdController.hpp"

using namespace bson ;

namespace engine
{

   INT32 _rtnCmpSection::woNCompare( const BSONObj &l,
                                     const BSONObj &r, UINT32 keyNum,
                                     const BSONObj &keyPattern ) const
   {
      BSONObjIterator itrL( l ) ;
      BSONObjIterator itrR( r ) ;
      BSONObjIterator itrK( keyPattern ) ;
      UINT32 i = 0 ;
      INT32 cmp = 0 ;
      BOOLEAN ordered = !keyPattern.isEmpty() ;

      for ( i = 0 ; i < keyNum && itrL.more() && itrR.more() ; ++i )
      {
        BSONElement eL = itrL.next() ;
        BSONElement eR = itrR.next() ;

        BSONElement eK ;
        if ( ordered )
        {
          if ( itrK.more() )
          {
            eK = itrK.next() ;
          }
          else
          {
            SDB_ASSERT( FALSE, "Key pattern is invalid" ) ;
            ordered = FALSE ;
          }
        }

        cmp = eL.woCompare( eR, FALSE ) ;
        if ( 0 != cmp )
        {
          if ( ordered && eK.numberInt() < 0 )
          {
            cmp = -cmp ;
          }
          return cmp ;
        }
      }

      if ( i < keyNum )
      {
        if ( itrL.more() )
        {
          cmp = 1 ;
        }
        else
        {
          cmp = -1 ;
        }
      }
      return cmp ;
   }

   /*
      _rtnContextData implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextData, RTN_CONTEXT_DATA, "DATA")

   _rtnContextData::_rtnContextData( INT64 contextID, UINT64 eduID )
   : _rtnContextBase( contextID, eduID )
   {
      _dmsCB            = NULL ;
      _su               = NULL ;
      _suLogicalID      = DMS_INVALID_LOGICCSID ;
      _mbContext        = NULL ;
      _scanType         = UNKNOWNSCAN ;
      _numToReturn      = -1 ;
      _numToSkip        = 0 ;

      _segmentScan      = FALSE ;
      _indexBlockScan   = FALSE ;
      _scanner          = NULL ;
      _direction        = 0 ;
      _queryModifier    = NULL ;
      _indexCover       = FALSE ;

      // Save query activity
      _enableMonContext = TRUE ;
      _enableQueryActivity = TRUE ;
      _rsFilter         = NULL ;
      _appendRIDFilter  = FALSE ;
      _isPrevSec        = FALSE ;
   }

   _rtnContextData::~_rtnContextData ()
   {
      rtnScannerFactory f ;
      f.releaseScanner( _scanner ) ;
      _scanner = NULL ;

      // first release plan
      setQueryActivity( _hitEnd ) ;
      _planRuntime.reset() ;

      // second release mb context
      if ( _mbContext && _su )
      {
         _su->data()->releaseMBContext( _mbContext ) ;
      }
      // last unlock su
      if ( _dmsCB && _su && -1 != contextID() )
      {
         _dmsCB->suUnlock ( _su->CSID() ) ;
      }
      // query modifier
      if ( _queryModifier )
      {
         SDB_OSS_DEL _queryModifier ;
         _queryModifier = NULL ;
         _dmsCB->writeDown( pmdGetThreadEDUCB() ) ;
      }
      _isPrevSec = FALSE ;
   }

   _rtnTBScanner* _rtnContextData::getTBScanner ()
   {
      return dynamic_cast<_rtnTBScanner *>( _scanner ) ;
   }

   _rtnIXScanner* _rtnContextData::getIXScanner ()
   {
      return dynamic_cast<_rtnIXScanner *>( _scanner ) ;
   }

   const CHAR* _rtnContextData::name() const
   {
      return "DATA" ;
   }

   RTN_CONTEXT_TYPE _rtnContextData::getType() const
   {
      return RTN_CONTEXT_DATA ;
   }

   BOOLEAN _rtnContextData::isWrite() const
   {
      return _queryModifier ? TRUE : FALSE ;
   }

   BOOLEAN _rtnContextData::needRollback() const
   {
      return isWrite() ;
   }

   void _rtnContextData::setResultSetFilter( rtnResultSetFilter *rsFilter,
                                             BOOLEAN appendMode )
   {
      _rsFilter = rsFilter ;
      _appendRIDFilter = appendMode ;
   }

   INT32 _rtnContextData::_getAdvanceOrderby( BSONObj &orderby,
                                              BOOLEAN isRange ) const
   {
      INT32 rc = SDB_OK ;

      /// not ixscan
      if ( IXSCAN != scanType() )
      {
         PD_LOG_MSG( PDERROR, "Table scan does not support advance" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
      }
      else if ( !isRange && _planRuntime.getPlan()->sortRequired() )
      {
         PD_LOG_MSG( PDERROR, "Orderby is not the same with index" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
      }
      else
      {
         orderby = _planRuntime.getPlan()->getKey().getOrderBy() ;

         if ( orderby.isEmpty() )
         {
            if ( isRange )
            {
               orderby = _planRuntime.getPlan()->getKeyPattern() ;
               SDB_ASSERT( !orderby.isEmpty(), "orderby should not be empty!" ) ;
               if ( -1 == _planRuntime.getPlan()->getDirection() )
               {
                  try
                  {
                     BSONObjBuilder ob ;
                     BSONObjIterator it( orderby ) ;
                     INT32 value = 0 ;
                     while ( it.more() )
                     {
                        BSONElement ele = it.next() ;
                        value = - ele.Int() ;
                        ob.append( ele.fieldName(), value ) ;
                     }
                     orderby = ob.obj() ;
                  }
                  catch( std::exception &e )
                  {
                     rc = ossException2RC( &e ) ;
                     PD_LOG ( PDERROR, "Failed to generate orderby bson, occur "
                              "exception: %s, rc: %d", e.what(), rc ) ;
                  }
               }
            }
            else
            {
               PD_LOG_MSG( PDERROR, "Context does not support advance without "
                           "orderby" ) ;
               rc = SDB_OPTION_NOT_SUPPORT ;
            }
         }
      }

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXDATA__PREPRAREDOADVANCE, "_rtnContextData::_prepareDoAdvance" )
   INT32 _rtnContextData::_prepareDoAdvance ( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCTXDATA__PREPRAREDOADVANCE ) ;

      rtnAdvanceSection sec ;
      INT32 type = MSG_ADVANCE_TO_FIRST_IN_VALUE ;

      if ( _nextAdvanceSecIt == _advanceSectionList.end() )
      {
         _hitEnd = TRUE ;
         rc = SDB_DMS_EOC ;
         goto done ;
      }

      sec = *_nextAdvanceSecIt ;
      if ( !sec.startIncluded )
      {
         type = MSG_ADVANCE_TO_NEXT_OUT_VALUE ;
      }

      rc = _doAdvance( type, sec.prefixNum, sec.startKey, _orderBy,
                       sec.startKey, TRUE, cb ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCTXDATA__PREPRAREDOADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXDATA__EXTRACTALLEQUALSEC, "_rtnContextData::_extractAllEqualSec" )
   INT32 _rtnContextData::_extractAllEqualSec( INT32 indexFieldNum,
                                               const BSONElement &eNum,
                                               const BSONElement &eVal )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCTXDATA__EXTRACTALLEQUALSEC ) ;

      INT32 prefixNum = 0 ;
      BSONObj indexValue ;
      BSONObjIterator indexIt ;

      try
      {
         if ( NumberInt != eNum.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Int",
                        FIELD_NAME_PREFIX_NUM ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( Array != eVal.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Array",
                        FIELD_NAME_INDEXVALUE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         prefixNum = eNum.numberInt() ;
         if ( prefixNum <= 0 )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] is invalid",
                        FIELD_NAME_PREFIX_NUM ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( prefixNum > indexFieldNum )
         {
            PD_LOG ( PDWARNING, "PrefixNum[%d] is too long, truncate to "
                     "the same as the number of order by field", prefixNum ) ;
            prefixNum = indexFieldNum ;
         }

         indexValue = eVal.embeddedObject() ;
         indexIt = BSONObjIterator( indexValue ) ;

         while ( indexIt.more() )
         {
            BSONObj keyObj ;
            BSONObjBuilder startBuilder ;
            BSONObjBuilder endBuilder ;
            rtnAdvanceSection section ;
            BSONObjIterator orderIt( _orderBy ) ;
            BSONObjIterator keyObjIt ;
            BSONElement elem = indexIt.next() ;

            if ( Object != elem.type() )
            {
               PD_LOG_MSG( PDERROR, "The section of Field[%s] must be Array",
                           FIELD_NAME_INDEXVALUE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            keyObj = elem.embeddedObject() ;
            keyObjIt = BSONObjIterator ( keyObj ) ;
            for ( INT32 i = 0 ; i < prefixNum && orderIt.more() ; ++ i )
            {

               BSONElement eField = orderIt.next() ;
               if ( keyObjIt.more() )
               {
                  BSONElement eVal = keyObjIt.next() ;
                  startBuilder.appendAs ( eVal, "" ) ;
                  endBuilder.appendAs ( eVal, "" ) ;
               }
               else
               {
                  if ( eField.numberInt() > 0 )
                  {
                     startBuilder.appendMinKey( "" ) ;
                     endBuilder.appendMaxKey( "" ) ;
                  }
                  else
                  {
                     startBuilder.appendMaxKey( "" ) ;
                     endBuilder.appendMinKey( "" ) ;
                  }
               }
            }

            section.prefixNum = prefixNum ;
            section.startIncluded = TRUE ;
            section.endIncluded = TRUE ;
            section.startKey = startBuilder.obj() ;
            section.endKey = endBuilder.obj() ;
            _advanceSectionList.push_back ( section ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to extract all equal section, rc: %d, "
                 "Occur exception: %s", rc, e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXDATA__EXTRACTALLEQUALSEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXDATA__EXTRACTRANGESEC, "_rtnContextData::_extractRangeSec" )
   INT32 _rtnContextData::_extractRangeSec( INT32 indexFieldNum,
                                            const BSONElement &eNum,
                                            const BSONElement &eVal,
                                            const BSONElement &eIndexValueInc )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCTXDATA__EXTRACTRANGESEC ) ;

      BSONObj keyVal ;
      BSONObj objValueInc ;
      BSONObj objPrefixNum ;
      BSONObj objValue ;
      BSONObjIterator objValueIt ;
      BSONObjIterator objValueIncIt ;
      BSONObjIterator objPrefixNumIt ;

      try
      {
         if ( Array != eIndexValueInc.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Array",
                        FIELD_NAME_INDEXVALUE_INCLUDED );
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( Array != eNum.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Array",
                        FIELD_NAME_PREFIX_NUM ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( Array != eVal.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Array",
                        FIELD_NAME_INDEXVALUE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         objValueInc = eIndexValueInc.embeddedObject() ;
         objPrefixNum = eNum.embeddedObject() ;
         objValue = eVal.embeddedObject() ;

         if ( objValueInc.nFields() != objValue.nFields() )
         {
            PD_LOG_MSG( PDERROR, "The number of Field[%s] must be equal "
                        "to Field[%s]", FIELD_NAME_INDEXVALUE_INCLUDED,
                        FIELD_NAME_INDEXVALUE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if( objPrefixNum.nFields() != objValue.nFields() )
         {
            PD_LOG_MSG( PDERROR, "The number of Field[%s] must be equal "
                        "to Field[%s]", FIELD_NAME_PREFIX_NUM,
                        FIELD_NAME_INDEXVALUE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         objValueIt = BSONObjIterator( objValue ) ;
         objValueIncIt = BSONObjIterator( objValueInc ) ;
         objPrefixNumIt = BSONObjIterator( objPrefixNum ) ;

         while ( objValueIt.more() )
         {
            INT32 prefixNum ;
            BOOLEAN start = TRUE ;
            BSONObj indexValue ;
            BSONObj indexValueInc ;
            BSONObjIterator indexValueIt ;
            BSONObjIterator indexValueIncIt ;
            rtnAdvanceSection section ;

            BSONElement eValue = objValueIt.next() ;
            BSONElement eValueInc = objValueIncIt.next() ;
            BSONElement ePrefixNum = objPrefixNumIt.next() ;

            if ( eValue.type() != Array )
            {
               PD_LOG_MSG( PDERROR, "The section of Field[%s] must be Array",
                           FIELD_NAME_INDEXVALUE ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( eValueInc.type() != Array )
            {
               PD_LOG_MSG( PDERROR, "The section of Field[%s] must be Array",
                           FIELD_NAME_INDEXVALUE_INCLUDED ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( ePrefixNum.type() != NumberInt )
            {
               PD_LOG_MSG( PDERROR, "The value of Field[%s] must be "
                           "Int", FIELD_NAME_PREFIX_NUM ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            prefixNum = ePrefixNum.numberInt() ;
            if ( prefixNum <= 0 )
            {
               PD_LOG_MSG( PDERROR, "Field[%s] is invalid",
                           FIELD_NAME_PREFIX_NUM ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            else if ( prefixNum > indexFieldNum )
            {
               PD_LOG ( PDWARNING, "PrefixNum[%d] is too long, truncate to "
                        "the same as the number of order by field", prefixNum ) ;
               prefixNum = indexFieldNum ;
            }

            indexValue = eValue.embeddedObject() ;
            indexValueInc = eValueInc.embeddedObject() ;
            indexValueIt = BSONObjIterator( indexValue ) ;
            indexValueIncIt = BSONObjIterator( indexValueInc ) ;

            while ( indexValueIt.more() )
            {
               BSONObj keyObj ;
               BSONObjBuilder builder ;
               BSONObjIterator orderIt( _orderBy ) ;
               BSONObjIterator keyObjIt ;
               BSONElement eIndexValue = indexValueIt.next() ;
               BSONElement eIndexValueInc = indexValueIncIt.next() ;

               if ( eIndexValue.type() != Object )
               {
                  PD_LOG_MSG( PDERROR, "The single index value of "
                              "section '%s' of Field[%s] must be Object",
                              eIndexValue.toString().c_str(),
                              FIELD_NAME_INDEXVALUE ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else if ( eIndexValueInc.type() != Bool )
               {
                  PD_LOG_MSG( PDERROR, "The single index include value of "
                              "section '%s' of Field[%s] must be Bool",
                              eIndexValueInc.toString().c_str(),
                              FIELD_NAME_INDEXVALUE_INCLUDED ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }

               keyObj = eIndexValue.embeddedObject() ;
               keyObjIt = BSONObjIterator ( keyObj ) ;
               for ( INT32 i = 0 ; i < prefixNum && orderIt.more() ; ++i )
               {
                  BSONElement eField = orderIt.next() ;
                  if ( keyObjIt.more() )
                  {
                     builder.appendAs( keyObjIt.next(), "" ) ;
                  }
                  else
                  {
                     BOOLEAN isPositive = eField.numberInt() > 0 ;
                     if ( isPositive == start )
                     {
                        builder.appendMinKey( "" ) ;
                     }
                     else
                     {
                        builder.appendMaxKey( "" ) ;
                     }
                  }
               }
               keyVal = builder.obj() ;

               if ( start )
               {
                  start = FALSE;
                  section.prefixNum = prefixNum ;
                  section.startIncluded = eIndexValueInc.Bool() ;
                  section.startKey = keyVal ;
               }
               else
               {
                  INT32 cmp = 0 ;

                  section.endIncluded = eIndexValueInc.Bool() ;
                  section.endKey = keyVal ;

                  cmp = _woNCompare( section.startKey, section.endKey,
                                     FALSE, section.prefixNum, _orderBy ) ;
                  if ( cmp > 0 )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "The start and end values of section "
                                 "[%s, %s] don't meet the index scan order",
                                 section.startKey.toString().c_str(),
                                 section.endKey.toString().c_str() ) ;
                     goto error ;
                  }
                  _advanceSectionList.push_back ( section ) ;
                  break ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to extract range section, rc: %d, "
                 "Occur exception: %s", rc, e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCTXDATA__EXTRACTRANGESEC, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCTXDATA__SETADVANCESECTION, "_rtnContextData::setAdvanceSection" )
   INT32 _rtnContextData::setAdvanceSection ( const BSONObj &arg )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNCTXDATA__SETADVANCESECTION ) ;

      BSONElement eNum ;
      BSONElement eVal ;
      BSONElement eValueInc ;
      BSONElement eIsAllEqual ;
      INT32 indexFieldNum = 0 ;
      BOOLEAN isAllEqual = FALSE ;
      ixmIndexCB *pIndexCB = NULL ;

      rtnIXScanner *ixScanner = NULL ;

      rc = _getAdvanceOrderby( _orderBy, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

      // check index
      ixScanner = getIXScanner() ;
      PD_CHECK( ixScanner, SDB_SYS, error, PDERROR,
                "Failed to set advance section, not a index scanner" ) ;
      pIndexCB = ixScanner->getIndexCB() ;
      if ( !pIndexCB )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !pIndexCB->isInitialized() )
      {
         rc = SDB_DMS_INIT_INDEX ;
         goto done ;
      }

      if ( pIndexCB->getFlag() != IXM_INDEX_FLAG_NORMAL )
      {
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto done ;
      }

      // compare the historical index OID with the current index oid, to make
      // sure the index is not changed during the time
      if ( !pIndexCB->isStillValid ( ixScanner->getIdxOID() ) ||
           ixScanner->getIdxLID() != pIndexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto done ;
      }

      try
      {
         eIsAllEqual = arg.getField( FIELD_NAME_IS_ALL_EQUAL ) ;
         eValueInc = arg.getField( FIELD_NAME_INDEXVALUE_INCLUDED ) ;
         eNum = arg.getField( FIELD_NAME_PREFIX_NUM ) ;
         eVal = arg.getField( FIELD_NAME_INDEXVALUE ) ;

         if ( Bool != eIsAllEqual.type() )
         {
            PD_LOG_MSG( PDERROR, "Field[%s] must be Bool",
                        FIELD_NAME_IS_ALL_EQUAL ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         indexFieldNum = pIndexCB->keyPattern().nFields() ;
         isAllEqual = eIsAllEqual.Bool() ;

         if ( isAllEqual )
         {
            rc = _extractAllEqualSec (indexFieldNum, eNum, eVal ) ;
            if ( rc )
            {
               goto error ;
            }
         }
         else
         {
            rc = _extractRangeSec(indexFieldNum, eNum, eVal, eValueInc) ;
            if ( rc )
            {
               goto error ;
            }
         }

         _advanceSectionList.sort( rtnCmpSection(_orderBy) ) ;
         _nextAdvanceSecIt = _advanceSectionList.begin() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to set advance section [%s], rc: %d, Occur "
                 "exception: %s", arg.toString().c_str(), rc, e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCTXDATA__SETADVANCESECTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXDATA_VALIDATE, "_rtnContextData::validate" )
   INT32 _rtnContextData::validate ( const BSONObj &record )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXDATA_VALIDATE ) ;

      INT32 cmp = 0 ;
      BOOLEAN matched = FALSE ;
      BSONObj curKeyObj ;
      rtnAdvanceSection sec ;
      INT32 prefixNum = -1 ;

      if ( _needValidate() &&
           _nextAdvanceSecIt == _advanceSectionList.end() )
      {
         rc = pdError( SDB_IXM_ADVANCE_EOC ) ;
         goto error ;
      }
      else if ( !_needValidate() )
      {
         goto done ;
      }

      sec = *_nextAdvanceSecIt ;

      /// generate keyVal
      if ( !_keyGen.isInit() )
      {
         rc = _keyGen.setKeyPattern( _orderBy ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Set index generate pattern failed, rc: %d", rc ) ;
            goto error ;
         }
      }

      rc = _keyGen.getKeys( record, curKeyObj ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Generate key from obj(%s) failed, rc: %d",
                          record.toPoolString().c_str(), rc ) ;
         goto error ;
      }

      try
      {
         while ( _nextAdvanceSecIt != _advanceSectionList.end() )
         {
            sec = *_nextAdvanceSecIt ;
            prefixNum = sec.prefixNum ;

            if ( _isPrevSec )
            {
               cmp = _woNCompare( curKeyObj, sec.endKey, FALSE,
                                  prefixNum, _orderBy ) ;
               if ( 0 == cmp )
               {
                  if ( sec.endIncluded )
                  {
                     matched = TRUE ;
                     break ;
                  }
                  else
                  {
                     _nextAdvanceSecIt++ ;
                     _isPrevSec = FALSE ;
                  }
               }
               else if ( cmp < 0 )
               {
                  matched = TRUE ;
                  break ;
               }
               else
               {
                  _nextAdvanceSecIt++ ;
                  _isPrevSec = FALSE ;
               }

            }
            else
            {
               cmp = _woNCompare( sec.startKey, curKeyObj,
                                  FALSE, prefixNum, _orderBy ) ;
               if ( 0 == cmp )
               {
                  if ( !sec.startIncluded )
                  {
                     rc = pdError( SDB_IXM_ADVANCE_EOC ) ;
                     PD_LOG( PDINFO, "Advance to the next section for scanning "
                                     "record, rc: %d", rc ) ;
                     goto error ;
                  }
                  else
                  {
                     matched = TRUE ;
                     _isPrevSec = TRUE ;
                     break ;
                  }
               }
               else if ( cmp < 0 )
               {
                  cmp = _woNCompare( curKeyObj, sec.endKey, FALSE,
                                     prefixNum, _orderBy ) ;
                  if ( 0 == cmp )
                  {
                     if ( sec.endIncluded )
                     {
                        matched = TRUE ;
                        _isPrevSec = TRUE ;
                        break ;
                     }
                     else
                     {
                        _nextAdvanceSecIt++ ;
                     }
                  }
                  else if ( cmp < 0 )
                  {
                     matched = TRUE ;
                     _isPrevSec = TRUE ;
                     break ;
                  }
                  else
                  {
                     _nextAdvanceSecIt++ ;
                  }
               }
               else
               {
                  rc = pdError( SDB_IXM_ADVANCE_EOC ) ;
                  PD_LOG( PDINFO, "Advance to the next section for scanning "
                                  "record, rc: %d", rc ) ;
                  goto error ;
               }
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to validate reccord, rc: %d, "
                 "Occur exception: %s", rc, e.what() ) ;
         goto error ;
      }

      if ( !matched )
      {
         rc = pdError( SDB_IXM_ADVANCE_EOC ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_RTNCTXDATA_VALIDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCTXDATA__DOADVANCE, "_rtnContextData::_doAdvance" )
   INT32 _rtnContextData::_doAdvance( INT32 type,
                                      INT32 prefixNum,
                                      const BSONObj &keyVal,
                                      const BSONObj &orderby,
                                      const BSONObj &arg,
                                      BOOLEAN isLocate,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCTXDATA__DOADVANCE ) ;

      BOOLEAN doLock = FALSE ;
      const BSONObj *pSaveObj = NULL ;
      ixmIndexCB *pIndexCB = NULL ;
      dmsRecordID rid ;

      rtnIXScanner *ixScanner = getIXScanner() ;
      PD_CHECK( ixScanner, SDB_SYS, error, PDERROR,
                "Failed to do advance, not a index scanner" ) ;

      if ( IXSCAN != _scanType )
      {
         PD_LOG( PDWARNING, "Table scan does not support advance" ) ;
         rc = SDB_OPTION_NOT_SUPPORT ;
         goto error ;
      }

      // lock
      if ( !_mbContext->isMBLock() )
      {
         rc = _mbContext->mbLock( SHARED ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Lock collection failed, rc: %d", rc ) ;
            goto error ;
         }
         doLock = TRUE ;
      }

      // check index
      pIndexCB = ixScanner->getIndexCB() ;
      if ( !pIndexCB )
      {
         PD_LOG ( PDERROR, "Failed to allocate memory for indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      if ( !pIndexCB->isInitialized() )
      {
         rc = SDB_DMS_INIT_INDEX ;
         goto done ;
      }

      if ( pIndexCB->getFlag() != IXM_INDEX_FLAG_NORMAL )
      {
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto done ;
      }

      // compare the historical index OID with the current index oid, to make
      // sure the index is not changed during the time
      if ( !pIndexCB->isStillValid ( ixScanner->getIdxOID() ) ||
           ixScanner->getIdxLID() != pIndexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto done ;
      }

      // check key and index def
      try
      {
         // build located object
         BSONObj objLocate ;
         BSONObj keyPattern = pIndexCB->keyPattern() ;
         INT32 keyPatternNum = keyPattern.nFields() ;
         INT32 cmp = 0 ;

         if ( _scanner->isEOF() )
         {
            goto done ;
         }

         if ( prefixNum > keyPatternNum )
         {
            PD_LOG ( PDWARNING, "PrefixNum[%s] is too long, truncate to "
                     "the same as the number of order by field", prefixNum ) ;
            prefixNum = keyPatternNum ;
         }

         if ( !_compareFieldName( orderby, keyPattern, prefixNum ) )
         {
            PD_LOG_MSG( PDERROR, "Orderby is not the prefix of key pattern" ) ;
            rc = SDB_OPTION_NOT_SUPPORT ;
            goto error ;
         }

         pSaveObj = ixScanner->getSavedObj() ;
         cmp = _woNCompare( keyVal, *pSaveObj, FALSE, prefixNum, orderby ) ;
         if ( cmp < 0 )
         {
            goto done ;
         }
         else if ( MSG_ADVANCE_TO_FIRST_IN_VALUE == type && 0 == cmp )
         {
            goto done ;
         }

         /// build next value
         if ( prefixNum == keyPatternNum && 0 == cmp )
         {
            objLocate = *pSaveObj ;
         }
         else
         {
            objLocate = _buildNextValueObj( keyPattern, keyVal,
                                            prefixNum, type ) ;
         }

         /// build next rid
         _buildNextRID( type, rid ) ;

         /// relocate
         rc = ixScanner->relocateRID( objLocate, rid ) ;
         if ( rc )
         {
            PD_LOG_MSG( PDERROR, "Relocate failed, rc: %d", rc ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      if ( doLock )
      {
         _mbContext->mbUnlock() ;
      }
      PD_TRACE_EXITRC( SDB_RTNCTXDATA__DOADVANCE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   BSONObj _rtnContextData::_buildNextValueObj( const BSONObj &keyPattern,
                                                const BSONObj &srcVal,
                                                UINT32 keepNum,
                                                INT32 type ) const
   {
      BSONObjBuilder builder ;
      BSONObjIterator itrKey( keyPattern ) ;
      BSONObjIterator itrVal( srcVal ) ;

      for ( UINT32 i = 0 ; i < keepNum && itrKey.more() && itrVal.more() ; ++i )
      {
         itrKey.next() ;
         builder.appendAs( itrVal.next(), "" ) ;
      }

      while( itrKey.more() )
      {
         BSONElement e = itrKey.next() ;

         if ( _direction * e.numberInt() > 0 )
         {
            if ( MSG_ADVANCE_TO_FIRST_IN_VALUE == type )
            {
               builder.appendMinKey( "" ) ;
            }
            else
            {
               builder.appendMaxKey( "" ) ;
            }
         }
         else
         {
            if ( MSG_ADVANCE_TO_FIRST_IN_VALUE == type )
            {
               builder.appendMaxKey( "" ) ;
            }
            else
            {
               builder.appendMinKey( "" ) ;
            }
         }
      }

      return builder.obj() ;
   }

   void _rtnContextData::_buildNextRID( INT32 type,
                                        dmsRecordID &rid ) const
   {
      if ( _direction > 0 )
      {
         if ( MSG_ADVANCE_TO_FIRST_IN_VALUE == type )
         {
            rid.resetMin() ;
         }
         else
         {
            rid.resetMax() ;
         }
      }
      else
      {
         if ( MSG_ADVANCE_TO_FIRST_IN_VALUE == type )
         {
            rid.resetMax() ;
         }
         else
         {
            rid.resetMin() ;
         }
      }
   }

   BOOLEAN _rtnContextData::_compareFieldName( const BSONObj &orderby,
                                               const BSONObj &keyPattern,
                                               UINT32 prefixNum ) const
   {
      BSONObjIterator itO( orderby ) ;
      BSONObjIterator itK( keyPattern ) ;
      UINT32 i = 0 ;

      for ( ; i < prefixNum && itO.more() && itK.more() ; ++i )
      {
         if ( 0 != ossStrcmp( itO.next().fieldName(),
                              itK.next().fieldName() ) )
         {
            return FALSE ;
         }
      }

      return i == prefixNum ? TRUE : FALSE ;
   }

   void _rtnContextData::_toString( stringstream & ss )
   {
      if ( NULL != _su && NULL != _planRuntime.getPlan() )
      {
         ss << ",Collection:" << _planRuntime.getCLFullName() ;
         ss << ",Matcher:" << _planRuntime.getParsedMatcher().toString() ;
      }
      ss << ",ScanType:" << ( ( TBSCAN == _scanType ) ? "TBSCAN" : "IXSCAN" ) ;
      if ( _numToReturn > 0 )
      {
         ss << ",NumToReturn:" << _numToReturn ;
      }
      if ( _numToSkip > 0 )
      {
         ss << ",NumToSkip:" << _numToSkip ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTDATA_OPIXSC, "_rtnContextData::_openIXScan" )
   INT32 _rtnContextData::_openIXScan( dmsStorageUnit *su,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB *cb,
                                       const rtnReturnOptions &returnOptions,
                                       const BSONObj *blockObj,
                                       INT32 direction )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCONTEXTDATA_OPIXSC );

      rtnScannerFactory f ;
      rtnIXScanner *tmp = NULL ;
      rtnScannerType scanType = ( DPS_INVALID_TRANS_ID != cb->getTransID() ) ?
                               SCANNER_TYPE_MERGE : SCANNER_TYPE_DISK ;
      BOOLEAN isAsync = ( DPS_INVALID_TRANS_ID == cb->getTransID() ) &&
                        ( !_queryModifier ) &&
                        ( pmdGetKRCB()->getBPSCB()->isPrefetchEnabled() ) ;
      rtnPredicateList *predList = NULL ;

      // for index scan, we maintain context by runtime instead of by DMS
      ixmIndexCB indexCB ( _planRuntime.getIndexCBExtent(),
                           su->index(), NULL ) ;

      // index block scan
      if ( blockObj )
      {
         PD_LOG( PDERROR, "Block scan is not supported for index scan" ) ;
         rc = SDB_ENGINE_NOT_SUPPORT ;
         goto error ;
      }

      rc = rtnIsIndexCBValid( &indexCB, _planRuntime.getIndexCBExtent(),
                              _planRuntime.getIndexName(),
                              _planRuntime.getIndexLID(), su, mbContext ) ;
      if ( rc )
      {
         goto error ;
      }

      /// set direction
      _direction = _planRuntime.getPlan()->getDirection() ;

      // get the predicate list
      predList = _planRuntime.getPredList() ;
      SDB_ASSERT ( predList, "predList can't be NULL" ) ;

      // create scanner
      if ( _scanner )
      {
         f.releaseScanner( _scanner ) ;
         _scanner = NULL ;
      }

      rc = f.createIXScanner( scanType, &indexCB, predList, su, mbContext, isAsync, cb, tmp ) ;
      if ( rc )
      {
         goto error ;
      }
      _scanner = tmp ;

   done :
      PD_TRACE_EXITRC ( SDB_RTNCONTEXTDATA_OPIXSC , rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNCONTEXTDATA_OPTBSC, "_rtnContextData::_openTBScan" )
   INT32 _rtnContextData::_openTBScan( dmsStorageUnit *su,
                                       dmsMBContext *mbContext,
                                       pmdEDUCB * cb,
                                       const rtnReturnOptions &returnOptions,
                                       const BSONObj *blockObj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_RTNCONTEXTDATA_OPTBSC );

      rtnScannerFactory f ;
      rtnTBScanner *tmp = NULL ;
      rtnScannerType scanType = ( DPS_INVALID_TRANS_ID != cb->getTransID() ) ?
                               SCANNER_TYPE_MERGE : SCANNER_TYPE_DISK ;
      BOOLEAN isAsync = ( DPS_INVALID_TRANS_ID == cb->getTransID() ) &&
                        ( !_queryModifier ) &&
                        ( pmdGetKRCB()->getBPSCB()->isPrefetchEnabled() ) ;

      if ( blockObj )
      {
         PD_LOG( PDERROR, "Block scan is not supported for table scan" ) ;
         rc = SDB_ENGINE_NOT_SUPPORT ;
         goto error ;
      }

      // create scanner
      if ( _scanner )
      {
         f.releaseScanner( _scanner ) ;
         _scanner = NULL ;
      }
      rc = f.createTBScanner( scanType, su, mbContext, isAsync, cb, tmp ) ;
      if ( rc )
      {
         goto error ;
      }
      _scanner = tmp ;

   done:
      PD_TRACE_EXITRC ( SDB_RTNCONTEXTDATA_OPTBSC , rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::open( dmsStorageUnit *su,
                                dmsMBContext *mbContext,
                                pmdEDUCB *cb,
                                const rtnReturnOptions &returnOptions,
                                const BSONObj *blockObj,
                                INT32 direction )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isStictType = FALSE ;
      BSONObj selector = returnOptions.getSelector() ;

      SDB_ASSERT( su && mbContext, "Invalid param" ) ;
      SDB_ASSERT( _planRuntime.hasPlan(), "Invalid plan" ) ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_QUERY ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                         DMS_MB_ATTR_STRICTDATAMODE ) )
      {
         isStictType = TRUE ;
      }

      _hitEnd = FALSE ;

      if ( TBSCAN == _planRuntime.getScanType() )
      {
         rc = _openTBScan( su, mbContext, cb, returnOptions, blockObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open tbscan, rc: %d", rc ) ;

         mbContext->mbStat()->_crudCB.increaseTbScan( 1 ) ;
      }
      else if ( IXSCAN == _planRuntime.getScanType() )
      {
         rc = _openIXScan( su, mbContext, cb, returnOptions,
                           blockObj, direction ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open ixscan, rc: %d", rc ) ;

         mbContext->mbStat()->_crudCB.increaseIxScan( 1 ) ;
      }
      else
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unknow scan type: %d", _planRuntime.getScanType() ) ;
         goto error ;
      }

      if ( selector.isEmpty() &&
           IXSCAN == _planRuntime.getScanType() &&
           returnOptions.testFlag( FLG_FORCE_INDEX_SELECTOR ) )
      {
         /**
          * get index key pattern, then ergodic it and rewrite its value
          * keyPattern: { a: 1, b: 1 }
          * selector:   {} =>
          *             { a: { $include: 1 }, b: { $include: 1 } }
          */
         try
         {
            BSONObj keyPattern = _planRuntime.getPlan()->getKeyPattern() ;
            BSONObjBuilder ob ;
            BSONObjIterator it( keyPattern ) ;
            BSONObj includeObj = BSON( "$include" << 1 ) ;
            while ( it.more() )
            {
               BSONElement ele = it.next() ;
               ob.append( ele.fieldName(), includeObj ) ;
            }
            selector = ob.obj() ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG ( PDERROR, "Failed to generate selector bson, occur "
                     "exception: %s, rc: %d", e.what(), rc ) ;
            goto error ;
         }
      }

      // once context is opened, let's construct matcher and selector
      if ( !selector.isEmpty() )
      {
         IXM_FIELD_NAME_SET selectSet ;
         try
         {
            if( TRUE == pmdGetOptionCB()->isIndexCoverOn() )
            {
               rc = _selector.loadPattern ( selector, isStictType, &selectSet ) ;
            }
            else
            {
               rc = _selector.loadPattern ( selector, isStictType, NULL ) ;
            }
            PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for select: "
                         "%s, rc: %d", selector.toString().c_str(), rc ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed selector loadPattern, exception: %s, rc=%d",
                         e.what(), rc ) ;
         }

         _evalIndexCover( selectSet ) ;

         rtnIXScanner *ixScanner = getIXScanner() ;
         if ( ixScanner )
         {
            ixScanner->setIndexCover( _indexCover ) ;
         }
      }

      _dmsCB = pmdGetKRCB()->getDMSCB() ;
      _su = su ;
      _suLogicalID = su->LogicalCSID() ;
      _mbContext = mbContext ;
      _scanType = _planRuntime.getScanType() ;

      _returnOptions.setSelector( returnOptions.getSelector().getOwned() ) ;
      _returnOptions.setSkip( returnOptions.getSkip() ) ;
      _returnOptions.setLimit( returnOptions.getLimit() ) ;
      _returnOptions.resetFlag( returnOptions.getFlag() ) ;
      _numToReturn = returnOptions.getLimit() ;
      _numToSkip = returnOptions.getSkip() ;

      _isOpened = TRUE ;
      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      mbContext->mbUnlock() ;
      return rc ;
   error:
      _isOpened = FALSE ;
      _hitEnd = TRUE ;
      goto done ;
   }

   INT32 _rtnContextData::openTraversal( dmsStorageUnit *su,
                                         dmsMBContext *mbContext,
                                         rtnIXScanner *scanner,
                                         pmdEDUCB *cb,
                                         const BSONObj &selector,
                                         INT64 numToReturn,
                                         INT64 numToSkip )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN strictDataMode = FALSE ;

      SDB_ASSERT( su && mbContext && scanner, "Invalid param" ) ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }
      if ( IXSCAN != _planRuntime.getScanType() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Open traversal must IXSCAN" ) ;
         goto error ;
      }

      rc = mbContext->mbLock( SHARED ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      if ( !dmsAccessAndFlagCompatiblity ( mbContext->mb()->_flag,
                                           DMS_ACCESS_TYPE_QUERY ) )
      {
         PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                  mbContext->mb()->_flag ) ;
         rc = SDB_DMS_INCOMPATIBLE_MODE ;
         goto error ;
      }
      if ( OSS_BIT_TEST( mbContext->mb()->_attributes,
                         DMS_MB_ATTR_STRICTDATAMODE ) )
      {
         strictDataMode = TRUE ;
      }

      if ( _scanner )
      {
         SDB_OSS_DEL _scanner ;
      }
      _scanner = scanner ;

      // once context is opened, let's construct matcher and selector
      if ( !selector.isEmpty() )
      {
         try
         {
            rc = _selector.loadPattern ( selector, strictDataMode ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Invalid pattern is detected for select: %s: %s",
                     selector.toString().c_str(), e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Invalid pattern is detected for select: "
                      "%s, rc: %d", selector.toString().c_str(), rc ) ;
      }

      _dmsCB = pmdGetKRCB()->getDMSCB() ;
      _su = su ;
      _suLogicalID = su->LogicalCSID() ;
      _mbContext = mbContext ;
      _scanType = _planRuntime.getScanType() ;
      _numToReturn = numToReturn ;
      _numToSkip = numToSkip > 0 ? numToSkip : 0 ;

      _isOpened = TRUE ;
      _hitEnd = _scanner->isEOF() ? TRUE : FALSE ;

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

   done:
      mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextData::setQueryModifier ( rtnQueryModifier* modifier )
   {
      SDB_ASSERT( NULL == _queryModifier, "_queryModifier already exists" ) ;

      _queryModifier = modifier ;
   }

   void _rtnContextData::setQueryActivity ( BOOLEAN hitEnd )
   {
      if ( _planRuntime.canSetQueryActivity() &&
           enabledMonContext() &&
           enabledQueryActivity() )
      {
         _planRuntime.setQueryActivity( MON_SELECT, _monCtxCB, _returnOptions,
                                        hitEnd ) ;
      }
   }

   INT32 _rtnContextData::_queryModify( pmdEDUCB* eduCB,
                                        const dmsRecordID& recordID,
                                        ossValuePtr recordDataPtr,
                                        BSONObj& obj,
                                        IDmsOprHandler* pHandler,
                                        const dmsTransRecordInfo *pInfo )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _queryModifier, "_queryModifier can't be null" ) ;
      // check id index
      if ( OSS_BIT_TEST( _mbContext->mb()->_attributes,
                         DMS_MB_ATTR_NOIDINDEX ) )
      {
         PD_LOG( PDERROR, "can not update data when autoIndexId is false" ) ;
         rc = SDB_RTN_AUTOINDEXID_IS_FALSE ;
         goto error ;
      }

      if ( _queryModifier->isUpdate() )
      {
         BSONObj* newObjPtr = NULL ;

         if ( _queryModifier->returnNew() )
         {
            newObjPtr = &obj ;
         }
         else
         {
            obj = obj.getOwned() ;
         }

         SDB_ASSERT( NULL != _queryModifier->getDollarList(),
                     "dollarList can't be null" ) ;

         rc = _su->data()->updateRecord( _mbContext, recordID,
                                         recordDataPtr, eduCB, getDPSCB(),
                                         _queryModifier->getModifier(),
                                         newObjPtr, pHandler ) ;
         PD_RC_CHECK( rc, PDERROR, "Update record failed, rc: %d", rc ) ;
         _queryModifier->getDollarList()->clear() ;
      }
      else if ( _queryModifier->isRemove() )
      {
         rc = _su->data()->deleteRecord( _mbContext, recordID,
                                         recordDataPtr, eduCB, getDPSCB(),
                                         pHandler, pInfo ) ;
         PD_RC_CHECK( rc, PDERROR, "Delete record failed, rc: %d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_prepareData( pmdEDUCB *cb )
   {
      vector<INT64>* dollarList = NULL ;
      DMS_ACCESS_TYPE accessType = DMS_ACCESS_TYPE_FETCH ;
      INT32 rc = SDB_OK ;

      if ( NULL != cb && NULL != _mbContext )
      {
         cb->registerMonCRUDCB( &( _mbContext->mbStat()->_crudCB ) ) ;
      }

      if ( _queryModifier )
      {
         if ( _queryModifier->isUpdate() )
         {
            accessType = DMS_ACCESS_TYPE_UPDATE ;
            dollarList = _queryModifier->getDollarList() ;
         }
         else if ( _queryModifier->isRemove() )
         {
            accessType = DMS_ACCESS_TYPE_DELETE ;
         }
         else
         {
            SDB_ASSERT( FALSE, "_queryModifier is invalid" ) ;
            PD_LOG( PDERROR, "_queryModifier is invalid" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      {
         if ( TBSCAN == _scanType )
         {
            rtnTBScanner *tbScanner = getTBScanner() ;
            PD_CHECK( tbScanner, SDB_SYS, error, PDERROR,
                     "Failed to prepare data, table scanner is invalid" ) ;
            rc = _prepareByTBScan( tbScanner, cb, accessType, dollarList ) ;
         }
         else if ( IXSCAN == _scanType )
         {
            rtnIXScanner *ixScanner = getIXScanner() ;
            PD_CHECK( ixScanner, SDB_SYS, error, PDERROR,
                     "Failed to prepare data, index scanner is invalid" ) ;
            rc = _prepareByIXScan( ixScanner, cb, accessType, dollarList ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
         }
      }

   done:
      if ( NULL != cb && NULL != _mbContext )
      {
         cb->unregisterMonCRUDCB() ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__EVALCOVER, "_rtnContextData::_evalIndexCover" )
   INT32 _rtnContextData::_evalIndexCover( IXM_FIELD_NAME_SET &selectSet )
   {
      INT32 rc = SDB_OK ;
      IXM_FIELD_NAME_SET::iterator it ;
      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__EVALCOVER );

      _indexCover = FALSE ;
      if ( !_planRuntime.isIndexCover() )
      {
         goto done ;
      }

      try
      {
         if( selectSet.size() > 0 )
         {
            ixmIndexCover index( _planRuntime.getPlan()->getKeyPattern() ) ;
            it = selectSet.begin() ;
            while( it != selectSet.end() )
            {
               if( FALSE == index.cover( (*it) ) )
               {
                  goto done ;
               }
               ++ it ;
            }
            _indexCover = TRUE ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to eval IndexCover, exception: %s, rc=%d",
                      e.what(), rc ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNCONTEXTDATA__EVALCOVER, rc );
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__SELANDAPPD, "_rtnContextData::_selectAndAppend" )
   INT32 _rtnContextData::_selectAndAppend( mthSelector *selector,
                                            BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObj selObj ;
      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__SELANDAPPD );

      if ( selector )
      {
         rc = selector->select( obj, selObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build select record,"
                      "src obj: %s, rc: %d", obj.toString().c_str(),
                      rc ) ;
      }
      else
      {
         selObj = obj ;
      }

      rc = append( selObj, &obj ) ;
      PD_RC_CHECK( rc, PDERROR, "Append obj[%s] failed, rc: %d",
                   selObj.toString().c_str(), rc ) ;

   done:
      PD_TRACE_EXITRC ( SDB__RTNCONTEXTDATA__SELANDAPPD, rc );
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_innerAppend( mthSelector *selector,
                                        _mthRecordGenerator &generator )
   {
      INT32 rc = SDB_OK ;

      while ( generator.hasNext() )
      {
         BSONObj record ;
         rc = generator.getNext( record ) ;
         PD_RC_CHECK( rc, PDERROR, "get next record failed:rc=%d", rc ) ;

         rc = _selectAndAppend( selector, record ) ;
         PD_RC_CHECK( rc, PDERROR, "selectAndAppend failed:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__PREPAREBYTBSCAN, "_rtnContextData::_prepareByTBScan" )
   INT32 _rtnContextData::_prepareByTBScan( rtnTBScanner *tbScanner,
                                            pmdEDUCB * cb,
                                            DMS_ACCESS_TYPE accessType,
                                            vector<INT64>* dollarList )
   {
      INT32 rc = SDB_OK ;

      dmsRecordID recordID ;
      ossValuePtr recordDataPtr = 0 ;
      _mthRecordGenerator generator ;
      BOOLEAN hasLocked = _mbContext->isMBLock() ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      mthMatchRuntime *matchRuntime = _planRuntime.getMatchRuntime( TRUE ) ;
      mthSelector *selector   = NULL ;
      INT32 startNumRecords   = numRecords() ;

      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__PREPAREBYTBSCAN );

      if ( _selector.isInitialized() )
      {
         selector = &_selector ;
      }

      if ( NULL != _queryModifier )
      {
         generator.setQueryModify( TRUE ) ;
      }

      while ( numRecords() == startNumRecords )
      {
         dmsDataScanner scanner( _su->data(),
                                 _mbContext,
                                 tbScanner,
                                 matchRuntime,
                                 accessType,
                                 _numToReturn,
                                 _numToSkip,
                                 _returnOptions.getFlag() ) ;
         _mthMatchTreeContext mthContext( NULL ) ;
         if ( NULL != dollarList )
         {
            mthContext.enableDollarList() ;
         }

         // prefetch
         if ( eduID() != cb->getID() && !isOpened() )
         {
            rc = SDB_DMS_CONTEXT_IS_CLOSE ;
            goto error ;
         }

         while ( SDB_OK == ( rc = scanner.advance( recordID, generator, cb, &mthContext ) ) )
         {
            try
            {
               generator.getDataPtr( recordDataPtr ) ;
               BSONObj obj( (const CHAR*)recordDataPtr ) ;

               if ( _rsFilter )
               {
                  if ( _appendRIDFilter )
                  {
                     BOOLEAN pushed = FALSE ;
                     rc = _rsFilter->push( recordID, pushed ) ;
                     PD_RC_CHECK( rc, PDERROR, "Failed to push record ID to "
                                 "result set filter, rc: %d", rc ) ;
                     if ( !pushed )
                     {
                        continue ;
                     }
                  }
                  else
                  {
                     if ( _rsFilter->isFiltered( recordID ) )
                     {
                        continue ;
                     }
                  }
               }

               if ( _queryModifier )
               {
                  //dollarList is pointed to _queryModifier->getDollarList()
                  mthContext.getDollarList( dollarList ) ;
                  rc = _queryModify( cb, recordID, recordDataPtr,
                                    obj, scanner.callbackHandler(),
                                    scanner.recordInfo() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to query modify, rc: %d", rc ) ;
                  generator.resetValue( obj, &mthContext ) ;
               }

               rc = _innerAppend( selector, generator ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append record, rc: %d", rc ) ;
            }
            catch ( exception &e )
            {
               PD_LOG( PDERROR, "Failed to fetch data, occur exception: %s", e.what() ) ;
               rc = ossException2RC( &e ) ;
               goto error ;
            }
            // increase counter
            DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;
            // decrease numToReturn
            if ( _numToReturn > 0 )
            {
               --_numToReturn ;
            }

            //do not clear dollarlist flag
            mthContext.clearRecordInfo() ;
         }

         if ( SDB_OK != rc && SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to run scanner, rc: %d", rc ) ;
            goto error ;
         }
         rc = SDB_OK ;

         _numToReturn = scanner.getMaxRecords() ;
         _numToSkip   = scanner.getSkipNum() ;

         if ( ( 0 == _numToReturn ) ||
            ( scanner.isHitEnd() ) )
         {
            _hitEnd = TRUE ;
            break ;
         }
         else
         {
            _recordID = scanner.getCurRID() ;
         }

         if ( !hasLocked )
         {
            _mbContext->pause() ;
         }
      }

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      if ( !hasLocked )
      {
         _mbContext->pause() ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTDATA__PREPAREBYTBSCAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTDATA__PREPAREBYIXSCAN, "_rtnContextData::_prepareByIXScan" )
   INT32 _rtnContextData::_prepareByIXScan( rtnIXScanner *ixScanner,
                                            pmdEDUCB *cb,
                                            DMS_ACCESS_TYPE accessType,
                                            vector<INT64>* dollarList )
   {
      INT32 rc                   = SDB_OK ;
      mthMatchRuntime *matchRuntime = _planRuntime.getMatchRuntime( TRUE ) ;
      mthSelector *selector      = NULL ;
      monAppCB * pMonAppCB       = cb ? cb->getMonAppCB() : NULL ;
      BOOLEAN hasLocked          = _mbContext->isMBLock() ;
      INT32 startNumRecords      = numRecords();

      dmsRecordID rid ;
      BSONObj dataRecord ;

      PD_TRACE_ENTRY ( SDB__RTNCONTEXTDATA__PREPAREBYIXSCAN );
      if ( _selector.isInitialized() )
      {
         selector = &_selector ;
      }

      _mthRecordGenerator generator ;
      dmsRecordID recordID ;
      ossValuePtr recordDataPtr = 0 ;

      PD_CHECK( ixScanner, SDB_SYS, error, PDERROR,
                "Failed to do advance, not a index scanner" ) ;

      if ( NULL != _queryModifier )
      {
         generator.setQueryModify( TRUE ) ;
      }

      // loop until we read something in the buffer
      while ( numRecords() == startNumRecords )
      {
         _mthMatchTreeContext mthContext( _needValidate() ? this : NULL ) ;
         if ( NULL != dollarList )
         {
            mthContext.enableDollarList() ;
         }

         // prefetch
         if ( eduID() != cb->getID() && !isOpened() )
         {
            rc = SDB_DMS_CONTEXT_IS_CLOSE ;
            goto error ;
         }

         dmsIndexScanner secScanner( _su->data(),
                                     _mbContext,
                                     ixScanner,
                                     matchRuntime,
                                     accessType,
                                     _numToReturn,
                                     _numToSkip,
                                     _returnOptions.getFlag() ) ;
         if ( isCountMode() )
         {
            secScanner.enableCountMode() ;
         }

         while ( SDB_OK == ( rc = secScanner.advance( recordID, generator,
                                                      cb, &mthContext ) ) )
         {
            if ( _rsFilter )
            {
               if ( _appendRIDFilter )
               {
                  BOOLEAN pushed = FALSE ;
                  rc = _rsFilter->push( recordID, pushed ) ;
                  PD_RC_CHECK( rc, PDERROR, "Push record id to result set "
                               "filter failed: %d", rc ) ;
                  if ( !pushed )
                  {
                     continue ;
                  }
               }
               else
               {
                  if ( _rsFilter->isFiltered( recordID ) )
                  {
                     continue ;
                  }
               }
            }

            if ( !isCountMode() )
            {
               try
               {
                  generator.getDataPtr( recordDataPtr ) ;
                  BSONObj obj( (const CHAR*)recordDataPtr ) ;

                  if ( _queryModifier )
                  {
                     //dollarList is pointed to _queryModifier->getDollarList()
                     mthContext.getDollarList( dollarList ) ;
                     rc = _queryModify( cb, recordID, recordDataPtr,
                                        obj, secScanner.callbackHandler(),
                                        secScanner.recordInfo() ) ;
                     PD_RC_CHECK( rc, PDERROR, "Failed to query modify" ) ;
                     generator.resetValue( obj, &mthContext ) ;
                  }

                  rc = _innerAppend( selector, generator ) ;
                  if ( SDB_IXM_ADVANCE_EOC == rc )
                  {
                     // stop the scanner, so we can restart later
                     secScanner.stop () ;
                     goto done ;
                  }
                  PD_RC_CHECK( rc, PDERROR, "innerAppend failed:rc=%d", rc ) ;

                  // make sure we still have room to read another
                  // record_max_sz (i.e. 16MB). if we have less than 16MB
                  // to 256MB, we can't safely assume the next record we
                  // read will not overflow the buffer, so let's just break
                  // before reading the next record
                  if ( buffEndOffset() + DMS_RECORD_MAX_SZ >
                       RTN_RESULTBUFFER_SIZE_MAX )
                  {
                     secScanner.stop () ;
                     // let's break if there's no room for another max record
                     break ;
                  }
               }
               catch ( std::exception &e )
               {
                  PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               // increase counter
               DMS_MON_OP_COUNT_INC( pMonAppCB, MON_SELECT, 1 ) ;
            }
            else
            {
               static BSONObj dummyObj ;
               rc = append( dummyObj ) ;
               PD_RC_CHECK( rc, PDERROR, "Append empty obj failed, rc: %d",
                            rc ) ;
            }

            //do not clear dollarlist flag
            mthContext.clearRecordInfo() ;
         }

         if ( rc && SDB_DMS_EOC != rc )
         {
            if ( SDB_IXM_ADVANCE_EOC == rc )
            {
               // stop the scanner, so we can restart later
               secScanner.stop () ;
               goto done ;
            }
            PD_LOG( PDERROR, "Extent scanner failed, rc: %d", rc ) ;
            goto error ;
         }

         _numToReturn = secScanner.getMaxRecords() ;
         _numToSkip   = secScanner.getSkipNum() ;

         if ( 0 == _numToReturn )
         {
            _hitEnd = TRUE ;
            break ;
         }

         if ( secScanner.isHitEnd() )
         {
            if ( _indexBlockScan )
            {
               _indexBlocks.erase( _indexBlocks.begin() ) ;
               _indexBlocks.erase( _indexBlocks.begin() ) ;
               _indexRIDs.erase( _indexRIDs.begin() ) ;
               _indexRIDs.erase( _indexRIDs.begin() ) ;
               if ( _indexBlocks.size() < 2 )
               {
                  _hitEnd = TRUE ;
                  break ;
               }
            }
            else
            {
               _hitEnd = TRUE ;
               break ;
            }
         }

         if ( !hasLocked )
         {
            _mbContext->pause() ;
         }
      } // end while

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done :
      if ( !hasLocked )
      {
         _mbContext->pause() ;
      }
      PD_TRACE_EXITRC ( SDB__RTNCONTEXTDATA__PREPAREBYIXSCAN, rc );
      return rc ;
   error :
      goto done ;
   }

   INT32 _rtnContextData::_parseSegments( const BSONObj &obj,
                                          SEGMENT_VEC &segments )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      segments.clear() ;

      try
      {
         BSONObjIterator it ( obj ) ;
         while ( it.more() )
         {
            ele = it.next() ;
            if ( NumberInt != ele.type() )
            {
               PD_LOG( PDWARNING, "Datablocks[%s] value type[%d] is not NumberInt",
                       obj.toString().c_str(), ele.type() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            segments.push_back( ele.numberInt() ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse segments: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_parseRID( const BSONElement & ele,
                                     dmsRecordID & rid )
   {
      INT32 rc = SDB_OK ;
      rid.reset() ;

      if ( ele.eoo() )
      {
         goto done ;
      }
      else if ( Array != ele.type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDWARNING, "Field[%s] type is not Array",
                 ele.toString().c_str() ) ;
         goto error ;
      }
      else
      {
         UINT32 count = 0 ;
         BSONElement ridEle ;
         BSONObjIterator it( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            ridEle = it.next() ;
            if ( NumberInt != ridEle.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "RID type is not NumberInt in field[%s]",
                       ele.toString().c_str() ) ;
               goto error ;
            }
            if ( 0 == count )
            {
               rid._extent = ridEle.numberInt() ;
            }
            else if ( 1 == count )
            {
               rid._offset = ridEle.numberInt() ;
            }

            ++count ;
         }

         if ( 2 != count )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "RID array size[%d] is not 2", count ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextData::_parseIndexBlocks( const BSONObj &obj,
                                             vector< BSONObj > &indexBlocks,
                                             vector< dmsRecordID > &indexRIDs )
   {
      INT32 rc = SDB_OK ;
      BSONElement ele ;
      BSONObj indexObj ;
      BSONObj startKey, endKey ;
      dmsRecordID startRID, endRID ;

      indexBlocks.clear() ;
      indexRIDs.clear() ;

      try
      {
         BSONObjIterator it ( obj ) ;
         while ( it.more() )
         {
            ele = it.next() ;
            if ( Object != ele.type() )
            {
               PD_LOG( PDWARNING, "Indexblocks[%s] value type[%d] is not "
                       "Object", obj.toString().c_str(), ele.type() ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            indexObj = ele.embeddedObject() ;
            // StartKey
            rc = rtnGetObjElement( indexObj, FIELD_NAME_STARTKEY, startKey ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s] from obj[%s], "
                         "rc: %d", FIELD_NAME_STARTKEY,
                         indexObj.toString().c_str(), rc ) ;
            // EndKey
            rc = rtnGetObjElement( indexObj, FIELD_NAME_ENDKEY, endKey ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to get field[%s] from obj[%s], "
                         "rc: %d", FIELD_NAME_ENDKEY,
                         indexObj.toString().c_str(), rc ) ;
            // StartRID
            rc = _parseRID( indexObj.getField( FIELD_NAME_STARTRID ),
                            startRID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to parse %s, rc: %d",
                         FIELD_NAME_STARTRID, rc ) ;

            // EndRID
            rc = _parseRID( indexObj.getField( FIELD_NAME_ENDRID ), endRID ) ;
            PD_RC_CHECK( rc, PDWARNING, "Failed to parse %s, rc: %d",
                         FIELD_NAME_ENDRID, rc ) ;

            indexBlocks.push_back( rtnNullKeyNameObj( startKey ).getOwned() ) ;
            indexBlocks.push_back( rtnNullKeyNameObj( endKey ).getOwned() ) ;

            indexRIDs.push_back( startRID ) ;
            indexRIDs.push_back( endRID ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse indexBlocks: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( indexBlocks.size() != indexRIDs.size() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "block array size is not the same with rid array" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _rtnContextParaData implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextParaData, RTN_CONTEXT_PARADATA, "PARADATA")

   _rtnContextParaData::_rtnContextParaData( INT64 contextID, UINT64 eduID )
   :_rtnContextData( contextID, eduID )
   {
      _isParalled = FALSE ;
      _curIndex   = 0 ;
      _step       = 1 ;
   }

   _rtnContextParaData::~_rtnContextParaData()
   {
      setQueryActivity( _hitEnd ) ;
      vector< rtnContextData* >::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         (*it)->_close () ;
         ++it ;
      }
      it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         (*it)->waitForPrefetch() ;
         SDB_OSS_DEL (*it) ;
         ++it ;
      }
      _vecContext.clear () ;
   }

   const CHAR* _rtnContextParaData::name() const
   {
      return "PARADATA" ;
   }

   RTN_CONTEXT_TYPE _rtnContextParaData::getType () const
   {
      return RTN_CONTEXT_PARADATA ;
   }

   INT32 _rtnContextParaData::open( dmsStorageUnit *su,
                                    dmsMBContext *mbContext,
                                    pmdEDUCB *cb,
                                    const rtnReturnOptions &returnOptions,
                                    const BSONObj *blockObj,
                                    INT32 direction )
   {
      return SDB_ENGINE_NOT_SUPPORT ;
   }

   void _rtnContextParaData::_removeSubContext( rtnContextData *pContext )
   {
      vector< rtnContextData* >::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         if ( *it == pContext )
         {
            pContext->waitForPrefetch() ;
            SDB_OSS_DEL pContext ;
            _vecContext.erase( it ) ;
            break ;
         }
         ++it ;
      }
   }

   INT32 _rtnContextParaData::_openSubContext( pmdEDUCB *cb,
                                               const rtnReturnOptions &subReturnOptions,
                                               const BSONObj *blockObj )
   {
      INT32 rc = SDB_OK ;

      dmsMBContext *mbContext = NULL ;
      rtnContextData *dataContext = NULL ;

      rc = _su->data()->getMBContext( &mbContext, _planRuntime.getCLMBID(),
                                      DMS_INVALID_CLID, DMS_INVALID_CLID, -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get dms mb context, rc: %d", rc ) ;
      PD_CHECK( _planRuntime.getCLLID() == mbContext->clLID(), SDB_DMS_NOTEXIST,
                error, PDERROR, "Failed to get dms mb context, rc: %d",
                SDB_DMS_NOTEXIST ) ;

      // create a new context
      dataContext = SDB_OSS_NEW rtnContextData( -1, eduID() ) ;
      if ( !dataContext )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Alloc sub context out of memory" ) ;
         goto error ;
      }
      _vecContext.push_back( dataContext ) ;

      dataContext->getPlanRuntime()->inheritRuntime( &_planRuntime ) ;

      rc = dataContext->open( _su, mbContext, cb, subReturnOptions, blockObj,
                              _direction ) ;
      PD_RC_CHECK( rc, PDERROR, "Open sub context failed, blockObj: %s, "
                   "rc: %d", blockObj->toString().c_str(), rc ) ;

      mbContext = NULL ;

      dataContext->enablePrefetch ( cb, &_prefWather ) ;
      dataContext->setEnableQueryActivity( FALSE ) ;

      // sample timetamp
      if ( cb->getMonConfigCB()->timestampON )
      {
         dataContext->getMonCB()->recordStartTimestamp() ;
      }
      dataContext->getSelector().setStringOutput(
         getSelector().getStringOutput() ) ;

   done :
      return rc ;
   error :
      if ( mbContext )
      {
         _su->data()->releaseMBContext( mbContext ) ;
      }
      goto done ;
   }

   INT32 _rtnContextParaData::_checkAndPrefetch ()
   {
      INT32 rc = SDB_OK ;
      rtnContextData *pContext = NULL ;
      vector< rtnContextData* >::iterator it = _vecContext.begin() ;
      while ( it != _vecContext.end() )
      {
         pContext = *it ;
         if ( pContext->eof() && pContext->isEmpty() )
         {
            pContext->waitForPrefetch() ;
            SDB_OSS_DEL pContext ;
            it = _vecContext.erase( it ) ;
            continue ;
         }
         else if ( !pContext->isEmpty() ||
                   pContext->_getWaitPrefetchNum() > 0 )
         {
            ++it ;
            continue ;
         }
         pContext->_onDataEmpty() ;
         ++it ;
      }

      if ( _vecContext.size() == 0 )
      {
         rc = SDB_DMS_EOC ;
         _hitEnd = TRUE ;
      }

      return rc ;
   }

   const BSONObj* _rtnContextParaData::_nextBlockObj ()
   {
      BSONArrayBuilder builder ;
      UINT32 curIndex = _curIndex ;

      if ( _curIndex >= _step ||
           ( TBSCAN == _scanType && _curIndex >= _segments.size() ) ||
           ( IXSCAN == _scanType && _curIndex + 1 >= _indexBlocks.size() ) )
      {
         return NULL ;
      }
      ++_curIndex ;

      if ( TBSCAN == _scanType )
      {
         while ( curIndex < _segments.size() )
         {
            builder.append( _segments[curIndex] ) ;
            curIndex += _step ;
         }
      }
      else if ( IXSCAN == _scanType )
      {
         while ( curIndex + 1 < _indexBlocks.size() )
         {
            builder.append( BSON( FIELD_NAME_STARTKEY <<
                                  _indexBlocks[curIndex] <<
                                  FIELD_NAME_ENDKEY <<
                                  _indexBlocks[curIndex+1] <<
                                  FIELD_NAME_STARTRID <<
                                  BSON_ARRAY( _indexRIDs[curIndex]._extent <<
                                              _indexRIDs[curIndex]._offset ) <<
                                  FIELD_NAME_ENDRID <<
                                  BSON_ARRAY( _indexRIDs[curIndex+1]._extent <<
                                              _indexRIDs[curIndex+1]._offset )
                                 )
                            ) ;
            curIndex += _step ;
         }
      }
      else
      {
         return NULL ;
      }

      _blockObj = builder.arr() ;
      return &_blockObj ;
   }

   INT32 _rtnContextParaData::_getSubCtxWithData( rtnContextData **ppContext,
                                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      UINT32 index = 0 ;

      do
      {
         index = 0 ;
         _prefWather.reset() ;

         while ( index < _vecContext.size() )
         {
            rc = _vecContext[index]->prefetchResult () ;
            if ( rc && SDB_DMS_EOC != rc )
            {
               goto error ;
            }
            rc = SDB_OK ;

            if ( !_vecContext[index]->isEmpty() &&
                 !_vecContext[index]->_isInPrefetching () )
            {
               *ppContext = _vecContext[index] ;
               goto done ;
            }
            ++index ;
         }
      } while ( _prefWather.waitDone( OSS_ONE_SEC * 5 ) > 0 ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextParaData::_doAdvance( INT32 type,
                                          INT32 prefixNum,
                                          const BSONObj &keyVal,
                                          const BSONObj &orderby,
                                          const BSONObj &arg,
                                          BOOLEAN isLocate,
                                          _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;

      if ( !_isParalled )
      {
         rc = _rtnContextData::_doAdvance( type, prefixNum, keyVal, orderby,
                                           arg, isLocate, cb ) ;
      }
      else
      {
         rc = _rtnContextBase::_doAdvance( type, prefixNum, keyVal, orderby,
                                           arg, isLocate, cb ) ;
      }

      return rc ;
   }

   INT32 _rtnContextParaData::_getSubContextData( pmdEDUCB * cb )
   {
      INT32 rc = SDB_OK ;
      rtnContextData *pContext = NULL ;
      INT64 maxReturnNum = -1 ;
      INT32 startNumRecords = numRecords();

      while ( numRecords() == startNumRecords && 0 != _numToReturn )
      {
         pContext = NULL ;
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( _numToSkip <= 0 )
         {
            rc = _getSubCtxWithData( &pContext, cb ) ;
            if ( rc )
            {
               goto error ;
            }
         }

         if ( !pContext && _vecContext.size() > 0 )
         {
            pContext = _vecContext[0] ;
         }

         // get data
         if ( pContext )
         {
            rtnContextBuf buffObj ;
            if ( _numToSkip > 0 )
            {
               maxReturnNum = _numToSkip ;
            }
            else
            {
               maxReturnNum = -1 ;
            }

            // get data
            rc = pContext->getMore( maxReturnNum, buffObj, cb ) ;
            if ( rc )
            {
               _removeSubContext( pContext ) ;
               if ( SDB_DMS_EOC != rc )
               {
                  PD_LOG( PDERROR, "Failed to get more from sub context, "
                          "rc: %d", rc ) ;
                  goto error ;
               }
               continue ;
            }

            if ( _numToSkip > 0 )
            {
               _numToSkip -= buffObj.recordNum() ;
               continue ;
            }

            if ( _numToReturn > 0 && buffObj.recordNum() > _numToReturn )
            {
               buffObj.truncate( _numToReturn ) ;
            }
            // append data
            rc = appendObjs( buffObj.data(), buffObj.size(),
                             buffObj.recordNum() ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add objs, rc: %d", rc ) ;
            if ( _numToReturn > 0 )
            {
               _numToReturn -= buffObj.recordNum() ;
            }
         } // end if ( pContext )

         if ( SDB_OK != _checkAndPrefetch() )
         {
            break ;
         }
      } // while ( isEmpty() && 0 != _numToReturn )

      if ( 0 == _numToReturn )
      {
         _hitEnd = TRUE ;
      }

      if ( !isEmpty() )
      {
         rc = SDB_OK ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextParaData::_prepareData( pmdEDUCB * cb )
   {
      if ( !_isParalled )
      {
         return _rtnContextData::_prepareData( cb ) ;
      }
      else
      {
         return _getSubContextData( cb ) ;
      }
   }

   /*
      _rtnContextTemp implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextTemp, RTN_CONTEXT_TEMP, "TEMP")

   _rtnContextTemp::_rtnContextTemp( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _suLID( DMS_INVALID_LOGICCSID ),
    _mbLID( DMS_INVALID_LOGICCLID )
   {
   }

   _rtnContextTemp::~_rtnContextTemp ()
   {
   }

   const CHAR* _rtnContextTemp::name() const
   {
      return "TEMP" ;
   }

   RTN_CONTEXT_TYPE _rtnContextTemp::getType () const
   {
      return RTN_CONTEXT_TEMP ;
   }

   INT32 _rtnContextTemp::open( UINT32 suLID,
                                UINT32 mbLID,
                                const CHAR *csName,
                                const CHAR *clShortName,
                                const CHAR *optrDesc )
   {
      INT32 rc = SDB_OK ;

      if ( _isOpened )
      {
         rc = SDB_DMS_CONTEXT_IS_OPEN ;
         goto error ;
      }

      try
      {
         _processName.clear() ;
         _optrDesc.clear() ;

         if ( NULL != csName )
         {
            _processName.assign( csName ) ;
            if ( NULL != clShortName )
            {
               _processName.append( "." ) ;
               _processName.append( clShortName ) ;
            }
         }

         if ( NULL != optrDesc )
         {
            _optrDesc.assign( optrDesc ) ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to open temp context for collection, "
                 "occur exception %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      _suLID = suLID ;
      _mbLID = mbLID ;
      _isOpened = TRUE ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _rtnContextTemp::_prepareData( pmdEDUCB * cb )
   {
      SDB_ASSERT( FALSE, "should not be here" ) ;
      return SDB_DMS_EOC ;
   }

   void _rtnContextTemp::_toString( stringstream &ss )
   {
      try
      {
         ss << ",Name:" << _processName << ",Detail:" << _optrDesc ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDWARNING, "Failed to build string for temp context, "
                 "occur exception %s", e.what() ) ;
      }
   }

}
