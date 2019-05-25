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

   Source File Name = optStatUnit.hpp

   Descriptive Name = Optimizer Statistics Object Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Statistics
   Objects.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef OPTSTATUNIT_HPP__
#define OPTSTATUNIT_HPP__

#include "dmsStatUnit.hpp"
#include "dmsSUCache.hpp"
#include "rtnPredicate.hpp"
#include "ixm.hpp"
#include "utilList.hpp"
#include "../bson/bson.h"

using namespace std ;
using namespace bson ;

namespace engine
{

   class _dmsMBContext ;

   class _optCollectionStat ;
   typedef _optCollectionStat optCollectionStat ;

   typedef _utilList<rtnPredicate *> rtnStatPredList ;

   /*
      _optStatListKey define
    */
   class _optStatListKey : public _utilList<const rtnKeyBoundary *>,
                           public _dmsStatKey
   {
      public :
         _optStatListKey () ;

         virtual ~_optStatListKey () {}

         virtual INT32 compareValue( INT32 cmpFlag, INT32 incFlag,
                                     const BSONObj &rValue ) ;

         virtual BOOLEAN compareAllValues ( UINT32 startIdx, INT32 cmpFlag,
                                            const BSONObj &rValue ) ;

         virtual string toString () ;

         OSS_INLINE virtual UINT32 size ()
         {
            return _utilList<const rtnKeyBoundary *>::size() ;
         }

         OSS_INLINE virtual const BSONElement &firstElement ()
         {
            return front()->_bound ;
         }

         OSS_INLINE void pushKeyBound ( const rtnKeyBoundary *pValue )
         {
            SDB_ASSERT( pValue, "Should not input NULL value" ) ;
            push_back( pValue ) ;
            _included = _included && pValue->_inclusive ;
         }

         OSS_INLINE void popElement ( BOOLEAN included )
         {
            pop_back() ;
            _included = included ;
         }
      protected :

   } ;

   typedef class _optStatListKey optStatListKey ;

   /*
      _optStatElementKey define
    */
   class _optStatElementKey : public BSONElement,
                              public _dmsStatKey
   {
      public :
         explicit _optStatElementKey ( const BSONElement &element,
                                       BOOLEAN included ) ;

         virtual ~_optStatElementKey () {}

         virtual INT32 compareValue ( INT32 cmpFlag, INT32 incFlag,
                                      const BSONObj &rValue ) ;

         virtual BOOLEAN compareAllValues ( UINT32 startIdx, INT32 cmpFlag,
                                            const BSONObj &rValue ) ;

         OSS_INLINE virtual string toString ()
         {
            return BSONElement::toString( TRUE, TRUE ) ;
         }

         OSS_INLINE virtual UINT32 size ()
         {
            return 1 ;
         }

         OSS_INLINE virtual const BSONElement &firstElement ()
         {
            return (const BSONElement &)(*this) ;
         }
   } ;

   typedef class _optStatElementKey optStatElementKey ;

   /*
      _optStatUnit define
    */
   class _optStatUnit : public SDBObject
   {
      public :
         _optStatUnit ( UINT64 totalRecords )
         : _totalRecords( totalRecords )
         {
         }

         virtual ~_optStatUnit () {}

         virtual UINT64 getTotalRecords ( BOOLEAN realTime = FALSE ) const = 0 ;

         virtual double evalPredicate ( const CHAR *pFieldName,
                                        rtnPredicate &predicate,
                                        BOOLEAN mixCmp,
                                        BOOLEAN &isAllRange ) const ;

         virtual double evalKeyPair ( const CHAR *pFieldName,
                                      dmsStatKey &startKey,
                                      dmsStatKey &stopKey,
                                      BOOLEAN isEqual,
                                      INT32 majorType,
                                      BOOLEAN mixCmp,
                                      double &scanSelectivity ) const = 0 ;

         virtual BOOLEAN isValid () const = 0 ;

         virtual UINT64 getCreateTime () const = 0 ;

      protected :
         INT32 _evalKeyPair ( const dmsIndexStat *pIndexStat,
                              rtnStatPredList::iterator &predIter,
                              optStatListKey &startKeys,
                              optStatListKey &stopKeys,
                              BOOLEAN isEqual,
                              double &predSelectivity,
                              double &scanSelectivity ) const ;

