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

   Source File Name = ixmIndexKey.hpp

   Descriptive Name = Index Management Index Key Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index key generation from a given index definition and a data record.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMINDEXKEY_HPP_
#define IXMINDEXKEY_HPP_

#include "core.hpp"
#include "utilPooledObject.hpp"
#include "../bson/bson.h"
#include "pd.hpp"
#include "ossMemPool.hpp"
#include "utilArray.hpp"
#include "utilBitmap.hpp"

using namespace bson;

namespace engine
{

   // pre-definition
   class _ixmIndexCB ;
   class _ixmIndexKeyGen ;
   class _ixmKeyGenBase ;

   class _ixmIndexFieldCmp
   {
   public:
      bool operator()(const CHAR* l, const CHAR* r )
      {
         return ossStrcmp( l, r ) < 0 ;
      }
   } ;

   typedef ossPoolSet< const CHAR*, _ixmIndexFieldCmp >        IXM_FIELD_NAME_SET ;
   typedef ossPoolMap< const CHAR*, INT32, _ixmIndexFieldCmp > IXM_INDEX_FIELD_MAP ;
   typedef _utilArray< const CHAR* >                           IXM_ELE_RAWDATA_ARRAY ;
   /*
      IXM get undefined key object
    */
   BSONObj ixmGetUndefineKeyObj( INT32 fieldNum ) ;


   typedef class _ixmIndexNode ixmIndexNode ;
   typedef _utilArray< ixmIndexNode* > IXM_INDEX_NODE_PTR_ARRAY ;

   /*
      _ixmIndexNode define
    */
   class _ixmIndexNode : public utilPooledObject
   {
   public:
      #define DOT_NAME_LEAST_LEVEL 2
      _ixmIndexNode()
      : _name( "" ),
        _fieldIndex( 0 ),
        _level( 0 ),
        _reserveSize( 0 )
      {
      }

      _ixmIndexNode( const StringData &name, UINT32 level, UINT32 size )
      : _name( name ),
        _fieldIndex( 0 ),
        _level( level ),
        _reserveSize( size )
      {
      }

      ~_ixmIndexNode()
      {
         reset() ;
      }

      void reset()
      {
         for( UINT32 i = 0; i < _children.size(); i++ )
         {
            SDB_ASSERT( NULL != _children[i], "ixmIndexNode _children is NULL" ) ;
            SDB_OSS_DEL ( _children[i] ) ;
         }
         _children.clear( TRUE ) ;
      }

      BOOLEAN isLeaf() const
      {
         return _children.empty() ;
      }

      const StringData &getName() const
      {
         return _name ;
      }

      UINT32 getFieldIndex() const
      {
         return _fieldIndex ;
      }

      UINT32 getLevel() const
      {
         return _level ;
      }

      BOOLEAN isEmbedded() const
      {
         return ( _level >= DOT_NAME_LEAST_LEVEL ? TRUE : FALSE ) ;
      }

      void setFieldIndex( INT32 fieldIndex )
      {
         _fieldIndex = fieldIndex ;
      }

      UINT32 getExtraSize() const
      {
         UINT32 extraSize = 0 ;
         // children extraSize
         for( UINT32 i = 0; i < _children.size(); i++ )
         {
            extraSize += _children[i]->getExtraSize() ;
         }

         // root
         if( 0 == _level )
         {
         }
         else if( isLeaf() )
         {
            extraSize += _name.size() ;
         }
         else
         {
            /*
               BSONObj struct:
               |length(UINT32)   |BSONElements(...)   |EOO(CHAR)  |

               BSONElement struct:
               |type(CHAR) |fieldName(CHAR*) |value(...) |
            */
            // length:4 type:1 fieldName:_name.size()+1 value:not extra EOO:1
            extraSize += 4 + 1 + _name.size() + 1 + 1 ;
         }

         return extraSize ;
      }

      UINT32 childrenSize() const
      {
         return _children.size() ;
      }

      UINT32 childrenCapacity() const
      {
         return _children.capacity() ;
      }
      // reserve capacity for children
      INT32 childrenReserve( INT32 size )
      {
         return _children.reserve( size ) ;
      }

