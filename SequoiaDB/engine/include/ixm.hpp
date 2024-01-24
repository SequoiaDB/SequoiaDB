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
#include "utilPooledObject.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ixmIndexKey.hpp"
#include "msgDef.h"
#include "../bson/ordering.h"
#include "sdbInterface.hpp"
#include "utilResult.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{
   class _dmsContext ;
   class _dmsStorageIndex ;

   #define IXM_PAGE_SIZE4K  DMS_PAGE_SIZE4K

   #define IXM_KEY_SIZE_LIMIT          4096
   #define IXM_KEY_NODE_NUM_MIN        3
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
      // <0 if current object sit before argment rid
      // =0 means extent/offset are the same
      // >0 means current obj sit after argment rid
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
   #define IXM_EXTENT_TYPE_GLOBAL         0x0010
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
                                 // the _scanExtLID is valid
      UINT16      _type ;
      CHAR        _reserved[2] ;
      dmsOffset   _scanExtOffset ;
   } ;
   typedef class _ixmIndexCBExtent ixmIndexCBExtent ;
   #define IXM_INDEX_CB_EXTENT_METADATA_SIZE (sizeof(ixmIndexCBExtent))

   #define IXM_INDEX_IS_CREATING(flag) (0!=((flag) & IXM_INDEX_FLAG_CREATING))
   #define IXM_INDEX_IS_DROPPING(flag) (0!=((flag) & IXM_INDEX_FLAG_DROPPING))
   #define IXM_INDEX_IS_INVALID(flag)  (0!=((flag) & IXM_INDEX_FLAG_INVALID ))

   /*
      Get index flag description
   */
   const CHAR* ixmGetIndexFlagDesp( UINT16 indexFlag ) ;

   /*
      _ixmIndexCB define
   */
   class _ixmIndexCB : public utilPooledObject
   {
      #define NAME_IS_INITED()               ( _fieldInitedFlag &  0x00000001 )
      #define SET_NAME_INITED()              ( _fieldInitedFlag |= 0x00000001 )
      #define KEY_IS_INITED()                ( _fieldInitedFlag &  0x00000002 )
      #define SET_KEY_INITED()               ( _fieldInitedFlag |= 0x00000002 )
      #define V_IS_INITED()                  ( _fieldInitedFlag &  0x00000004 )
      #define SET_V_INITED()                 ( _fieldInitedFlag |= 0x00000004 )
      #define UNIQUE_IS_INITED()             ( _fieldInitedFlag &  0x00000008 )
      #define SET_UNIQUE_INITED()            ( _fieldInitedFlag |= 0x00000008 )
      #define UNIQUE1_IS_INITED()            ( _fieldInitedFlag &  0x00000010 )
      #define SET_UNIQUE1_INITED()           ( _fieldInitedFlag |= 0x00000010 )
      #define ENFORCED_IS_INITED()           ( _fieldInitedFlag &  0x00000020 )
      #define SET_ENFORCED_INITED()          ( _fieldInitedFlag |= 0x00000020 )
      #define ENFORCED1_IS_INITED()          ( _fieldInitedFlag &  0x00000040 )
      #define SET_ENFORCED1_INITED()         ( _fieldInitedFlag |= 0x00000040 )
      #define NOTNULL_IS_INITED()            ( _fieldInitedFlag &  0x00000080 )
      #define SET_NOTNULL_INITED()           ( _fieldInitedFlag |= 0x00000080 )
      #define DROPDUPS_IS_INITED()           ( _fieldInitedFlag &  0x00000400 )
      #define SET_DROPDUPS_INITED()          ( _fieldInitedFlag |= 0x00000400 )
      #define NOTARRAY_IS_INITED()           ( _fieldInitedFlag &  0x00000800 )
      #define SET_NOTARRY_INITED()           ( _fieldInitedFlag |= 0x00000800 )
      #define DMS_ID_KEY_NAME_IS_INITED()    ( _fieldInitedFlag &  0x00001000 )
      #define SET_DMS_ID_KEY_NAME_INITED()   ( _fieldInitedFlag |= 0x00001000 )
      #define NAME_EXT_DATA_IS_INITED()      ( _fieldInitedFlag &  0x00002000 )
      #define SET_NAME_EXT_DATA_INITED()     ( _fieldInitedFlag |= 0x00002000 )
      #define ID_INDEX_IS_INITED()           ( _fieldInitedFlag &  0x00004000 )
      #define SET_ID_INDEX_INITED()          ( _fieldInitedFlag |= 0x00004000 )
      #define GLOB_INDEX_IS_INITED()         ( _fieldInitedFlag &  0x00008000 )
      #define SET_GLOB_INDEX_INITED()        ( _fieldInitedFlag |= 0x00008000 )
      #define UNIQUEID_IS_INITED()           ( _fieldInitedFlag &  0x00010000 )
      #define SET_UNIQUEID_INITED()          ( _fieldInitedFlag |= 0x00010000 )

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
            SDB_ASSERT ( sizeof ( _IDToInsert ) == 17,
                         "IDToInsert should be 17 bytes" ) ;
         }
      } ;
      typedef class _IDToInsert IDToInsert ;