      protected :
         UINT64 _totalRecords ;
   } ;

   /*
      _optIndexStat define
    */
   class _optIndexStat : public _optStatUnit
   {
      public :
         _optIndexStat ( const optCollectionStat &collectionStat,
                         const ixmIndexCB &indexCB ) ;

         virtual ~_optIndexStat () {}

         OSS_INLINE const dmsIndexStat *getIndexStat () const
         {
            return _pIndexStat ;
         }

         OSS_INLINE const BSONObj &getKeyPattern () const
         {
            return _keyPattern ;
         }

         OSS_INLINE virtual UINT64 getTotalRecords ( BOOLEAN realTime = FALSE ) const
         {
            return ( _pIndexStat && !realTime ) ?
                   _pIndexStat->getTotalRecords() : _totalRecords ;
         }

         OSS_INLINE UINT32 getIndexPages ( BOOLEAN realTime = FALSE ) const ;

         OSS_INLINE UINT32 getIndexLevels () const
         {
            return _pIndexStat ? _pIndexStat->getIndexLevels() :
                                 DMS_STAT_DEF_IDX_LEVELS ;
         }

         OSS_INLINE virtual BOOLEAN isValid () const
         {
            return ( _pIndexStat && _pIndexStat->isValidForEstimate() ) ;
         }

         double evalPredicateList ( const CHAR *pFieldName,
                                    rtnStatPredList &predList,
                                    BOOLEAN mixCmp,
                                    double &scanSelectivity ) const ;

         virtual double evalKeyPair ( const CHAR *pFieldName,
                                      dmsStatKey &startKey,
                                      dmsStatKey &stopKey,
                                      BOOLEAN isEqual,
                                      INT32 majorType,
                                      BOOLEAN mixCmp,
                                      double &scanSelectivity ) const ;

         OSS_INLINE virtual UINT64 getCreateTime () const
         {
            return ( isValid() ? _pIndexStat->getCreateTime() : 0 ) ;
         }

      protected :
         const optCollectionStat &  _collectionStat ;
         const dmsIndexStat *       _pIndexStat ;

         BSONObj                    _keyPattern ;
   } ;

   typedef _optIndexStat optIndexStat ;

   /*
      _optCollectionStat define
    */
   class _optCollectionStat : public _optStatUnit
   {
      public :
         _optCollectionStat ( UINT32 pageSize,
                              _dmsMBContext *mbContext,
                              const dmsStatCache *statCache ) ;

         virtual ~_optCollectionStat () {}

         OSS_INLINE const dmsCollectionStat *getCollectionStat () const
         {
            return _pCollectionStat ;
         }

         OSS_INLINE const dmsIndexStat *getIndexStat ( const CHAR *pIndexName ) const
         {
            return ( _pCollectionStat && pIndexName ) ?
                        _pCollectionStat->getIndexStat( pIndexName ) : NULL ;
         }

         OSS_INLINE const dmsIndexStat *getFieldStat ( const CHAR *pFieldName ) const
         {
            return ( _pCollectionStat && pFieldName ) ?
                     _pCollectionStat->getFieldStat( pFieldName ) : NULL ;
         }

         OSS_INLINE virtual UINT64 getTotalRecords ( BOOLEAN realTime = FALSE ) const
         {
            return ( _pCollectionStat && !realTime ) ?
                   _pCollectionStat->getTotalRecords() : _totalRecords ;
         }

         OSS_INLINE UINT32 getTotalDataPages ( BOOLEAN realTime = FALSE ) const
         {
            return ( _pCollectionStat && !realTime ) ?
                   _pCollectionStat->getTotalDataPages() : _totalDataPages ;
         }

         OSS_INLINE UINT64 getTotalDataSize ( BOOLEAN realTime = FALSE ) const
         {
            return ( _pCollectionStat && !realTime ) ?
                   _pCollectionStat->getTotalDataSize() : _totalDataSize ;
         }

         OSS_INLINE UINT32 getPageSize () const
         {
            return _pageSize ;
         }

         OSS_INLINE UINT32 getNumIndexes () const
         {
            return _numIndexes ;
         }