      INT32 appendChild( _ixmIndexNode *child )
      {
         INT32 rc = SDB_OK ;
         if( 0 != _reserveSize )
         {
            rc = _children.reserve( _reserveSize ) ;
            if( SDB_OK !=  rc )
            {
               return rc ;
            }
            _reserveSize = 0 ;
         }
         return _children.append( child ) ;
      }

      IXM_INDEX_NODE_PTR_ARRAY &getChildren()
      {
         return _children ;
      }

      _ixmIndexNode *getLastChild()
      {
         if ( !_children.empty() )
         {
            return _children[_children.size() - 1] ;
         }
         return NULL;
      }
   private:
      StringData _name ;
      UINT32 _fieldIndex ;
      UINT32 _level ;
      IXM_INDEX_NODE_PTR_ARRAY _children ;
      UINT32 _reserveSize ;
   } ;

   /*
      _ixmIndexCover define
    */
   // calculate indexCover of the cover arg in param
   class _ixmIndexCover : public utilPooledObject
   {
   public:
      _ixmIndexCover( const BSONObj &keyPattern ) ;
      ~_ixmIndexCover() ;

      BOOLEAN cover( const CHAR* fieldName ) ;
      BOOLEAN cover( const IXM_FIELD_NAME_SET &fieldSet ) ;

      UINT32                 getNfields() ;
      INT32                  getExtraSize( UINT32 &size ) ;
      INT32                  getTree( ixmIndexNode *&tree ) ;
      INT32                  ensureBuff( UINT32 size, CHAR *&pBuff ) ;
      const CHAR*            getBuf() const ;
      IXM_ELE_RAWDATA_ARRAY& getContainer() ;
      INT32                  reInitContainer() ;

   private:
      INT32 _fieldNameToNodes( const CHAR* fieldName,
                               ixmIndexNode *&node ) ;
      INT32 _initTree() ;
      INT32 _initKeyFieldMap() ;

   private:
      BSONObj                 _keyPattern ;
      BOOLEAN                 _treeInited ;
      BOOLEAN                 _keyFieldMapInited ;
      ixmIndexNode            _root ;
      IXM_INDEX_FIELD_MAP     _keyFieldMap ;
      UINT32                  _nfields ;
      UINT32                  _bufSize ;
      CHAR*                   _bufPtr ;
      IXM_ELE_RAWDATA_ARRAY   _container ;
      UINT32                  _extraSize ;
   } ;
   typedef class _ixmIndexCover ixmIndexCover ;

   typedef class _ixmKeyField ixmKeyField ;

   /*
      _ixmKeyField define
    */
   // parsed key field in key generator
   class _ixmKeyField : public utilPooledObject
   {
   public:
      _ixmKeyField()
      : _order( 0 ),
        _nameLen( 0 ),
        _name( NULL )
      {
      }

      _ixmKeyField( const _ixmKeyField &field )
      : _order( field._order ),
        _nameLen( field._nameLen ),
        _name( field._name )
      {
      }

      ~_ixmKeyField()
      {
      }

      _ixmKeyField &operator =( const _ixmKeyField &field )
      {
         _order = field._order ;
         _nameLen = field._nameLen ;
         _name = field._name ;
         return ( *this ) ;
      }

      INT32 init( const BSONElement &element ) ;

      OSS_INLINE INT32 getOrder() const
      {
         return _order ;
      }

      OSS_INLINE UINT32 getNameLen() const
      {
         return _nameLen ;
      }

      OSS_INLINE const CHAR *getName() const
      {
         return _name ;
      }

   protected:
      // order of key
      INT32          _order ;
      // name length of key
      UINT32         _nameLen ;
      // field name of key
      const CHAR *   _name ;
   } ;

   typedef class _ixmKeyField ixmKeyField ;
   typedef _utilArray< ixmKeyField > IXM_KEY_FIELD_ARRAY ;

   // for element cache
   typedef _utilArray< BSONElement > IXM_KEY_ELEMENT_ARRAY ;

   /*
      _ixmIndexKeyBuilder define
    */
   class _ixmKeyBuilder : public utilPooledObject
   {
   public:
      _ixmKeyBuilder( BOOLEAN temporary = TRUE )
      : _temporary( temporary )
      {
      }

      ~_ixmKeyBuilder()
      {
      }