#pragma pack()

      // raw data for control block extent
      const ixmIndexCBExtent *_extent ;
      // whether if this CB is initialized
      BOOLEAN _isInitialized ;
      // object for index def
      BSONObj     _infoObj ;
      // associated storage unit pointer
      _dmsStorageIndex *_pIndexSu ;
      _dmsContext      *_pContext ;
      // page size
      INT32 _pageSize ;
      // current version
      UINT8       _version ;
      // extent ID for the control block extent
      dmsExtentID _extentID ;

      mutable BOOLEAN _isGlobalIndex ;
      mutable utilCLUniqueID _indexCLUID ;
      mutable const CHAR * _indexCLName ;

      mutable utilIdxUniqueID _idxUniqID ;

      mutable UINT8 _indexObjVersion ;
      mutable BSONObj _keyPattern ;
      mutable const CHAR* _name ;
      mutable BOOLEAN _unique ;
      mutable BOOLEAN _enforced ;
      mutable BOOLEAN _notNull ;
      mutable BOOLEAN _notArray ;
      mutable BOOLEAN _dropDups ;
      mutable BOOLEAN _isIDIndex ;
      mutable OID _oid ;
      mutable const CHAR* _nameExtData ;
      mutable UINT32 _fieldInitedFlag ;

      // Whether the given extent is a valid control block
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
      // load metadata from extent
      void _init()
      {
         if ( !_verify() )
         {
            PD_LOG ( PDERROR, "ixm detailed extent is not valid" ) ;
            return ;
         }
         _version = _extent->_version ;
         // create index def from page
         try
         {
            _infoObj = BSONObj( ((const CHAR*)_extent) +
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

      // we want index key generator able to directly access control block
      // private data
      friend class _ixmIndexKeyGen ;

   public:
      // Before using ixmIndexCB, after create the object user must check
      // isInitialized ()
      // create index details from existing extent
      _ixmIndexCB ( dmsExtentID extentID,
                    _dmsStorageIndex *pIndexSu,
                    _dmsContext *context ) ;
      // create index details into a newly allocated extent
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

      dmsRecordID getScanRID() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return dmsRecordID( _extent->_scanExtLID,
                             _extent->_scanExtOffset ) ;
      }

      void setScanRID( const dmsRecordID &rid ) ;

      dmsExtentID scanExtLID () const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_scanExtLID ;
      }

      void scanExtLID ( UINT32 extLID ) ;

      dmsOffset getScanExtOffset () const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return _extent->_scanExtOffset ;
      }

      void setScanExtOffset ( UINT32 offset ) ;

      // remove all field name from bson object
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
      INT32 getKeysFromObject ( const BSONObj &obj,
                                BSONObjSet &keys,
                                BOOLEAN *pAllUndefined = NULL,
                                BOOLEAN checkValid = FALSE,
                                utilWriteResult *pResult = NULL ) const ;

      INT32 checkKeys( const BSONObj &obj,
                       const BSONObjSet &keys,
                       const BSONElement &arrEle,
                       utilWriteResult *pResult = NULL ) const ;

      /* get the key pattern for this object.
         e.g., { lastname:1, firstname:1 }
      */
      BSONObj keyPattern() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !KEY_IS_INITED() )
         {
            try
            {
               _keyPattern = _infoObj.getObjectField( IXM_KEY_FIELD ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract key from index pattern: %s",
                       e.what() ) ;
               _keyPattern = BSONObj() ;
            }
            SET_KEY_INITED() ;
         }
         return _keyPattern ;
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

      // is the given field exist in the index?
      BOOLEAN inKeyPattern( const CHAR *key ) const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return keyPatternOffset( key ) >= 0 ;
      }
      // return the name of index
      OSS_INLINE const CHAR *getName() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !NAME_IS_INITED() )
         {
            try
            {
               _name = _infoObj.getStringField( IXM_NAME_FIELD ) ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract name from index pattern: %s",
                       e.what() ) ;
            }
            SET_NAME_INITED() ;
         }
         return _name ;
      }

      BOOLEAN isIDIndex() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !ID_INDEX_IS_INITED() )
         {
            try
            {
               if( 0 == ossStrcmp( getName(), IXM_ID_KEY_NAME ) )
               {
                  _isIDIndex = TRUE ;
               }
               else
               {
                  _isIDIndex = FALSE ;
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract is ID_index from index pattern: %s",
                       e.what() ) ;
            }
            SET_ID_INDEX_INITED() ;
         }
         return _isIDIndex ;
      }
      // get the version of index, 0 by default
      INT32 version() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !V_IS_INITED() )
         {
            try
            {
               BSONElement e = _infoObj[ IXM_V_FIELD ] ;
               if( e.type() == NumberInt )
               {
                  _indexObjVersion = e._numberInt();
               }
               else
               {
                  _indexObjVersion = 0;
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract version from index pattern: %s",
                       e.what() ) ;
            }
            SET_V_INITED() ;
         }
         return _indexObjVersion ;
      }

      static INT32 generateIndexType( const BSONObj &obj, UINT16 &type )
      {
         INT32 rc = SDB_OK ;

         try
         {
            BSONElement ele = obj.getField( IXM_GLOBAL_FIELD ) ;
            if ( EOO != ele.type() )
            {
               if ( Bool != ele.type() )
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG( PDERROR, "Invalid [%s] field type[%d], rc: %d",
                          IXM_GLOBAL_FIELD, ele.type(), rc ) ;
                  goto error ;
               }
               if ( ele.boolean() )
               {
                  type |= IXM_EXTENT_TYPE_GLOBAL ;
               }
            }

            ele = obj.getField( IXM_KEY_FIELD ) ;
            if ( ele.type() != Object )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid index key type[%d], rc: %d",
                       ele.type(), rc ) ;
               goto error ;
            }
            rc = generateIndexTypeByKey( ele.embeddedObject(), type ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDERROR, "Occur exception: %s", e.what() ) ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      /// generate index type from input bsonobj.
      static INT32 generateIndexTypeByKey( const BSONObj &keyObj, UINT16 &type )
      {
         INT32 rc = SDB_OK ;

         try
         {
            if ( keyObj.isEmpty() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "IndexDef can't be empty: %s",
                           keyObj.toString().c_str() ) ;
               goto error ;
            }
            {
            BOOLEAN hasGeo = FALSE ;
            BOOLEAN hasOther = FALSE ;
            BSONObjIterator i( keyObj ) ;
            while ( i.more() )
            {
               BSONElement ele = i.next() ;
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
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR,
                                 "Index key value should be 1/-1 or 'text': %s",
                                 keyObj.toString().c_str() ) ;
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
                        /// one index can only have one geo field.
                        /// 2d index must be the first field.
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
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR,
                                 "Index key value should be 1/-1 or 'text': %s",
                                 keyObj.toString().c_str() ) ;
                     goto error ;
                  }
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR,
                              "Index key value should be 1/-1 or 'text': %s",
                              keyObj.toString().c_str() ) ;
                  goto error ;
               }
            }
            }
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }

         if ( ( IXM_EXTENT_TYPE_TEXT & type ) &&
              ( ~IXM_EXTENT_TYPE_TEXT & type ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Text index can not mix with other kinds of "
                        "indices: %s", keyObj.toString().c_str() ) ;
            goto error ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      static INT32 checkIndexKey( const bson::BSONObj &obj )
      {
         INT32 rc = SDB_OK ;

         if ( obj.isEmpty() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Index key can't be empty: %s",
                        obj.toString().c_str() ) ;
            goto error ;
         }

         {
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next() ;
            const CHAR *fieldName = e.fieldName() ;
            if ( NULL == fieldName || '\0' == fieldName[0] )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR,
                           "The field name in index key can't be empty: %s",
                           obj.toString().c_str() ) ;
               goto error ;
            }
            else if ( NULL != ossStrchr( fieldName, '$' ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Field name[%s] in index key can't contain "
                           "special symbol '$': %s", fieldName,
                           obj.toString().c_str() ) ;
               goto error ;
            }
         }
         }

         try
         {
            // index key obj shouldn't has more than 32 field
            Ordering::make ( obj ) ;
         }
         catch( ... )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "The number of fields in composite index"
                        " can't exceed 32: %s", obj.toString().c_str() ) ;
            goto error ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      // check whether a given object is valid, usually this is called before
      // creating index
      static INT32 checkIndexDef ( const BSONObj &obj, BOOLEAN isSys = FALSE )
      {
         INT32 rc = SDB_OK ;
         INT32 fieldCount = 0 ;
         BOOLEAN isUniq = FALSE ;
         BOOLEAN enforced = FALSE ;
         UINT16 type = 0 ;
         BSONObj indexKey ;
         const CHAR* indexName = NULL ;
         BSONElement ele ;

         // make sure the object contains key and name field, and may include
         // "v", "dropDups", "unique" fields, and not include any other fields

         // '_id' field
         if ( obj.hasField( DMS_ID_KEY_NAME ) )
         {
            fieldCount ++ ;
         }
         else
         {
            // make sure the index def is not too large
            INT32 defSize = obj.objsize() + sizeof(_IDToInsert) ;
            if ( defSize + IXM_INDEX_CB_EXTENT_METADATA_SIZE >= IXM_PAGE_SIZE4K )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "The size of index definition[%d] is "
                           "too large", defSize ) ;
               goto error ;
            }
         }

         // 'UniqueID' field
         if ( obj.hasField( FIELD_NAME_UNIQUEID ) )
         {
            fieldCount ++ ;
         }

         // 'key' field
         ele = obj.getField( IXM_KEY_FIELD ) ;
         if ( ele.type() != Object )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid index key type[%d] in obj[%s], rc: %d",
                    ele.type(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
         indexKey = ele.embeddedObject() ;
         rc = generateIndexTypeByKey( indexKey, type ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get index type by key[%s], rc: %d",
                    indexKey.toString().c_str(), rc ) ;
            goto error ;
         }
         rc = checkIndexKey( indexKey ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to check index key[%s], rc: %d",
                    indexKey.toString().c_str(), rc ) ;
            goto error ;
         }
         fieldCount ++ ;

         // 'name' field
         ele = obj.getField( IXM_NAME_FIELD ) ;
         if ( ele.type() != String )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid index name type[%d] in obj[%s], rc: %d",
                    ele.type(), obj.toString().c_str(), rc ) ;
            goto error ;
         }
         indexName = ele.valuestr() ;
         rc = dmsCheckIndexName( indexName, isSys ) ;
         if ( rc )
         {
            goto error ;
         }
         fieldCount ++ ;

         // other filed
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
         if ( obj.hasField ( IXM_NOTNULL_FIELD ) )
         {
            fieldCount ++ ;
         }
         if ( obj.hasField ( IXM_NOTARRAY_FIELD ))
         {
            fieldCount ++ ;
         }
         if ( obj.hasField ( IXM_DROPDUP_FIELD ) )
         {
            fieldCount ++ ;
         }
         if ( obj.hasField( IXM_GLOBAL_FIELD ) )
         {
            fieldCount++ ;
         }
         if ( obj.hasField( IXM_GLOBAL_OPTION_FIELD ) )
         {
            BSONElement ele ;
            BSONObj globalOptions ;

            ele = obj.getField( IXM_GLOBAL_OPTION_FIELD ) ;
            if ( Object != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR,
                           "The type of field[%s] in index definition is "
                           "invalid: %d", IXM_GLOBAL_OPTION_FIELD, ele.type() ) ;
               goto error ;
            }

            globalOptions = ele.embeddedObject() ;

            ele = globalOptions.getField( FIELD_NAME_CL_UNIQUEID ) ;
            if ( NumberLong != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR,
                           "The type of field[%s] in index definition is "
                           "invalid: %d", FIELD_NAME_CL_UNIQUEID, ele.type() ) ;
               goto error ;
            }

            fieldCount++ ;
         }
         if ( obj.hasField( IXM_STANDALONE_FIELD ) )
         {
            fieldCount++ ;
         }

         if ( obj.hasField( IXM_CREATETIME_FIELD ) )
         {
            fieldCount++ ;
         }
         if ( obj.hasField( IXM_REBUILDTIME_FIELD ) )
         {
            fieldCount++ ;
         }

         // make sure no other fields, unless it is a geo index.
         if ( fieldCount != obj.nFields() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR,
                        "Index definition has unrecognized field" ) ;
            goto error ;
         }

         if ( !isUniq && enforced )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Can't specify \"enforced\" as true in an"
                        " non-unique index" ) ;
            goto error ;
         }

      done:
         return rc ;
      error:
         goto done ;
      }

      // get the uniqueness
      BOOLEAN unique() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !UNIQUE_IS_INITED() )
         {
            try
            {
               _unique = _infoObj.getField( IXM_UNIQUE_FIELD ).trueValue() ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract unique from index pattern: %s",
                       e.what() ) ;
            }
            SET_UNIQUE_INITED() ;
         }
         return _unique ;
      }

      BOOLEAN isGlobal() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;

         if ( !GLOB_INDEX_IS_INITED() )
         {
            _initGlobIndexInfo() ;
         }

         return _isGlobalIndex ;
      }

      utilCLUniqueID getIndexCLUID() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;

         if ( !GLOB_INDEX_IS_INITED() )
         {
            _initGlobIndexInfo() ;
         }

         return _indexCLUID ;
      }

      const CHAR* getIndexCLName() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;

         if ( !GLOB_INDEX_IS_INITED() )
         {
            _initGlobIndexInfo() ;
         }

         return _indexCLName ;
      }

      utilIdxUniqueID getUniqueID() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !UNIQUEID_IS_INITED() )
         {
            try
            {
               BSONElement ele = _infoObj.getField( IXM_UNIQUEID_FIELD ) ;
               if ( ele.isNumber() )
               {
                  _idxUniqID = (utilIdxUniqueID)ele.numberLong() ;
                  SET_UNIQUEID_INITED() ;
               }
               else if ( !ele.eoo() )
               {
                  PD_LOG( PDERROR,
                          "Invalid field[%s] type[%d] from index obj[%s]",
                          IXM_UNIQUEID_FIELD, ele.type(),
                          _infoObj.toString().c_str() ) ;
               }
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract index unqiue id from "
                       "index pattern: %s", e.what() ) ;
            }
         }
         return _idxUniqID ;
      }

      INT32 changeUniqueID( utilIdxUniqueID uniqueID ) ;

      // get enforcement
      BOOLEAN enforced() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !ENFORCED_IS_INITED() )
         {
            try
            {
               _enforced = _infoObj[ IXM_ENFORCED_FIELD ].trueValue() ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract enforced from index pattern: %s",
                       e.what() ) ;
            }
            SET_ENFORCED_INITED() ;
         }
         return _enforced ;
      }

      BOOLEAN notNull() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !NOTNULL_IS_INITED() )
         {
            try
            {
               _notNull = _infoObj[ IXM_NOTNULL_FIELD ].trueValue() ;
            }
            catch( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract notNull from index pattern: %s",
                       e.what() ) ;
            }
            SET_NOTNULL_INITED() ;
         }
         return _notNull ;
      }

      BOOLEAN notArray() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !NOTARRAY_IS_INITED() )
         {
            try
            {
               if( 0 == ossStrcmp( getName(), IXM_ID_KEY_NAME ) )
               {
                  _notArray = TRUE ;
               }
               else
               {
                  _notArray = _infoObj.getBoolField( IXM_NOTARRAY_FIELD ) ;
               }
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract notArray from index pattern: %s",
                       e.what() ) ;
            }
            SET_NOTARRY_INITED() ;
         }
         return _notArray ;
      }

      BOOLEAN standalone() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         return ( getUniqueID() != UTIL_UNIQUEID_NULL &&
                  utilIsStandaloneIdx( getUniqueID() ) ) ;
      }

      /** return true if dropDups was set when building index (if any
        * duplicates, dropdups drops the duplicating objects) */
      BOOLEAN dropDups() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !DROPDUPS_IS_INITED() )
         {
            try
            {
               _dropDups = _infoObj.getBoolField( IXM_DROPDUP_FIELD ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract dropDups from index pattern: %s",
                       e.what() ) ;
            }
            SET_DROPDUPS_INITED() ;
         }
         return _dropDups ;
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

         if( !DMS_ID_KEY_NAME_IS_INITED() )
         {
            try
            {
               _infoObj.getField( DMS_ID_KEY_NAME ).Val( _oid ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG ( PDERROR, "Failed to get index id: %s", e.what() ) ;
               return SDB_SYS ;
            }
            SET_DMS_ID_KEY_NAME_INITED() ;
         }
         oid = _oid ;
         return SDB_OK ;
      }
      BOOLEAN isStillValid ( const OID &oid ) const
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

      INT32 truncate ( BOOLEAN removeRoot, UINT16 indexFlag,
                       ossAtomic64* pDelKeyCnt = NULL ) ;

      BOOLEAN isSameDef( const BSONObj &defObj,
                         BOOLEAN strict = FALSE ) const ;

      // Extend the index definition. Only append new element.
      INT32 extendDef( const BSONElement &ele ) ;

      // INT32 appendDef() ;
      const CHAR *getExtDataName() const
      {
         SDB_ASSERT ( _isInitialized,
                      "index details must be initialized first" ) ;
         if( !NAME_EXT_DATA_IS_INITED() )
         {
            try
            {
               _nameExtData = _infoObj.getStringField( FIELD_NAME_EXT_DATA_NAME ) ;
            }
            catch ( std::exception &e )
            {
               PD_LOG( PDERROR, "Unable to extract extra data name from index pattern: %s",
                       e.what() ) ;
            }
            SET_NAME_EXT_DATA_INITED() ;
         }
         return _nameExtData ;
      }

   protected:
      INT32 _initGlobIndexInfo() const ;

      INT32 _checkNullKeys( const BSONObjSet &keys,
                            utilWriteResult *pResult ) const ;
      INT32 _checkArrayKeys( const BSONObj &obj,
                             const BSONObjSet &keys,
                             const BSONElement &arrEle,
                             utilWriteResult *pResult ) const ;
   } ;
   typedef class _ixmIndexCB ixmIndexCB ;

   bson::BSONObj ixmGetIDIndexDefine () ;
   bson::BSONObj ixmGetIDIndexDefine( UINT64 idxUniqueID ) ;
}

#endif /* IXM_HPP_ */

