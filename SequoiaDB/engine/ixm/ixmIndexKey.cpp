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

   Source File Name = ixmIndexKey.cpp

   Descriptive Name = Index Manager Index Key Generator

   When/how to use: this program may be used on binary and text-formatted
   versions of Index Manager component. This file contains functions for index
   key generator, which is used to create key pairs from data record and index
   definition.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ixm.hpp"
#include "ixmIndexKey.hpp"
#include "ossMem.hpp"
#include "pdTrace.hpp"
#include "ixmTrace.hpp"
#include "utilMemListPool.hpp"
#include "utilStr.hpp"
#include "ossLatch.hpp"

using namespace bson ;

namespace engine
{

   #define IXM_MAX_PREALLOCATED_UNDEFKEY        ( 10 )

   /*
       IXM Tool functions
    */
   BSONObj ixmGetUndefineKeyObj( INT32 fieldNum )
   {
      static BSONObj s_undefineKeys[ IXM_MAX_PREALLOCATED_UNDEFKEY ] ;
      static BOOLEAN s_init = FALSE ;
      static ossSpinXLatch s_latch ;

      // init undefine keys
      if ( FALSE == s_init )
      {
         s_latch.get() ;
         if ( FALSE == s_init )
         {
            for ( SINT32 i = 0; i < IXM_MAX_PREALLOCATED_UNDEFKEY ; ++i )
            {
               BSONObjBuilder b ;
               for ( SINT32 j = 0; j <= i; ++j )
               {
                  b.appendUndefined("") ;
               }
               s_undefineKeys[i] = b.obj() ;
            }
            s_init = TRUE ;
         }
         s_latch.release() ;
      }

      if ( fieldNum > 0 && fieldNum <= IXM_MAX_PREALLOCATED_UNDEFKEY )
      {
         return s_undefineKeys[ fieldNum - 1 ] ;
      }
      else
      {
         BSONObjBuilder b ;
         for ( INT32 i = 0; i < fieldNum; ++i )
         {
            b.appendUndefined("") ;
         }
         return b.obj() ;
      }
   }

   _ixmIndexCover::_ixmIndexCover( const BSONObj &keyPattern )
   : _keyPattern( keyPattern ),
     _treeInited( FALSE ),
     _keyFieldMapInited( FALSE ),
     _nfields( 0 ),
     _bufSize( 0 ),
     _bufPtr( NULL ),
     _extraSize( 0 )
   {
   }
   _ixmIndexCover::~_ixmIndexCover()
   {
      if( _bufPtr )
      {
         SDB_THREAD_FREE( _bufPtr ) ;
      }
   }

