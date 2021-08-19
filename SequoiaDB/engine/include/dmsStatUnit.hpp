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

   Source File Name = dmsStatsUnit.hpp

   Descriptive Name = DMS Statistics Units Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   statistics objects.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#ifndef DMSSTATUNIT_HPP__
#define DMSSTATUNIT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "utilMap.hpp"
#include "utilSUCache.hpp"
#include "utilString.hpp"
#include "../bson/bson.h"
#include <cmath>

using namespace std ;
using namespace bson ;

namespace engine
{
   // Fields to create indexes of SYSSTAT collections
   #define DMS_STAT_COLLECTION_SPACE           "CollectionSpace"
   #define DMS_STAT_COLLECTION                 "Collection"

   #define DMS_STAT_IDX_INDEX                  "Index"

   #define DMS_STAT_IDX_MCV                    "MCV"

   #define DMS_STAT_DEF_VERSION                ( 0 )
   #define DMS_STAT_DEF_AVG_NUM_FIELDS         ( 10 )
   #define DMS_STAT_DEF_TOTAL_PAGES            ( 1 )
   #define DMS_STAT_DEF_IDX_LEVELS             ( 1 )
   #define DMS_STAT_DEF_TOTAL_RECORDS          ( 200 )
   #define DMS_STAT_DEF_DATA_SIZE              ( 400 )

   // Default selectivity of a range predicate
   #define DMS_STAT_PRED_RANGE_DEF_SELECTIVITY ( 0.05 )

   // Default selectivity of a $et predicate
   #define DMS_STAT_PRED_EQ_DEF_SELECTIVITY    ( 0.005 )

   #define DMS_STAT_ROUND( x, min, max ) \
           ( OSS_MIN( OSS_MAX( ( x ), ( min ) ), ( max ) ) )

   #define DMS_STAT_ROUND_SELECTIVITY( x ) \
           DMS_STAT_ROUND( ( x ), ( 0.0 ), ( 1.0 ) )

   #define DMS_STAT_FRACTION_SCALE             ( 10000 )

   #define DMS_STAT_ROUND_INT( x ) \
           ( ( ( x ) >= 0.0 ) ? floor( ( x ) + 0.5 ) : ceil( ( x ) - 0.5 ) )

   /*
      _dmsStatKey define
    */
   class _dmsStatKey
   {
      public :
         _dmsStatKey ( BOOLEAN included = TRUE )
         : _included( included )
         {
         }

         virtual ~_dmsStatKey () {}

         virtual INT32 compareValue ( INT32 cmpFlag, INT32 incFlag,
                                      const BSONObj &rValue ) = 0 ;

         virtual BOOLEAN compareAllValues ( UINT32 startIdx, INT32 cmpFlag,
                                            const BSONObj &rValue ) = 0 ;

         virtual string toString () = 0 ;

         virtual UINT32 size () = 0 ;

         virtual const BSONElement &firstElement () = 0 ;

         OSS_INLINE BOOLEAN isIncluded () const
         {
            return _included ;
         }

         OSS_INLINE void setIncluded ( BOOLEAN included )
         {
            _included = included ;
         }

      protected :
         OSS_INLINE INT32 _equalButLeftMore ( INT32 incFlag )
         {
            // The compared elements are equal, but left has more elements,
            // normally it is left > right
            // But if incFlag is -1 which means a virtual $minKey is appended
            // left, so right > left
            return incFlag < 0 ? -1 : 1 ;
         }

         OSS_INLINE INT32 _equalButRightMore ( INT32 incFlag )
         {
            // The compared elements are equal, but right has more elements,
            // normally it is left < right
            // But if incFlag is 1 which means a virtual $maxKey is appended to
            // left, so left > right
            return incFlag > 0 ? 1 : -1 ;
         }

         OSS_INLINE INT32 _equalDefault ( INT32 incFlag )
         {
            // The compared elements are equal
            // If $maxKey is appended to left, left > right
            // If $minKey is appended to left, left < right
            return incFlag > 0 ? 1 : ( incFlag < 0 ? -1 : 0 ) ;
         }

      protected :
         BOOLEAN _included ;
   } ;

   typedef class _dmsStatKey dmsStatKey ;

   /*
      _dmsStatValues define
    */
   class _dmsStatValues : public SDBObject
   {
      public :
         _dmsStatValues () ;

         virtual ~_dmsStatValues () ;

         INT32 init ( UINT32 size, UINT32 allocSize ) ;

         INT32 pushBack ( const BSONObj &boValue ) ;

