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

   Source File Name = ixm.hpp

   Descriptive Name = Index Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   Index Record ID (IRID) and Index Control Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXM_HPP_
#define IXM_HPP_

#include "ixm_common.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ixmIndexKey.hpp"
#include "msgDef.h"

using namespace std ;
using namespace bson ;

namespace engine
{
   class _dmsContext ;
   class _dmsStorageIndex ;

   #define IXM_PAGE_SIZE4K  DMS_PAGE_SIZE4K

   #define IXM_KEY_MAX_SIZE            1000
   #define IXM_INVALID_OFFSET          0
   #define IXM_INDEX_NAME_SIZE         1024
   #define IXM_ID_KEY_NAME             "$id"
   #define IXM_SHARD_KEY_NAME          "$shard"
   #define IXM_2D_KEY_TYPE             "2d"
   #define IXM_TEXT_KEY_TYPE           "text"
   #define IXM_POSITIVE_KEY_TYPE       1
   #define IXM_REVERSE_KEY_TYPE        -1

   typedef UINT16 ixmKeySlot ;
   typedef UINT16 ixmRIDSlot ;
   typedef UINT16 ixmOffset ;

   /*
      _ixmRecordID define
   */
   class _ixmRecordID : SDBObject
   {
   public :
      dmsExtentID _extent ;
      UINT16      _slot ;
      _ixmRecordID ()
      {
         _extent = DMS_INVALID_EXTENT ;
         _slot = 0 ;
      }
      _ixmRecordID ( dmsExtentID extent, UINT16 slot )
      {
         _extent = extent ;
         _slot = slot ;
      }
      _ixmRecordID& operator=(const _ixmRecordID &rhs)
      {
         _extent=rhs._extent;
         _slot=rhs._slot;
         return *this;
      }
      BOOLEAN isNull ()
      {
         return DMS_INVALID_EXTENT == _extent ;
      }
      BOOLEAN operator!=(const _ixmRecordID &rhs)
      {
         return !(_extent == rhs._extent &&
                  _slot == rhs._slot) ;
      }
      BOOLEAN operator==(const _ixmRecordID &rhs)
      {
         return ( _extent == rhs._extent &&
                  _slot == rhs._slot) ;
      }
      INT32 compare ( const _ixmRecordID &rhs )
      {
         if (_extent != rhs._extent)
            return _extent - rhs._extent ;
         else
            return (INT32)_slot - (INT32)rhs._slot ;
      }
      void reset()
      {
         _extent = DMS_INVALID_EXTENT ;
         _slot = 0 ;
      }
   } ;
   typedef class _ixmRecordID ixmRecordID ;

   /*
      INDEX CB EXTENT EYE CATCHER DEFINE
   */
   #define IXM_EXTENT_CB_EYECATCHER0      'I'
   #define IXM_EXTENT_CB_EYECATCHER1      'C'
   /*
      INDEX CB EXTENT FLAG DEFINE
   */
   #define IXM_INDEX_FLAG_NORMAL          0
   #define IXM_INDEX_FLAG_CREATING        1
   #define IXM_INDEX_FLAG_DROPPING        2
   #define IXM_INDEX_FLAG_INVALID         3
   #define IXM_INDEX_FLAG_TRUNCATING      4
   /*
      INDEX CB EXTENT TYPE DEFINE
   */
   #define IXM_EXTENT_TYPE_NONE           0x0000
   #define IXM_EXTENT_TYPE_POSITIVE       0x0001
   #define IXM_EXTENT_TYPE_REVERSE        0x0002
   #define IXM_EXTENT_TYPE_2D             0x0004
   #define IXM_EXTENT_TYPE_TEXT           0x0008
   #define IXM_EXTENT_HAS_TYPE(type,dst)  (type&dst)
   /*
      _ixmIndexCBExtent define
   */
   class _ixmIndexCBExtent : public SDBObject
   {
   public :
      CHAR        _eyeCatcher [2] ;
      UINT16      _indexFlag ;
      UINT16      _mbID ;        // 1 to 4096
      CHAR        _flag ;
      CHAR        _version ;
      dmsExtentID _logicID ;
      dmsExtentID _rootExtentID ;
      dmsExtentID _scanExtLID ;  // only when flag is IXM_INDEX_FLAG_CREATING,
      UINT16      _type ;
      CHAR        _reserved[6] ;
   } ;
   typedef class _ixmIndexCBExtent ixmIndexCBExtent ;
   #define IXM_INDEX_CB_EXTENT_METADATA_SIZE (sizeof(ixmIndexCBExtent))