   INT32 _ixmIndexCover::_initKeyFieldMap()
   {
      INT32 rc = SDB_OK ;
      try
      {
         _nfields = 0;
         _keyPattern = _keyPattern.getOwned() ;
         BSONObjIterator iter( _keyPattern ) ;
         // ensure _keyFieldMap is empty
         _keyFieldMap.clear() ;
         while( iter.more() )
         {
            BSONElement ele = iter.next() ;
            _keyFieldMap[ ele.fieldName() ] = _nfields ++;
         }
         _keyFieldMapInited = TRUE ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDWARNING, "Init index field map, Occur exception: %s", e.what() ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // keyPattern is { "a.c":1,"a.b":1 }
   // then tree is
   //   a   //
   //  / \  //
   // b   c //
   INT32 _ixmIndexCover::_initTree()
   {
      INT32 rc = SDB_OK ;
      // index tree has be generated, direct return root node pointer
      if( !_treeInited )
      {
         if( !_keyFieldMapInited )
         {
            rc = _initKeyFieldMap() ;
            PD_RC_CHECK( rc, PDWARNING, "Init index field map failed, rc: %d", rc ) ;
         }
         try
         {
            IXM_INDEX_FIELD_MAP::iterator iter ;
            // ensure _root is empty
            _root.reset() ;
            // reserve vector capacity for children
            rc = _root.childrenReserve( _keyFieldMap.size() ) ;
            PD_RC_CHECK( rc, PDWARNING, "Index field tree reserve children space failed, rc: %d",
                         rc ) ;

            for( iter = _keyFieldMap.begin() ;
                 iter != _keyFieldMap.end() ;
                 iter ++ )
            {
               ixmIndexNode *node = NULL ;
               // the node is the leaf of the field
               rc = _fieldNameToNodes( iter->first, node ) ;
               PD_RC_CHECK( rc, PDWARNING, "Index field name to nodes failed, rc: %d", rc ) ;
               // iter->second is the index of field in key
               // if a.b is contained by a ,then node is null
               if( NULL != node )
               {
                  node->setFieldIndex( iter->second );
               }
            }
            _treeInited = TRUE ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_RC_CHECK( rc, PDWARNING, "Init index field tree Occur exception: %s", e.what() ) ;
         }
      }
   done:
      return rc ;
   error:
      _root.reset() ;
      goto done ;
   }

   // name:a.c then nodes relation as flows:
   //      a   //
   //     /    //
   //    c     //
   // node c is leaf node as arg out
   // if already exist tree a after a.c merged. then tree is a     //
   //                      /                                / \    //
   //                     b                                b   c   //
   INT32 _ixmIndexCover::_fieldNameToNodes( const CHAR* fieldName,
                                            ixmIndexNode *&node )
   {
      INT32 rc = SDB_OK ;
      UINT32 level = 0 ;
      ixmIndexNode *parent = &_root ;
      ixmIndexNode *notAppendNode = NULL ;
      const char* pos = fieldName ;

      try
      {
         while ( pos )
         {
            const char *p = strchr(pos, '.');
            INT32 len = 0 ;
            len = p? (p - pos) : ossStrlen( pos ) ;
            StringData name( pos, len ) ;
            pos = p? (p + 1) : NULL ;

            level ++ ;

            ixmIndexNode *lastNode = parent->getLastChild() ;
            if( NULL == lastNode )
            {
               node = SDB_OSS_NEW ixmIndexNode( name, level,
                                                parent->childrenCapacity()/2 + 1 ) ;

               PD_CHECK( NULL != node, SDB_OOM, error, PDWARNING,
                         "Alloc index node memory failed, rc: %d", SDB_OOM ) ;

               notAppendNode = node ;
               rc = parent->appendChild( node ) ;
               PD_RC_CHECK( rc, PDWARNING, "Append child failed, rc: %d", rc ) ;
               notAppendNode = NULL ;
            }
            else
            {
               if ( name.size() == lastNode->getName().size() &&
                    0 == ossStrncmp( name.data(), lastNode->getName().data(), name.size() ) )
               {
                  if( lastNode->isLeaf() )
                  {
                     // contain by last field, ignore current field
                     break ;
                  }
                  else
                  {
                     node = lastNode ;
                  }
               }
               else
               {
                  node = SDB_OSS_NEW ixmIndexNode( name, level,
                                                   parent->childrenCapacity()/2 + 1) ;
                  PD_CHECK( NULL != node, SDB_OOM, error, PDWARNING,
                            "Alloc index node memory failed, rc: %d", SDB_OOM ) ;
                  notAppendNode = node ;
                  rc = parent->appendChild( node ) ;
                  PD_RC_CHECK( rc, PDWARNING, "Append child failed, rc: %d", rc ) ;
                  notAppendNode = NULL ;
               }
            }
            parent = node ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_RC_CHECK( rc, PDWARNING, "Index filed name to nodes, Occur exception: %s", e.what() ) ;
      }
   done:
      return rc ;
   error:
      if( notAppendNode )
      {
         SDB_OSS_DEL( notAppendNode ) ;
      }
      goto done ;
   }

   UINT32 _ixmIndexCover::getNfields()
   {
      return _nfields ;
   }
   INT32 _ixmIndexCover::getExtraSize( UINT32 &size )
   {
      INT32 rc = _initTree() ;
      PD_RC_CHECK( rc, PDWARNING, "Init index tree failed, rc: %d", rc ) ;
      if( 0 == _extraSize )
      {
         _extraSize = _root.getExtraSize() ;
      }
      size = _extraSize ;

   done:
      return rc ;
   error:
      goto done ;
   }
   INT32 _ixmIndexCover::getTree( ixmIndexNode *&tree )
   {
      INT32 rc = _initTree() ;
      PD_RC_CHECK( rc, PDWARNING, "Init index tree failed, rc: %d", rc ) ;
      tree = &_root ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _ixmIndexCover::ensureBuff( UINT32 size, CHAR *&pBuff )
   {
      INT32 rc = SDB_OK ;
      if( _bufSize >= size && _bufPtr)
      {
         pBuff = _bufPtr ;
      }
      else if( NULL != _bufPtr )
      {
         pBuff = ( CHAR* )SDB_THREAD_REALLOC( _bufPtr, size ) ;
         if( NULL == pBuff )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         _bufPtr = pBuff ;
         _bufSize = size ;
      }
      else
      {
         pBuff = ( CHAR* )SDB_THREAD_ALLOC( size ) ;
         if( NULL == pBuff )
         {
            _bufSize = 0 ;
            _bufPtr = NULL ;
            rc = SDB_OOM ;
            goto error ;
         }
         _bufPtr = pBuff ;
         _bufSize = size ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR* _ixmIndexCover::getBuf() const
   {
      return _bufPtr ;
   }

   INT32 _ixmIndexCover::reInitContainer()
   {
      INT32 rc = SDB_OK ;
      _container.clear( FALSE ) ;
      if( _container.capacity() < _nfields )
      {
         rc = _container.reserve( _nfields ) ;
      }
      return rc ;
   }

   IXM_ELE_RAWDATA_ARRAY& _ixmIndexCover::getContainer()
   {
      return _container ;
   }

   BOOLEAN _ixmIndexCover::cover( const CHAR* fieldName )
   {
      BOOLEAN indexCover = FALSE ;
      INT32 rc = SDB_OK ;

      if( NULL == fieldName || '\0' == *fieldName )
      {
         indexCover = TRUE ;
         goto done ;
      }

      if( !_keyFieldMapInited )
      {
         rc = _initKeyFieldMap() ;
         PD_RC_CHECK( rc, PDWARNING, "Init index field map failed, rc: %d", rc ) ;
      }

      if( _keyFieldMap.end() != _keyFieldMap.find( fieldName ) )
      {
         indexCover = TRUE ;
         goto done ;
      }

   done:
      return indexCover ;
   error:
      goto done ;
   }

   BOOLEAN _ixmIndexCover::cover( const IXM_FIELD_NAME_SET &fieldSet )
   {
      BOOLEAN indexCover = FALSE ;

      IXM_FIELD_NAME_SET::iterator iter = fieldSet.begin() ;
      for( iter = fieldSet.begin() ;
         iter != fieldSet.end() ;
         iter ++ )
      {
         if( FALSE == cover( *iter ) )
         {
            goto done ;
         }
      }
      indexCover = TRUE ;
   done:
      return indexCover ;
   }

   /*
      IXM Global opt var
    */
   const static BSONObj g_UndefinedObj(
                              BSONObjBuilder().appendUndefined( "" ).obj() ) ;
   const static BSONElement g_UndefinedElt = g_UndefinedObj.firstElement() ;

   /*
      _ixmKeyField implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMKEYFIELD_INIT, "_ixmKeyField::init" )
   INT32 _ixmKeyField::init( const BSONElement &element )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMKEYFIELD_INIT ) ;

      try
      {
         // cache field name
         _name = element.fieldName() ;
         // cache length of name
         _nameLen = ossStrlen( _name ) ;
         if ( element.isNumber() )
         {
            // cache order
            _order = element.numberInt() ;
            PD_CHECK( -1 == _order || 1 == _order,
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to initialize key field [%s], "
                      "order [%d] is invalid", _name, _order ) ;
         }
         else if ( String == element.type() &&
                   ( 0 == ossStrcmp( IXM_2D_KEY_TYPE,
                                     element.valuestr() ) ||
                     0 == ossStrcmp( IXM_TEXT_KEY_TYPE,
                                     element.valuestr() ) ) )

         {
            // default order
            _order = 1 ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to parse order, it is invalid format" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize key field, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMKEYFIELD_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _ixmKeyGenBase define and implement
    */
   class _ixmKeyGenBase : public utilPooledObject
   {
   public:
      _ixmKeyGenBase( ixmIndexKeyGen *pIndexGen,
                      const BSONObj *pObject,
                      BOOLEAN keepKeyName,
                      BOOLEAN ignoreUndefined,
                      BSONElement *arrEle,
                      ixmKeyBuilder *pBuilder )
      : _pIndexGen( pIndexGen ),
        _pObject( pObject ),
        _keepKeyName( keepKeyName ),
        _ignoreUndefined( ignoreUndefined ),
        _arrElePos( -1 ),
        _pArrEle( arrEle ),
        _pArrKeyField( NULL ),
        _tempBuilder( TRUE ),
        _pBuilder( ( NULL == pBuilder ) ?
                   ( ( NULL == pIndexGen->_pKeyBuilder ) ?
                     ( &_tempBuilder ) :
                     ( pIndexGen->_pKeyBuilder ) ) :
                   ( pBuilder ) )
      {
         SDB_ASSERT( NULL != _pIndexGen, "index generator is invalid" ) ;
         SDB_ASSERT( NULL != _pObject, "object is invalid" ) ;
         SDB_ASSERT( NULL != _pBuilder, "builder is invalid" ) ;
      }

      virtual ~_ixmKeyGenBase()
      {
      }

      OSS_INLINE const BSONObj &getObject() const
      {
         return ( *_pObject ) ;
      }

      OSS_INLINE IXM_KEY_ELEMENT_ARRAY &getKeyCache()
      {
         return _pBuilder->getKeyCache() ;
      }

      OSS_INLINE ixmKeyField *getArrKeyField()
      {
         return _pArrKeyField ;
      }

      OSS_INLINE void saveArrEle( const BSONElement &arrEle )
      {
         if ( NULL != _pArrEle )
         {
            *_pArrEle = arrEle ;
         }
      }

      OSS_INLINE virtual BOOLEAN needParseOrder() const
      {
         return FALSE ;
      }

      virtual INT32 saveWithUndefinedKeys() = 0 ;
      virtual INT32 saveWithNormalKeys() = 0 ;
      virtual INT32 saveWithArrayKey( const BSONElement &arrEle ) = 0 ;

      OSS_INLINE virtual INT32 prepareForArrayKey( INT32 arrElePos,
                                                   ixmKeyField *pArrKeyField )
      {
         SDB_ASSERT( arrElePos >= 0 && arrElePos < (INT32)( _getNFields() ),
                     "array position is invalid" ) ;
         SDB_ASSERT( NULL != pArrKeyField, "array key field is invalid" ) ;
         _arrElePos = arrElePos ;
         _pArrKeyField = pArrKeyField ;
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 doneForArrayKey()
      {
         return SDB_OK ;
      }

   protected:
      OSS_INLINE INT32 _buildKeys( BSONObj &keys )
      {
         return _pIndexGen->_buildKeys( _pBuilder,
                                        _keepKeyName,
                                        _ignoreUndefined,
                                        keys ) ;
      }

      OSS_INLINE UINT32 _getNFields() const
      {
         return _pIndexGen->getNFields() ;
      }

      OSS_INLINE const BSONObj &_getKeyPattern() const
      {
         return _pIndexGen->_keyPattern ;
      }

      OSS_INLINE IXM_KEY_FIELD_ARRAY &_getKeyFields()
      {
         return _pIndexGen->_keyFields ;
      }

      OSS_INLINE ixmKeyField &_getKeyField( INT32 pos )
      {
         return _pIndexGen->_keyFields[ pos ] ;
      }

      OSS_INLINE const BSONObj &_getUndefinedKeys() const
      {
         return _pIndexGen->_undefinedKey ;
      }

   protected:
      ixmIndexKeyGen *  _pIndexGen ;
      const BSONObj *   _pObject ;
      BOOLEAN           _keepKeyName ;
      BOOLEAN           _ignoreUndefined ;
      INT32             _arrElePos ;
      BSONElement *     _pArrEle ;
      ixmKeyField *     _pArrKeyField ;
      ixmKeyBuilder     _tempBuilder ;
      ixmKeyBuilder *   _pBuilder ;
   } ;

   typedef class _ixmKeyGenBase ixmKeyGenBase ;

   /*
      _ixmKeyObjGen define
    */
   class _ixmKeyObjGen : public _ixmKeyGenBase
   {
   public:
      _ixmKeyObjGen( ixmIndexKeyGen *pIndexGen,
                     const BSONObj *pObject,
                     BSONObj *pOutputKeys,
                     BOOLEAN keepKeyName,
                     BOOLEAN ignoreUndefined,
                     BSONElement *pArrEle,
                     ixmKeyBuilder *pBuilder )
      : _ixmKeyGenBase( pIndexGen, pObject, keepKeyName, ignoreUndefined,
                        pArrEle, pBuilder ),
        _pOutputKeys( pOutputKeys )
      {
         SDB_ASSERT( NULL != pOutputKeys, "output keys is invalid" ) ;
      }

      virtual ~_ixmKeyObjGen()
      {
      }

      OSS_INLINE virtual BOOLEAN needParseOrder() const
      {
         // generate a single key, we need pick one key against order in key
         // pattern from key sets if given object contains array key field,
         // so we need to care order
         return TRUE ;
      }

      OSS_INLINE virtual INT32 saveWithUndefinedKeys()
      {
         if ( !_ignoreUndefined )
         {
            *_pOutputKeys = _getUndefinedKeys() ;
         }
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 saveWithNormalKeys()
      {
         return _buildKeys( *_pOutputKeys ) ;
      }

      virtual INT32 saveWithArrayKey( const BSONElement &arrEle ) ;

      OSS_INLINE virtual INT32 prepareForArrayKey( INT32 arrElePos,
                                                   ixmKeyField *pArrKeyField )
      {
         _foundArrEle = BSONElement() ;
         return _ixmKeyGenBase::prepareForArrayKey( arrElePos, pArrKeyField ) ;
      }

      OSS_INLINE virtual INT32 doneForArrayKey()
      {
         _pBuilder->getKeyCache()[ _arrElePos ] = _foundArrEle ;
         return _buildKeys( *_pOutputKeys ) ;
      }

   protected:
      BSONObj *   _pOutputKeys ;
      BSONElement _foundArrEle ;
   } ;

   typedef class _ixmKeyObjGen ixmKeyObjGen ;

   /*
      _ixmKeyObjGen implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMKEYOBJGEN_SAVEWITHARRKEY, "_ixmKeyObjGen::saveWithArrayKey" )
   OSS_INLINE INT32 _ixmKeyObjGen::saveWithArrayKey( const BSONElement &arrEle )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_IXMKEYOBJGEN_SAVEWITHARRKEY ) ;

      // found array element needs to be returned
      if ( _foundArrEle.eoo() )
      {
         // not found yet, save the field
         _foundArrEle = arrEle ;
      }
      else if ( !arrEle.eoo() || !_ignoreUndefined )
      {
         // already found one, save the field with comparison against key
         // pattern
         INT32 order = _pArrKeyField->getOrder() ;

         // - if order > 0, get smallest value of array
         // - if order < 0, get largest value of array
         if ( ( order > 0 &&
                _foundArrEle.woCompare( arrEle, FALSE ) > 0 ) ||
              ( order < 0 &&
                _foundArrEle.woCompare( arrEle, FALSE ) < 0 ) )
         {
            _foundArrEle = arrEle ;
         }
      }

      PD_TRACE_EXITRC( SDB_IXMKEYOBJGEN_SAVEWITHARRKEY, rc ) ;

      return rc ;
   }

   /*
      _ixmKeySetGen define
    */
   class _ixmKeySetGen : public _ixmKeyGenBase
   {
   public:
      _ixmKeySetGen( ixmIndexKeyGen *pIndexGen,
                     const BSONObj *pObject,
                     BSONObjSet *pOutputKeySet,
                     BOOLEAN keepKeyName,
                     BOOLEAN ignoreUndefined,
                     BSONElement *pArrEle,
                     ixmKeyBuilder *pBuilder )
      : _ixmKeyGenBase( pIndexGen, pObject, keepKeyName, ignoreUndefined,
                        pArrEle, pBuilder ),
        _pKeySet( pOutputKeySet )
      {
         SDB_ASSERT( NULL != pOutputKeySet, "output key set is invalid" ) ;
      }

      virtual ~_ixmKeySetGen()
      {
      }

      OSS_INLINE virtual BOOLEAN needParseOrder() const
      {
         // generate key set, we will generate all keys into key set,
         // so we don't care order
         return FALSE ;
      }

      OSS_INLINE virtual INT32 saveWithUndefinedKeys()
      {
         if ( !_ignoreUndefined )
         {
            _pKeySet->insert( _getUndefinedKeys() ) ;
         }
         return SDB_OK ;
      }

      OSS_INLINE virtual INT32 saveWithNormalKeys()
      {
         return _saveKeys() ;
      }

      OSS_INLINE virtual INT32 saveWithArrayKey( const BSONElement &arrEle )
      {
         _pBuilder->getKeyCache()[ _arrElePos ] = arrEle ;
         return _saveKeys() ;
      }

   protected:
      INT32 _saveKeys() ;

   protected:
      BSONObjSet *   _pKeySet ;
   } ;

   typedef class _ixmKeySetGen ixmKeySetGen ;

   /*
      _ixmKeySetGen implement
    */
   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMKEYSETGEN__SAVEKEYS, "_ixmKeySetGen::_saveKeys" )
   OSS_INLINE INT32 _ixmKeySetGen::_saveKeys()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_IXMKEYSETGEN__SAVEKEYS ) ;

      BSONObj outputKeys ;

      rc = _buildKeys( outputKeys ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build keys, rc: %d", rc ) ;

      if ( !outputKeys.isEmpty() )
      {
         _pKeySet->insert( outputKeys.getOwned() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_IXMKEYSETGEN__SAVEKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _ixmIndexKeyGen implement
    */
   // default constructor
   _ixmIndexKeyGen::_ixmIndexKeyGen()
   : _nFields( 0 ),
     _pKeyBuilder( NULL )
   {
   }

   // create key generator from index control block
   _ixmIndexKeyGen::_ixmIndexKeyGen ( const _ixmIndexCB *indexCB )
   : _nFields( 0 ),
     _pKeyBuilder( NULL )
   {
      SDB_ASSERT ( indexCB, "details can't be NULL" ) ;
      _keyPattern = indexCB->keyPattern() ;
      if ( SDB_OK != _init() )
      {
         PD_LOG( PDWARNING, "Failed to initialize key generator" ) ;
      }
   }
   // create key generator from key
   _ixmIndexKeyGen::_ixmIndexKeyGen ( const BSONObj &keyDef )
   : _nFields( 0 ),
     _pKeyBuilder( NULL )
   {
      try
      {
         _keyPattern = keyDef.getOwned() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to get key pattern owned, "
                 "occur exception: %s", e.what() ) ;
      }
      if ( SDB_OK != _init() )
      {
         PD_LOG( PDWARNING, "Failed to initialize key generator" ) ;
      }
   }

   _ixmIndexKeyGen::~_ixmIndexKeyGen()
   {
      _release() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMINXKEYGEN_GETKEYS_OBJ, "ixmIndexKeyGen::getKeys" )
   INT32 _ixmIndexKeyGen::getKeys ( const BSONObj &obj,
                                    BSONObj &keys,
                                    BSONElement *pArrEle,
                                    BOOLEAN keepKeyName,
                                    BOOLEAN ignoreUndefined,
                                    BOOLEAN *pAllUndefined,
                                    ixmKeyBuilder *pBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_IXMINXKEYGEN_GETKEYS_OBJ ) ;

      BOOLEAN allUndefined = FALSE ;
      ixmKeyObjGen keyGen( this, &obj, &keys, keepKeyName, ignoreUndefined,
                           pArrEle, pBuilder ) ;

      rc = _getKeys( &keyGen, allUndefined ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get keys from object, "
                   "rc: %d", rc ) ;

      if ( NULL != pAllUndefined )
      {
         *pAllUndefined = allUndefined ;
      }

   done:
      PD_TRACE_EXITRC( SDB_IXMINXKEYGEN_GETKEYS_OBJ, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMINXKEYGEN_GETKEYS_SET, "ixmIndexKeyGen::getKeys" )
   INT32 _ixmIndexKeyGen::getKeys ( const BSONObj &obj,
                                    BSONObjSet &keySet,
                                    BSONElement *pArrEle,
                                    BOOLEAN keepKeyName,
                                    BOOLEAN ignoreUndefined,
                                    BOOLEAN *pAllUndefined,
                                    ixmKeyBuilder *pBuilder )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_IXMINXKEYGEN_GETKEYS_SET ) ;

      BOOLEAN allUndefined = FALSE ;
      ixmKeySetGen keyGen( this, &obj, &keySet, keepKeyName, ignoreUndefined,
                           pArrEle, pBuilder ) ;

      rc = _getKeys( &keyGen, allUndefined ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get key set from object, "
                   "rc: %d", rc ) ;

      if ( NULL != pAllUndefined )
      {
         *pAllUndefined = allUndefined ;
      }

   done:
      PD_TRACE_EXITRC( SDB_IXMINXKEYGEN_GETKEYS_SET, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMINXKEYGEN_SETKEYPATTERN, "ixmIndexKeyGen::setKeyPattern" )
   INT32 _ixmIndexKeyGen::setKeyPattern( const BSONObj &keyPattern )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_IXMINXKEYGEN_SETKEYPATTERN ) ;

      // release old generator
      _release() ;

      try
      {
         _keyPattern = keyPattern.getOwned() ;
         PD_CHECK( !_keyPattern.isEmpty(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to reset key pattern, given "
                   "key pattern is empty" ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to set key pattern, occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // initialize with new key pattern
      rc = _init() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize key generator,"
                   "rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB_IXMINXKEYGEN_SETKEYPATTERN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // note this validate is validating whether an key def has fields other than
   // 1/-1, this check should NOT be directly used against an index key def,
   // because it may contains inregular key def like 2d index
   BOOLEAN _ixmIndexKeyGen::validateKeyDef ( const BSONObj &keyDef )
   {
      BSONObjIterator i ( keyDef ) ;
      INT32 count = 0 ;
      while ( i.more () )
      {
         ++count ;
         BSONElement ie = i.next () ;

         // Check key name first
         const CHAR *fieldName = ie.fieldName() ;
         if ( NULL == fieldName ||
              '\0' == fieldName[0] ||
              NULL != ossStrchr( fieldName, '$' ) )
         {
            return FALSE ;
         }

         // Check key order
         if ( ie.type() != NumberInt ||
              ( ie.numberInt() != -1 &&
                ie.numberInt() != 1 ) )
         {
            return FALSE ;
         }
      }
      // at least we need 1 field
      return 0 != count ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__INIT, "_ixmIndexKeyGen::_init" )
   INT32 _ixmIndexKeyGen::_init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMINXKEYGEN__INIT ) ;

      _release() ;

      if ( _keyPattern.isEmpty() )
      {
         goto done ;
      }

      try
      {
         // for each key in key pattern, initialize key fields
         BSONObjIterator iter( _keyPattern ) ;
         while( iter.more() )
         {
            BSONElement keyElement = iter.next() ;

            ixmKeyField keyField ;
            rc = keyField.init( keyElement ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to initialize key field, "
                         "rc: %d", rc ) ;

            rc = _keyFields.append( keyField ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to add key field, rc: %d",
                         rc ) ;
            ++ _nFields ;
         }

         // check if empty
         PD_CHECK( _nFields > 0, SDB_SYS, error, PDERROR,
                   "Failed to parse key pattern, fields are empty" ) ;

         // generate undefined keys
         // WARNING: for keepKeyName mode, this should generate with
         // field names, but for forward compatibility, keep it without
         // field names
         _undefinedKey = ixmGetUndefineKeyObj( _nFields ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to initialize key generator, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMINXKEYGEN__INIT, rc ) ;
      return rc ;

   error:
      _release() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__RELEASE, "_ixmIndexKeyGen::_release" )
   void _ixmIndexKeyGen::_release()
   {
      PD_TRACE_ENTRY( SDB__IXMINXKEYGEN__RELEASE ) ;

      _nFields = 0 ;
      _keyFields.clear() ;
      _undefinedKey = BSONObj() ;

      PD_TRACE_EXIT( SDB__IXMINXKEYGEN__RELEASE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__GETKEYS, "_ixmIndexKeyGen::_getKeys" )
   INT32 _ixmIndexKeyGen::_getKeys( _ixmKeyGenBase *keyGen,
                                    BOOLEAN &allUndefined )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMINXKEYGEN__GETKEYS ) ;

      SDB_ASSERT( NULL != keyGen, "key generator is invalid" ) ;

      PD_CHECK( isInit(), SDB_SYS, error, PDERROR,
                "Failed to get keys, key generator is not initialized " ) ;

      try
      {
         SDB_ASSERT( _nFields > 0 && _nFields == _keyFields.size(),
                     "key fields are invalid" ) ;

         BSONElement arrEle ;
         const CHAR *arrEleName = NULL ;
         INT32 arrElePos = -1 ;

         rc = _extractKeys( keyGen->getObject(), keyGen->getKeyCache(),
                            allUndefined, arrEle, arrEleName, arrElePos ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to extract keys from object, "
                      "rc: %d", rc ) ;

         if ( allUndefined )
         {
            // no field is found, generate a undefined key
            rc = keyGen->saveWithUndefinedKeys() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save undefined keys, "
                         "rc: %d", rc ) ;
         }
         else if ( !arrEle.eoo() )
         {
            // array is found, get key from array
            SDB_ASSERT( arrElePos >= 0, "invalid array position" ) ;
            SDB_ASSERT( NULL != arrEleName, "invalid array name" ) ;

            ixmKeyField &arrKeyField = _keyFields[ arrElePos ] ;

            rc = keyGen->prepareForArrayKey( arrElePos, &arrKeyField ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to prepare for "
                         "extract array key [%s], rc: %d",
                         arrEleName, rc ) ;
            rc = _extractArrayKey( arrEle, arrEleName, keyGen ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to extract keys with array "
                         "element [%s], rc: %d", arrEleName, rc ) ;

            rc = keyGen->doneForArrayKey() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to done for "
                         "extract array key [%s], rc: %d",
                         arrEleName, rc ) ;
         }
         else
         {
            rc = keyGen->saveWithNormalKeys() ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save keys with "
                         "normal elements, rc: %d", rc ) ;
         }

         // save the array element if needed
         keyGen->saveArrEle( arrEle ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed get keys from object, occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMINXKEYGEN__GETKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__EXTRACTKEYS, "_ixmIndexKeyGen::_extractKeys" )
   INT32 _ixmIndexKeyGen::_extractKeys( const BSONObj &obj,
                                        IXM_KEY_ELEMENT_ARRAY &keyCache,
                                        BOOLEAN &allUndefined,
                                        BSONElement &arrEle,
                                        const CHAR *&arrEleName,
                                        INT32 &arrElePos )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMINXKEYGEN__EXTRACTKEYS ) ;

      UINT32 eooNum = 0 ;

      rc = keyCache.resize( _nFields ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to prepare key cache, rc: %d", rc ) ;

      for ( UINT32 i = 0 ; i < _nFields ; ++ i )
      {
         const CHAR *name = _keyFields[ i ].getName() ;
         SDB_ASSERT( '\0' != name[0], "can not be empty" ) ;
         BSONElement e = obj.getFieldDottedOrArray( name ) ;

         // if key is a and obj is {a:{b:[1,2,3]}} then e.type is Object
         if ( EOO == e.type() )
         {
            // field not found
            ++ eooNum ;
         }
         else if ( Array == e.type() )
         {
            // check if already found an array
            PD_CHECK( EOO == arrEle.type(), SDB_IXM_MULTIPLE_ARRAY, error,
                      PDERROR, "Failed to extract key for field [%s], "
                      "at most one array can be in the key, "
                      "already have array field [%s]", e.fieldName(),
                      arrEle.fieldName() ) ;
            arrEle = e ;
            arrEleName = name ;
            arrElePos = i ;
         }
         // cache in key field, and will be generated in an output key object
         // later
         keyCache[ i ] = e ;
      }

      allUndefined = ( eooNum == _nFields ? TRUE : FALSE ) ;

   done:
      PD_TRACE_EXITRC( SDB__IXMINXKEYGEN__EXTRACTKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__BUILDKEYS, "_ixmIndexKeyGen::_buildKeys" )
   INT32 _ixmIndexKeyGen::_buildKeys( _ixmKeyBuilder *pBuilder,
                                      BOOLEAN keepKeyName,
                                      BOOLEAN ignoreUndefined,
                                      BSONObj &keys )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMINXKEYGEN__BUILDKEYS ) ;

      SDB_ASSERT( NULL != pBuilder, "builder is invalid" ) ;

      try
      {
         IXM_KEY_ELEMENT_ARRAY &keyArray = pBuilder->getKeyCache() ;
         BSONObjBuilder &builder = pBuilder->getBuilder() ;

         builder.reset() ;
         for ( UINT32 i = 0 ; i < _nFields ; ++ i )
         {
            const CHAR *keyName = "" ;
            UINT32 keyNameLen = 0 ;
            const BSONElement &keyEle = keyArray[ i ] ;
            if ( keepKeyName )
            {
               const ixmKeyField &field = _keyFields[ i ] ;
               // keep key name
               keyName = field.getName() ;
               keyNameLen = field.getNameLen() ;
            }
            if ( keyEle.eoo() || Undefined == keyEle.type() )
            {
               if ( !ignoreUndefined )
               {
                  builder.appendUndefined( StringData( keyName,
                                                       keyNameLen ) ) ;
               }
            }
            else
            {
               builder.appendAs( keyEle,
                                 StringData( keyName, keyNameLen ) ) ;
            }
         }

         if ( pBuilder->isTemporary() )
         {
            // temporary builder, take owned
            keys = builder.obj() ;
         }
         else
         {
            // WARNING: reusable builder, should not take owned
            keys = builder.done() ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to build keys, occur exception: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMINXKEYGEN__BUILDKEYS, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__EXTARRKEY, "_ixmIndexKeyGen::_extractArrayKey" )
   INT32 _ixmIndexKeyGen::_extractArrayKey( const BSONElement &arrEle,
                                            const CHAR *arrEleName,
                                            _ixmKeyGenBase *keyGen )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__IXMINXKEYGEN__EXTARRKEY ) ;

      SDB_ASSERT( NULL != keyGen->getArrKeyField(),
                  "array key field is invalid" ) ;

      BSONObj arrObj = arrEle.embeddedObject() ;

      /// the element must be an empty array key when it is a empty array
      if ( arrObj.firstElement().eoo() )
      {
         // the matched field is empty array, save as empty array
         rc = keyGen->saveWithArrayKey( arrEle ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to save array element "
                      "in array key field [%s], rc: %d",
                      keyGen->getArrKeyField()->getName(), rc ) ;
      }
      else if ( '\0' == *arrEleName )
      {
         // hit the end of name, but still an array
         // we need to unpack the array, each element will be a key
         BSONObjIterator itr( arrObj ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;

            // on found the array key
            rc = keyGen->saveWithArrayKey( e ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save array element "
                         "in array key field [%s], rc: %d",
                         keyGen->getArrKeyField()->getName(), rc ) ;
         }
      }
      else
      {
         // unpack the array to find the field
         BSONObjIterator itr( arrObj ) ;
         while ( itr.more() )
         {
            const CHAR *curEleName = arrEleName ;
            BSONElement next = itr.next() ;
            BSONElement subEle = g_UndefinedElt ;
            if ( Object == next.type() )
            {
               // search deeper into the sub-object
               subEle =
                     next.embeddedObject().getFieldDottedOrArray( curEleName ) ;
               if ( Array == subEle.type() )
               {
                  // still got an array, should recursively extract
                  // from sub-object with sub-fields
                  rc = _extractArrayKey( subEle, curEleName, keyGen ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to extract key from array"
                               "element [%s], rc: %d",
                               keyGen->getArrKeyField()->getName(), rc ) ;
                  continue ;
               }
               // else if not an array, means we finally found the element
               // inside the sub-object
            }
            // else if not an object, means not matched
            // use the EOO element, which will build as undefined

            // on found the array key
            rc = keyGen->saveWithArrayKey( subEle ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to save array element "
                         "in array key field [%s], rc: %d",
                         keyGen->getArrKeyField()->getName(), rc ) ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__IXMINXKEYGEN__EXTARRKEY, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