         INT32 binarySearch ( dmsStatKey &keyValue, INT32 cmpFlag,
                              INT32 keyIncFlag, BOOLEAN &isEqual ) const ;

         OSS_INLINE UINT32 getSize () const
         {
            return _size ;
         }

         OSS_INLINE void setValue ( UINT32 idx, const BSONObj &boValue )
         {
            if ( idx < _size )
            {
               _pValues[ idx ] = boValue.getOwned() ;
            }
         }

         OSS_INLINE const BSONObj &getValue ( UINT32 idx ) const
         {
            SDB_ASSERT( idx < _size, "Wrong index" ) ;
            return _pValues[ idx ] ;
         }

         INT32 checkValues ( UINT32 numKeys, const BSONObj &keyPattern ) ;

      protected :

         void _clear () ;

         BOOLEAN _inRange ( UINT32 idx, dmsStatKey *pStartKey,
                            dmsStatKey *pStopKey ) const ;

      protected :
         UINT32            _numKeys ;
         UINT32            _size ;
         UINT32            _allocSize ;
         BSONObj *         _pValues ;
   } ;

   /*
      _dmsStatMCVSet define
    */
   class _dmsStatMCVSet : public _dmsStatValues
   {
      public :
         _dmsStatMCVSet () ;

         virtual ~_dmsStatMCVSet () ;

         INT32 init ( UINT32 size, UINT32 allocSize ) ;

         INT32 pushBack ( const BSONObj &boValue, UINT16 fraction ) ;

         OSS_INLINE void setFrac ( UINT32 idx, UINT16 fraction )
         {
            if ( idx < _size )
            {
               _pFractions[ idx ] = fraction ;
            }
         }

         OSS_INLINE double getFrac ( UINT32 idx ) const
         {
            if ( idx < _size )
            {
               return (double)_pFractions[ idx ] / (double)DMS_STAT_FRACTION_SCALE ;
            }
            return 0.0 ;
         }

         OSS_INLINE UINT16 getFracInt ( UINT32 idx ) const
         {
            if ( idx < _size )
            {
               return _pFractions[ idx ] ;
            }
            return 0 ;
         }

         OSS_INLINE void setTotalFrac ()
         {
            UINT16 totalFrac = 0 ;
            for ( UINT32 i = 0 ; i < _size ; i++ )
            {
               totalFrac += _pFractions[ i ] ;
            }
            totalFrac = DMS_STAT_ROUND( totalFrac, 0, DMS_STAT_FRACTION_SCALE ) ;
            _totalFrac = totalFrac ;
         }

         OSS_INLINE double getTotalFrac () const
         {
            return (double)_totalFrac / (double)DMS_STAT_FRACTION_SCALE ;
         }

         void clear () ;

         INT32 evalOperator ( dmsStatKey *pStartKey, dmsStatKey *pStopKey,
                              BOOLEAN &hitMCV,
                              double &predSelectivity,
                              double &scanSelectivity ) const ;

         INT32 evalETOperator ( dmsStatKey &key, BOOLEAN &hitMCV,
                                double &predSelectivity,
                                double &scanSelectivity ) const ;

      protected :
         UINT16 *          _pFractions ;
         UINT16            _totalFrac ;
   } ;

   typedef class _dmsStatMCVSet dmsStatMCVSet ;

   /*
      _dmsStatUnit define
    */
   class _dmsStatUnit : public _utilSUCacheUnit
   {
      public :
         _dmsStatUnit () ;

         // For statistics cache, mbID is ID of unit
         _dmsStatUnit ( UINT32 suLID, UINT16 mbID, UINT32 clLID,
                        UINT64 createTime ) ;

         virtual ~_dmsStatUnit () {}

         virtual const CHAR *getCSName () const = 0 ;

         virtual void setCSName ( const CHAR *pCSName ) = 0 ;

         virtual const CHAR *getCLName () const = 0 ;

         virtual void setCLName ( const CHAR *pCLName ) = 0 ;

         OSS_INLINE UINT16 getMBID () const
         {
            // For statistics cache, mbID is ID of unit
            return getUnitID() ;
         }

         OSS_INLINE void setMBID( UINT16 mbID )
         {
            // For statistics cache, mbID is ID of unit
            _setUnitID( mbID ) ;
         }

         OSS_INLINE UINT32 getSULogicalID () const
         {
            return _suLogicalID ;
         }

         OSS_INLINE void setSULogicalID ( UINT32 suLogicalID )
         {
            _suLogicalID = suLogicalID ;
         }