   #define IXM_INDEX_IS_CREATING(flag) (0!=((flag) & IXM_INDEX_FLAG_CREATING))
   #define IXM_INDEX_IS_DROPPING(flag) (0!=((flag) & IXM_INDEX_FLAG_DROPPING))
   #define IXM_INDEX_IS_INVALID(flag)  (0!=((flag) & IXM_INDEX_FLAG_INVALID ))

   /*
      _ixmIndexCB define
   */
   class _ixmIndexCB : public SDBObject
   {
   private:
#pragma pack(1)
      class _IDToInsert : public SDBObject
      {
      public :
         CHAR _type ;
         CHAR _id[4] ; // _id + '\0'
         OID _oid ;
         _IDToInsert ()
         {
            _type = (CHAR)jstOID ;
            _id[0] = '_' ;
            _id[1] = 'i' ;
            _id[2] = 'd' ;
            _id[3] = 0 ;
            SDB_ASSERT ( sizeof ( _IDToInsert) == 17,
                         "IDToInsert should be 17 bytes" ) ;
         }
      } ;
      typedef class _IDToInsert _IDToInsert ;
#pragma pack()

      const ixmIndexCBExtent *_extent ;
      BOOLEAN _isInitialized ;
      BSONObj     _infoObj ;
      _dmsStorageIndex *_pIndexSu ;
      _dmsContext      *_pContext ;
      INT32 _pageSize ;
      UINT8       _version ;
      dmsExtentID _extentID ;

      OSS_INLINE BOOLEAN _verify() const
      {
         if ( NULL == _extent ||
              IXM_EXTENT_CB_EYECATCHER0 != _extent->_eyeCatcher[0] ||
              IXM_EXTENT_CB_EYECATCHER1 != _extent->_eyeCatcher[1] )
         {
            return FALSE ;
         }
         if ( DMS_EXTENT_FLAG_INUSE != _extent->_flag )
         {
            return FALSE ;
         }
         return TRUE ;
      }
      void _init()
      {
         if ( !_verify() )
         {
            PD_LOG ( PDERROR, "ixm detailed extent is not valid" ) ;
            return ;
         }
         _version = _extent->_version ;
         try
         {
            _infoObj = BSONObj ( ((const CHAR*)_extent) +
                                 IXM_INDEX_CB_EXTENT_METADATA_SIZE ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to load info obj for index: %s",
                     e.what() ) ;
            return ;
         }
         _isInitialized = TRUE ;
      }

      static BOOLEAN _isValidKey( const bson::BSONObj &obj )
      {
         if ( obj.isEmpty() )
         {
            return FALSE ;
         }
         else
         {
            BSONObjIterator i( obj ) ;
            while ( i.more() )
            {
               BSONElement e = i.next() ;
               const CHAR *fieldName = e.fieldName() ;
               if ( NULL == fieldName ||
                    '\0' == fieldName[0] ||
                    NULL != ossStrchr( fieldName, '$' ) )
               {
                  return FALSE ;
               }
            }
         }

         return TRUE ;
      }

      friend class _ixmIndexKeyGen ;

   public:
      _ixmIndexCB ( dmsExtentID extentID,
                    _dmsStorageIndex *pIndexSu,
                    _dmsContext *context ) ;
      _ixmIndexCB ( dmsExtentID extentID,
                    const BSONObj &infoObj,
                    UINT16 mbID ,
                    _dmsStorageIndex *pIndexSu,
                    _dmsContext *context ) ;
      ~_ixmIndexCB() ;