      IXM_KEY_ELEMENT_ARRAY &getKeyCache()
      {
         return _keyCache ;
      }

      BSONObjBuilder &getBuilder()
      {
         return _builder ;
      }

      BOOLEAN isTemporary() const
      {
         return _temporary ;
      }

   protected:
      IXM_KEY_ELEMENT_ARRAY   _keyCache ;
      BSONObjBuilder          _builder ;
      BOOLEAN                 _temporary ;
   } ;

   typedef class _ixmKeyBuilder ixmKeyBuilder ;

   // Index KeyGen is the operator to extract keys from given object
   // It depends on its underlying ixmIndexDetails control block
   // ixmIndexKeyGen is local to each thread
   class _ixmIndexKeyGen : public utilPooledObject
   {
   protected:
      // index key pattern
      BSONObj              _keyPattern ;

      // number of fields
      UINT32               _nFields ;
      // list of parsed key fields
      IXM_KEY_FIELD_ARRAY  _keyFields ;
      // undefined key contains specified number of elements
      // used to shortcut for generate undefined keys if the given
      // object matches no key pattern
      BSONObj              _undefinedKey ;

      // reusable key builder for building keys
      _ixmKeyBuilder *     _pKeyBuilder ;

      friend class _ixmKeyGenBase ;

   public:
      // default constructor
      _ixmIndexKeyGen() ;
      // create key generator from index control block
      _ixmIndexKeyGen ( const _ixmIndexCB *indexCB ) ;
      // create key generator from key def
      _ixmIndexKeyGen ( const BSONObj &keyDef ) ;
      // destructor
      ~_ixmIndexKeyGen() ;

      OSS_INLINE BOOLEAN isInit() const
      {
         return 0 != _nFields ;
      }

      // set reusable key builder
      OSS_INLINE void setKeyBuilder( _ixmKeyBuilder *pKeyBuilder )
      {
         _pKeyBuilder = pKeyBuilder ;
      }

      // this function overwrite _keyPattern.
      // This will make the ixmIndexKeyGen generate different key than
      // it supposed to
      INT32 setKeyPattern( const BSONObj &keyPattern ) ;

      OSS_INLINE const BSONObj &getKeyPattern() const
      {
         return _keyPattern ;
      }

      OSS_INLINE UINT32 getNFields() const
      {
         return _nFields ;
      }

      // get only one key object from object
      // for array key
      // - if order > 0, get smallest value of array
      // - if order < 0, get largest value of array
      INT32 getKeys ( const BSONObj &obj,
                      BSONObj &keys,
                      BSONElement *pArrEle = NULL,
                      BOOLEAN keepKeyName = FALSE,
                      BOOLEAN ignoreUndefined = FALSE,
                      BOOLEAN *pAllUndefined = NULL,
                      ixmKeyBuilder *pBuilder = NULL ) ;
      // get key set from object
      // for array key, generate all possible values from array
      INT32 getKeys ( const BSONObj &obj,
                      BSONObjSet &keySet,
                      BSONElement *pArrEle = NULL,
                      BOOLEAN keepKeyName = FALSE,
                      BOOLEAN ignoreUndefined = FALSE,
                      BOOLEAN *pAllUndefined = NULL,
                      ixmKeyBuilder *pBuilder = NULL ) ;

      static BOOLEAN validateKeyDef ( const BSONObj &keyDef ) ;

   protected:
      // disable copy
      _ixmIndexKeyGen( const _ixmIndexKeyGen & ) {}
      _ixmIndexKeyGen &operator =( const _ixmIndexKeyGen & ) { return *this ; }

      INT32 _init() ;
      void _release() ;

      // implement of get keys
      INT32 _getKeys( _ixmKeyGenBase *keyGen, BOOLEAN &allUndefined ) ;

      // extract keys from object
      INT32 _extractKeys( const BSONObj &obj,
                          IXM_KEY_ELEMENT_ARRAY &keyCache,
                          BOOLEAN &allUndefined,
                          BSONElement &arrEle,
                          const CHAR *&arrEleName,
                          INT32 &arrElePos ) ;

      // build keys
      INT32 _buildKeys( _ixmKeyBuilder *pBuilder,
                        BOOLEAN keepKeyName,
                        BOOLEAN ignoreUndefined,
                        BSONObj &keys ) ;