         OSS_INLINE UINT32 getCLLogicalID () const
         {
            return _clLogicalID ;
         }

         OSS_INLINE void setCLLogicalID ( UINT32 clLogicalID )
         {
            _clLogicalID = clLogicalID ;
         }

         OSS_INLINE UINT64 getSampleRecords () const
         {
            return _sampleRecords ;
         }

         OSS_INLINE void setSampleRecords( UINT64 sampleRecords )
         {
            _sampleRecords = sampleRecords ;
         }

         OSS_INLINE UINT64 getTotalRecords () const
         {
            return _totalRecords ;
         }

         OSS_INLINE void setTotalRecords ( UINT64 totalRecords )
         {
            _totalRecords = totalRecords ;
         }

         INT32 init ( const BSONObj &boStat ) ;

         INT32 postInit () ;

         virtual BSONObj toBSON () const ;

      protected :
         virtual INT32 _initItem ( const BSONObj &boStat ) = 0 ;

         virtual INT32 _postInit () = 0 ;

         virtual void _toBSON ( BSONObjBuilder &builder ) const = 0 ;

      protected :
         // Number of records in the sample
         UINT64   _sampleRecords ;

         // Number of records in the collection when collecting this statistics
         UINT64   _totalRecords ;

         UINT32   _suLogicalID ;
         UINT32   _clLogicalID ;
   } ;

   /*
      _dmsIndexStat define
    */
   class _dmsIndexStat : public _dmsStatUnit
   {
      typedef _utilString<128>   idxNameString ;

      public :
         _dmsIndexStat () ;

         _dmsIndexStat ( const CHAR *pCSName, const CHAR *pCLName,
                         const CHAR *pIndexName, UINT32 suLID, UINT16 mbID,
                         UINT32 clLID, UINT64 createTime ) ;

         virtual ~_dmsIndexStat () ;