         OSS_INLINE UINT32 getTotalIndexPages () const
         {
            return _totalIndexPages ;
         }

         OSS_INLINE UINT64 getTotalIndexSize () const
         {
            return _totalIndexSize ;
         }

         OSS_INLINE UINT32 getAvgIndexPages () const
         {
            return _avgIndexPages ;
         }

         OSS_INLINE UINT64 getAvgIndexSize () const
         {
            return _avgIndexSize ;
         }

         OSS_INLINE UINT32 getAvgNumFields () const
         {
            return _pCollectionStat ? _pCollectionStat->getAvgNumFields() :
                                      DMS_STAT_DEF_AVG_NUM_FIELDS ;
         }

         OSS_INLINE virtual BOOLEAN isValid () const
         {
            return ( _pCollectionStat != NULL ) ;
         }

         OSS_INLINE virtual UINT64 getCreateTime () const
         {
            return ( isValid() ? _pCollectionStat->getCreateTime() : 0 ) ;
         }

         INT32 initCurStat ( _dmsMBContext *mbContext ) ;

         double evalPredicateSet ( rtnPredicateSet &predicateSet,
                                   BOOLEAN mixCmp,
                                   double &scanSelectivity ) ;

         virtual double evalKeyPair ( const CHAR *pFieldName,
                                      dmsStatKey &startKey,
                                      dmsStatKey &stopKey,
                                      BOOLEAN isEqual,
                                      INT32 majorType,
                                      BOOLEAN mixCmp,
                                      double &scanSelectivity ) const ;

         double evalETOpterator ( const CHAR *pFieldName,
                                  const BSONElement &beValue ) const ;

         double evalGTOpterator ( const CHAR *pFieldName,
                                  const BSONElement &beValue,
                                  BOOLEAN included ) const ;

         double evalLTOpterator ( const CHAR *pFieldName,
                                  const BSONElement &beValue,
                                  BOOLEAN included ) const ;

         OSS_INLINE BOOLEAN isBestIndex ( const optIndexStat *indexStat ) const
         {
            if ( _bestIndexStat )
            {
               return indexStat->getIndexStat() == _bestIndexStat ;
            }
            return FALSE ;
         }

         OSS_INLINE const CHAR *getBestIndexName () const
         {
            return _bestIndexStat ? _bestIndexStat->getIndexName() : NULL ;
         }

         OSS_INLINE void _setBestIndex ( const dmsIndexStat *pIndexStat )
         {
            _bestIndexStat = pIndexStat ;
         }

      protected :
         double _evalKeyPair ( const BSONElement &startKey,
                               BOOLEAN startIncluded,
                               const BSONElement &stopKey,
                               BOOLEAN stopIncluded,
                               INT32 majorType,
                               BOOLEAN mixCmp ) const ;

         double _evalETOperator ( const BSONElement &beValue ) const ;

         double _evalRangeOperator ( const BSONElement &beStart,
                                     BOOLEAN startIncluded,
                                     const BSONElement &beStop,
                                     BOOLEAN stopIncluded ) const ;

         double _evalGTOperator ( const BSONElement &beStart,
                                  BOOLEAN startIncluded ) const ;

         double _evalLTOperator ( const BSONElement &beStop,
                                  BOOLEAN stopIncluded ) const ;

         const dmsIndexStat *_getMatchedIndex ( rtnPredicateSet &predicates ) const ;

      protected :
         /* Init from dmsMBContext */
         UINT32            _pageSize ;
         UINT32            _totalDataPages ;
         UINT64            _totalDataSize ;

         UINT32            _numIndexes ;
         UINT32            _totalIndexPages ;
         UINT64            _totalIndexSize ;
         UINT32            _avgIndexPages ;
         UINT64            _avgIndexSize ;

         const dmsCollectionStat *  _pCollectionStat ;
         const dmsIndexStat *       _bestIndexStat ;
   } ;

   /*
      _optIndexStat inline functions
    */
   UINT32 _optIndexStat::getIndexPages ( BOOLEAN realTime ) const
   {
      return ( _pIndexStat && !realTime ) ?
             _pIndexStat->getIndexPages() : _collectionStat.getAvgIndexPages() ;
   }

}

#endif //OPTSTATUNIT_HPP__