      // extract key from array
      INT32 _extractArrayKey( const BSONElement &arrEle,
                              const CHAR *arrEleName,
                              _ixmKeyGenBase *keyGen ) ;
   } ;
   typedef class _ixmIndexKeyGen ixmIndexKeyGen ;

   /*
      _ixmIdxHashBitmap define
    */
   // NOTE: hash index bitmap used to mark whether a field has been updated

   // calculate hash brings additional costs, so only consider update less
   // than 8 fields
   #define IXM_IDX_HASH_MAX_FIELD_NUM  ( 8 )

   // only save bitmap for first 8 indexes
   // NOTE: bitmap for collection will calculate from all indexes
   #define IXM_IDX_HASH_MAX_INDEX_NUM  ( 8 )

   // hash bitmap with 128 bits
   #define IXM_IDX_HASH_BITMAP_SIZE    ( 128 )

   class _ixmIdxHashBitmap : public _utilStackBitmap< IXM_IDX_HASH_BITMAP_SIZE >
   {
   public:
      void setFieldBit( const CHAR *fieldName )
      {
         setBit( calcIndex( fieldName ) ) ;
      }

      static UINT32 calcIndex( const CHAR *fieldName )
      {
         UINT32 hash = 5381 ;
         CHAR c ;
         // only take first level of field name
         while ( (c = *(fieldName ++)) && '.' != c )
            hash = ((hash << 5) + hash) + c ;
         return hash % IXM_IDX_HASH_BITMAP_SIZE ;
      }
   } ;

   typedef class _ixmIdxHashBitmap ixmIdxHashBitmap ;

   /*
      _ixmIdxHashArray define
    */

   // only maintain 7 hash values
   #define IXM_IDX_HASH_FIELD_NUM      ( 7 )

   class _ixmIdxHashArray : public _utilPooledObject
   {
   public:
      _ixmIdxHashArray()
      : _size( 0 )
      {
      }

      ~_ixmIdxHashArray() {}

      BOOLEAN isEmpty() const
      {
         return 0 == _size ;
      }

      BOOLEAN isValid() const
      {
         return _size > 0 && _size <= IXM_IDX_HASH_FIELD_NUM ;
      }

      void reset()
      {
         _size = 0 ;
      }

      void setField( UINT32 bitIndex )
      {
         // only maintain 7 hash values
         if ( _size < IXM_IDX_HASH_FIELD_NUM )
         {
            _fields[ _size ] = (UINT8)bitIndex ;
         }
         ++ _size ;
      }

      BOOLEAN testBitmap( const ixmIdxHashBitmap &bitmap ) const
      {
         if ( !bitmap.isEmpty() )
         {
            if ( _size > IXM_IDX_HASH_FIELD_NUM )
            {
               // too many fields, out-of-range hash values are missing
               return TRUE ;
            }
            else
            {
               // check if hash values in bitmap
               for ( UINT32 i = 0 ; i < _size ; ++ i )
               {
                  if ( bitmap.testBit( (UINT32)( _fields[ i ] ) ) )
                  {
                     return TRUE ;
                  }
               }
            }
         }

         return FALSE ;
      }

      ossPoolString toString() const
      {
         ixmIdxHashBitmap temp ;
         _toBitmap( temp ) ;
         return temp.toString() ;
      }

      BOOLEAN isEqual( const ixmIdxHashBitmap &bitmap ) const
      {
         ixmIdxHashBitmap temp ;
         _toBitmap( temp ) ;
         return temp.isEqual( bitmap ) ;
      }

      void mergeToBitmap( ixmIdxHashBitmap &bitmap )
      {
         _toBitmap( bitmap ) ;
      }

   protected:
      void _toBitmap( ixmIdxHashBitmap &bitmap ) const
      {
         for ( UINT32 i = 0 ; i < _size ; ++ i )
         {
            bitmap.setBit( (UINT32)_fields[ i ] ) ;
         }
      }

   protected:
      UINT8 _size ;
      UINT8 _fields[ IXM_IDX_HASH_FIELD_NUM ] ;
   } ;

   typedef class _ixmIdxHashArray ixmIdxHashArray ;

}

#endif //IXMINDEXKEY_HPP_