         OSS_INLINE virtual void setCSName ( const CHAR *pCSName )
         {
            if ( pCSName && *pCSName )
            {
               ossStrncpy( _pCSName, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
            }
            else
            {
               _pCSName[ 0 ] = '\0' ;
            }
         }

         OSS_INLINE virtual const CHAR *getCSName () const
         {
            return _pCSName ;
         }

         OSS_INLINE virtual void setCLName ( const CHAR *pCLName )
         {
            if ( pCLName && *pCLName )
            {
               ossStrncpy( _pCLName, pCLName, DMS_COLLECTION_NAME_SZ ) ;
            }
            else
            {
               _pCLName[ 0 ] = '\0' ;
            }
         }

         OSS_INLINE virtual const CHAR *getCLName () const
         {
            return _pCLName ;
         }

         OSS_INLINE virtual UINT8 getUnitType () const
         {
            return UTIL_SU_CACHE_UNIT_IXSTAT ;
         }

         OSS_INLINE virtual BOOLEAN addSubUnit ( utilSUCacheUnit *pSubUnit,
                                                 BOOLEAN ignoreCrtTime )
         {
            return FALSE ;
         }

         OSS_INLINE virtual void clearSubUnits () {}

         OSS_INLINE const CHAR *getIndexName () const
         {
            return _pIndexName.str() ;
         }

         OSS_INLINE void setIndexName ( const CHAR *pIndexName )
         {
            if ( pIndexName )
            {
               _pIndexName.append( pIndexName ) ;
            }
            else
            {
               _pIndexName.clear() ;
            }
         }

         OSS_INLINE dmsExtentID getIndexLogicalID () const
         {
            return _indexLogicalID ;
         }

         OSS_INLINE void setIndexLogicalID( dmsExtentID indexLogicalID )
         {
            _indexLogicalID = indexLogicalID ;
         }

         OSS_INLINE const BSONObj &getKeyPattern () const
         {
            return _keyPattern ;
         }

         OSS_INLINE void setKeyPattern ( const BSONObj &keyPattern )
         {
            _initKeyPattern( keyPattern ) ;
         }

         OSS_INLINE const CHAR *getFirstField () const
         {
            return _pFirstField ;
         }

         OSS_INLINE UINT32 getNumKeys () const
         {
            return _numKeys ;
         }

         OSS_INLINE UINT32 getIndexPages () const
         {
            return _indexPages ;
         }

         OSS_INLINE void setIndexPages( UINT32 indexPages )
         {
            _indexPages = indexPages ;
         }

         OSS_INLINE UINT32 getIndexLevels () const
         {
            return _indexLevels ;
         }

         OSS_INLINE void setIndexLevels ( UINT32 indexLevels )
         {
            _indexLevels = indexLevels ;
         }

         OSS_INLINE BOOLEAN isUnique () const
         {
            return _isUnique ;
         }

         OSS_INLINE void setUnique ( BOOLEAN isUnique )
         {
            _isUnique = isUnique ;
         }

         OSS_INLINE UINT64 getDistinctValues () const
         {
            return _distinctValues ;
         }

         OSS_INLINE void setDistinctValues ( UINT64 distinctValues )
         {
            _distinctValues = distinctValues ;
         }

         OSS_INLINE double getNullFrac () const
         {
            return (double)_nullFrac / (double)DMS_STAT_FRACTION_SCALE ;
         }

         OSS_INLINE void setNullFrac ( UINT16 nullFrac )
         {
            _nullFrac = DMS_STAT_ROUND( nullFrac, 0, DMS_STAT_FRACTION_SCALE ) ;
         }

         OSS_INLINE double getUndefFrac () const
         {
            return (double)_undefFrac / (double)DMS_STAT_FRACTION_SCALE ;
         }

         OSS_INLINE void setUndefFrac ( UINT16 undefFrac )
         {
            _undefFrac = DMS_STAT_ROUND( undefFrac, 0, DMS_STAT_FRACTION_SCALE ) ;
         }

         INT32 initMCVSet ( UINT32 allocSize ) ;

         INT32 pushMCVSet ( const BSONObj &boValue, double fraction ) ;

         INT32 evalRangeOperator ( dmsStatKey &startKey,
                                   dmsStatKey &stopKey,
                                   double &predSelectivity,
                                   double &scanSelectivity ) const ;

         INT32 evalETOperator ( dmsStatKey &key,
                                double &predSelectivity,
                                double &scanSelectivity ) const ;

         INT32 evalGTOperator ( dmsStatKey &startKey,
                                double &predSelectivity,
                                double &scanSelectivity ) const ;

         INT32 evalLTOperator ( dmsStatKey &stopKey,
                                double &predSelectivity,
                                double &scanSelectivity ) const ;

         OSS_INLINE BOOLEAN isValidForEstimate () const
         {
            return _mcvSet.getSize() > 0 ;
         }

      protected :
         virtual INT32 _initItem ( const BSONObj &boStat ) ;
         virtual INT32 _postInit () ;
         virtual void _toBSON ( BSONObjBuilder &builder ) const ;

         INT32 _initKeyPattern ( const BSONObj &boKeyPattern ) ;
         INT32 _initMCV ( const BSONObj &boMCV ) ;

         INT32 _evalOperator ( dmsStatKey *pStartKey, dmsStatKey *pStopKey,
                               double &predSelectivity, double &scanSelectivity ) const ;

      protected :
         // Name of collection space
         CHAR     _pCSName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;

         // Name of collection ( short name )
         CHAR     _pCLName [ DMS_COLLECTION_NAME_SZ + 1 ] ;

         idxNameString     _pIndexName ;

         dmsExtentID       _indexLogicalID ;

         // Definition of the index
         BSONObj           _keyPattern ;

         // First field in the index
         const CHAR *      _pFirstField ;

         // Number of keys in the index
         UINT32            _numKeys ;

         // Number of index pages
         UINT32            _indexPages ;

         // Number of index levels
         UINT32            _indexLevels ;

         // Is a unique index
         BOOLEAN           _isUnique ;

         // Number of distinct values in the index
         UINT64            _distinctValues ;

         UINT16            _nullFrac ;
         UINT16            _undefFrac ;

         dmsStatMCVSet     _mcvSet ;
   } ;

   typedef _dmsIndexStat dmsIndexStat ;

   typedef _utilMap< dmsExtentID, dmsIndexStat * > INDEX_STAT_MAP ;
   typedef INDEX_STAT_MAP::iterator INDEX_STAT_ITERATOR ;
   typedef INDEX_STAT_MAP::const_iterator INDEX_STAT_CONST_ITERATOR ;

   typedef _utilStringMap< dmsIndexStat * > FIELD_STAT_MAP ;
   typedef FIELD_STAT_MAP::iterator FIELD_STAT_ITERATOR ;
   typedef FIELD_STAT_MAP::const_iterator FIELD_STAT_CONST_ITERATOR ;

   /*
      _dmsCollectionStat define
    */
   class _dmsCollectionStat : public _dmsStatUnit
   {
      public :
         _dmsCollectionStat () ;