      BOOLEAN isInitialized () const
      {
         return _isInitialized ;
      }
      dmsExtentID getExtentID () const
      {
         return _extentID ;
      }
      UINT16 getIndexType() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_type ;
      }
      UINT16 getFlag () const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_indexFlag ;
      }

      void setFlag( UINT16 flag ) ;

      dmsExtentID getLogicalID() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_logicID ;
      }

      void setLogicalID( dmsExtentID logicalID ) ;
      void clearLogicID() ;

      UINT16 getMBID () const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_mbID ;
      }
      dmsExtentID scanExtLID () const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_scanExtLID ;
      }

      void scanExtLID ( UINT32 extLID ) ;

      BSONObj getKeyFromQuery ( const BSONObj & query ) const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         BSONObj k   = keyPattern () ;
         BSONObj res = query.extractFieldsUnDotted ( k ) ;
         return res ;
      }
      /* pull out the relevant key objects from obj, so we
         can index them.  Note that the set is multiple elements
         only when it's a "multikey" array.
         keys will be left empty if key not found in the object.
      */
      INT32 getKeysFromObject ( const BSONObj &obj, BSONObjSet &keys ) const ;

      /* get the key pattern for this object.
         e.g., { lastname:1, firstname:1 }
      */
      BSONObj keyPattern() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         try
         {
            return _infoObj.getObjectField( IXM_KEY_FIELD ) ;
         }
         catch( std::exception &e )
         {
            pdLog ( PDERROR, __FUNC__, __FILE__, __LINE__,
                    "Unable to extract key from index pattern: %s",
                    e.what() ) ;
         }
         return BSONObj() ;
      }

      BSONObj getDef() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _infoObj ;
      }

      /**
       * @return offset into keyPattern for key
                 -1 if doesn't exist
       */
      INT32 keyPatternOffset( const CHAR *key ) const;

      BOOLEAN inKeyPattern( const CHAR *key ) const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return keyPatternOffset( key ) >= 0 ;
      }
      OSS_INLINE const CHAR *getName() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _infoObj.getStringField( IXM_NAME_FIELD ) ;
      }
      static BOOLEAN isSysIndexPattern ( const BSONObj &pattern )
      {
         BSONObjIterator i(pattern);
         BSONElement e = i.next();
         if( ossStrcmp(e.fieldName(), DMS_ID_KEY_NAME) != 0 )
            return FALSE ;
         return i.next().eoo();
      }
      BOOLEAN isSysIndex() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return isSysIndexPattern( keyPattern() );
      }
      static INT32 versionForIndexObj( const BSONObj &obj )
      {
         BSONElement e = obj[ IXM_V_FIELD ];
         if( e.type() == NumberInt )
         {
            return e._numberInt();
         }
         return 0 ;
      }
      INT32 version() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return versionForIndexObj( _infoObj );
      }

      static BOOLEAN generateIndexType( const BSONObj &obj, UINT16 &type )
      {
         BOOLEAN rc = TRUE ;
         try
         {
            BSONObj keyPattern = obj.getObjectField( IXM_KEY_FIELD ) ;
            if ( keyPattern.isEmpty() )
            {
               goto error ;
            }
            {
            BOOLEAN hasGeo = FALSE ;
            BOOLEAN hasOther = FALSE ;
            BSONObjIterator i( keyPattern ) ;
            while ( i.more() )
            {
               BSONElement ele = i.next() ;
               if ( ele.eoo() )
               {
                  goto error ;
               }
               if ( ele.isNumber() )
               {
                  if ( IXM_POSITIVE_KEY_TYPE == ele.Number() )
                  {
                     type |= IXM_EXTENT_TYPE_POSITIVE ;
                  }
                  else if ( IXM_REVERSE_KEY_TYPE == ele.Number() )
                  {
                     type |= IXM_EXTENT_TYPE_REVERSE ;
                  }
                  else
                  {
                     goto error ;
                  }
                  hasOther = TRUE ;
               }
               else if ( String == ele.type() )
               {
                  if ( IXM_2D_KEY_TYPE == ele.String() )
                  {
                     if ( hasGeo || hasOther )
                     {
                        goto error ;
                     }
                     type |= IXM_EXTENT_TYPE_2D ;
                     hasGeo = TRUE ;
                  }
                  else if ( IXM_TEXT_KEY_TYPE == ele.String() )
                  {
                     type |= IXM_EXTENT_TYPE_TEXT ;
                  }
                  else
                  {
                     goto error ;
                  }
               }
               else
               {
                  goto error ;
               }
            }
            }
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to generate type from index: %s. "
                     "index:%s", e.what(), obj.toString().c_str() ) ;
            goto error ;
         }

         if ( ( IXM_EXTENT_TYPE_TEXT & type ) &&
              ( ~IXM_EXTENT_TYPE_TEXT & type ) )
         {
            PD_LOG( PDERROR, "Text index can not mix with other kinds of "
                    "indices:%s", obj.toString().c_str() ) ;
            goto error ;
         }

      done:
         return rc ;
      error:
         rc = FALSE ;
         goto done ;
      }

      static BOOLEAN validateKey ( const BSONObj &obj, BOOLEAN isSys = FALSE )
      {
         INT32 fieldCount = 0 ;
         BOOLEAN isUniq = FALSE ;
         BOOLEAN enforced = FALSE ;
         if ( obj.objsize() + sizeof(_IDToInsert) +
              IXM_INDEX_CB_EXTENT_METADATA_SIZE >= IXM_PAGE_SIZE4K )
         {
            return FALSE ;
         }
         UINT16 type = 0 ;
         if ( !generateIndexType( obj, type ) )
         {
            return FALSE ;
         }
         fieldCount ++ ;

         if ( !_isValidKey( obj.getObjectField( IXM_KEY_FIELD ) ) )
         {
            PD_LOG( PDERROR, "index key is invalid:%s",
                    obj.toString( FALSE, TRUE ).c_str() ) ;

            return FALSE ;
         }

         if ( ossStrlen ( obj.getStringField( IXM_NAME_FIELD )) == 0 )
         {
            return FALSE ;
         }
         fieldCount ++ ;
         if ( SDB_OK != dmsCheckIndexName ( obj.getStringField(IXM_NAME_FIELD),
                                            isSys ) )
         {
            return FALSE ;
         }
         if ( ossStrlen ( obj.getStringField(IXM_NAME_FIELD) )
              >= IXM_INDEX_NAME_SIZE )
         {
            return FALSE ;
         }
         if ( obj.hasField ( IXM_V_FIELD ) )
         {
            fieldCount ++ ;
         }
         if ( obj.hasField ( IXM_UNIQUE_FIELD ))
         {
            BSONElement e = obj.getField( IXM_UNIQUE_FIELD ) ;
            isUniq = e.booleanSafe() ;
            fieldCount ++ ;
         }
         if ( obj.hasField ( IXM_ENFORCED_FIELD ))
         {
            BSONElement e = obj.getField( IXM_ENFORCED_FIELD ) ;
            enforced = e.booleanSafe() ;
            fieldCount ++ ;
         }
         if ( obj.hasField ( IXM_DROPDUP_FIELD ))
         {
            fieldCount ++ ;
         }
         if ( obj.hasField (DMS_ID_KEY_NAME))
         {
            fieldCount ++ ;
         }
         if ( fieldCount != obj.nFields() )
         {
            return FALSE ;
         }

         if ( !isUniq && enforced )
         {
            PD_LOG( PDERROR, "should not specify \"enforced\" as true in an"
                    " non-unique index" ) ;
            return FALSE ;
         }
         return TRUE ;
      }

      BOOLEAN unique() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _infoObj[ IXM_UNIQUE_FIELD ].trueValue() ;
      }

      BOOLEAN enforced() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _infoObj[ IXM_ENFORCED_FIELD ].trueValue() ;
      }

      /** return true if dropDups was set when building index (if any
        * duplicates, dropdups drops the duplicating objects) */
      BOOLEAN dropDups() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _infoObj.getBoolField( IXM_DROPDUP_FIELD );
      }
      std::string toString() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _infoObj.toString() ;
      }

      INT32 allocExtent ( dmsExtentID &extentID ) ;
      INT32 freeExtent ( dmsExtentID extentID ) ;

      void setRoot ( dmsExtentID rootExtentID ) ;

      dmsExtentID getRoot() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_rootExtentID ;
      }
      _dmsStorageIndex *getSU ()
      {
         return _pIndexSu ;
      }
      _dmsContext* getContext ()
      {
         return _pContext ;
      }
      INT32 getIndexID ( OID &oid ) const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         try
         {
            _infoObj.getField(DMS_ID_KEY_NAME).Val(oid) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG ( PDERROR, "Failed to get index id: %s", e.what() ) ;
            return SDB_SYS ;
         }
         return SDB_OK ;
      }
      BOOLEAN isStillValid ( OID &oid ) const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if ( !_extent )
         {
            return FALSE ;
         }
         if ( _extent->_eyeCatcher[0] != IXM_EXTENT_CB_EYECATCHER0 ||
              _extent->_eyeCatcher[1] != IXM_EXTENT_CB_EYECATCHER1 )
         {
            return FALSE ;
         }
         if ( _extent->_indexFlag != IXM_INDEX_FLAG_NORMAL &&
              _extent->_indexFlag != IXM_INDEX_FLAG_CREATING )
         {
            return FALSE ;
         }
         OID curOID ;
         if ( SDB_OK != getIndexID ( curOID ) )
         {
            return FALSE ;
         }
         return curOID == oid ;
      }

      INT32 truncate ( BOOLEAN removeRoot ) ;

      BOOLEAN isSameDef( const BSONObj &defObj,
                         BOOLEAN strict = FALSE ) const ;

   } ;
   typedef class _ixmIndexCB ixmIndexCB ;

}

#endif /* IXM_HPP_ */