         _dmsCollectionStat ( const CHAR *pCSName, const CHAR *pCLName,
                              UINT32 suLID, UINT16 mbID, UINT32 clLID,
                              UINT64 createTime ) ;

         virtual ~_dmsCollectionStat () ;

         OSS_INLINE virtual const CHAR *getCSName () const
         {
            return _pCSName ;
         }

         OSS_INLINE virtual void setCSName ( const CHAR *pCSName )
         {
            if ( pCSName && *pCSName )
            {
               ossStrncpy( _pCSName, pCSName, DMS_COLLECTION_SPACE_NAME_SZ ) ;
            }
            else
            {
               _pCSName[ 0 ] = '\0' ;
            }
         }

         OSS_INLINE virtual const CHAR *getCLName () const
         {
            return _pCLName ;
         }

         OSS_INLINE virtual void setCLName ( const CHAR *pCLName )
         {
            if ( pCLName && *pCLName )
            {
               ossStrncpy( _pCLName, pCLName, DMS_COLLECTION_NAME_SZ ) ;
            }
            else
            {
               _pCLName[ 0 ] = '\0' ;
            }
         }

         OSS_INLINE virtual UINT8 getUnitType () const
         {
            return UTIL_SU_CACHE_UNIT_CLSTAT ;
         }

         OSS_INLINE UINT32 getTotalDataPages () const
         {
            return _totalDataPages ;
         }

         OSS_INLINE void setTotalDataPages ( UINT32 totalDataPages )
         {
            _totalDataPages = totalDataPages ;
         }

         OSS_INLINE UINT64 getTotalDataSize () const
         {
            return _totalDataSize ;
         }

         OSS_INLINE void setTotalDataSize ( UINT64 totalDataSize )
         {
            _totalDataSize = totalDataSize ;
         }

         OSS_INLINE UINT32 getAvgNumFields () const
         {
            return _avgNumFields ;
         }

         OSS_INLINE void setAvgNumFields ( UINT32 avgNumFields )
         {
            _avgNumFields = avgNumFields ;
         }

         OSS_INLINE const INDEX_STAT_MAP & getIndexStats () const
         {
            return _indexStats ;
         }

         OSS_INLINE INDEX_STAT_MAP & getIndexStats ()
         {
            return _indexStats ;
         }

         virtual BOOLEAN addSubUnit ( utilSUCacheUnit *pSubUnit,
                                      BOOLEAN ignoreCrtTime ) ;

         virtual void clearSubUnits () ;

         const dmsIndexStat *getIndexStat ( dmsExtentID indexLID ) const ;

         const dmsIndexStat *getFieldStat ( const CHAR *pFieldName ) const ;

         OSS_INLINE BOOLEAN addIndexStat ( dmsIndexStat *pIndexStat,
                                           BOOLEAN ignoreCrtTime )
         {
            return addSubUnit( pIndexStat, ignoreCrtTime ) ;
         }

         BOOLEAN removeIndexStat ( dmsExtentID indexLID,
                                   BOOLEAN findNewFieldStat ) ;

         BOOLEAN removeFieldStat ( const CHAR *pFieldName,
                                   BOOLEAN findNewFieldStat ) ;

         OSS_INLINE void renameCS ( const CHAR *pCSName )
         {
            setCSName( pCSName ) ;
         }

         OSS_INLINE void renameCL ( const CHAR *pCLName )
         {
            setCLName( pCLName ) ;
         }

      protected :
         virtual INT32 _initItem ( const BSONObj &boStat ) ;
         virtual INT32 _postInit () { return SDB_OK ; }
         virtual void _toBSON ( BSONObjBuilder &builder ) const ;

         void _addFieldStat ( dmsIndexStat *pIndexStat, BOOLEAN ignoreCrtTime ) ;

         void _removeFieldStat ( dmsIndexStat *pDeletingStat ) ;

         void _findNewFieldStat ( dmsIndexStat *pDeletingStat ) ;

      protected :
         // Name of collection space
         CHAR     _pCSName [ DMS_COLLECTION_SPACE_NAME_SZ + 1 ] ;

         // Name of collection ( short name )
         CHAR     _pCLName [ DMS_COLLECTION_NAME_SZ + 1 ] ;

         UINT32            _totalDataPages ;
         UINT64            _totalDataSize ;
         UINT32            _avgNumFields ;

         INDEX_STAT_MAP    _indexStats ;
         FIELD_STAT_MAP    _fieldStats ;
   } ;

   typedef _dmsCollectionStat dmsCollectionStat ;

}

#endif //DMSSTATUNIT_HPP__
