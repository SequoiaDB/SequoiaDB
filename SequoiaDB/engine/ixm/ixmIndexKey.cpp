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

   /*
      IXM Global opt var
   */
   const static BSONObj gUndefinedObj =
          BSONObjBuilder().appendUndefined("").obj() ;
   const static BSONElement gUndefinedElt = gUndefinedObj.firstElement() ;

   // provide a BSON object, generate keys based on index keygen
   // this object is only used by ixmIndexKeyGen class
   // this class only have 1 external function "getKeys" to extract a given
   // object to BSONObjSet. Note keys may contain one or more key, when there is
   // array included in the object
   class _ixmKeyGenerator
   {
   protected:
      const _ixmIndexKeyGen            *_keygen ;
      mutable ossPoolVector<BSONObj *> _objs ;
   public:
      _ixmKeyGenerator ( const _ixmIndexKeyGen *keygen )
      {
         _keygen = keygen ;
      }
      ~_ixmKeyGenerator()
      {
         ossPoolVector<BSONObj *>::iterator itr = _objs.begin() ;
         for ( ; itr != _objs.end(); itr++ )
         {
            SDB_OSS_DEL *itr ;
         }
      }
      // input: BSONObj obj
      // output: BSONObjSet &keys
      // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMKEYGEN_GETKEYS, "$_ixmKeyGenerator::getKeys" )
      INT32 getKeys ( const BSONObj &obj, BOOLEAN isKeepKeyName,
                      BSONObjSet &keys,
                      BSONElement *pArrEle,
                      BOOLEAN ignoreUndefined = FALSE ) const
      {
         INT32 rc = SDB_OK ;
         PD_TRACE_ENTRY ( SDB__IXMKEYGEN_GETKEYS );
         SDB_ASSERT( _keygen, "spec can't be NULL" ) ;
         SDB_ASSERT( !_keygen->_fieldNames.empty(), "can not be empty" ) ;
         ossPoolVector<const CHAR*> fieldNames ( _keygen->_fieldNames ) ;
         BSONElement arrEle ;
         try
         {
            rc = _getKeys( fieldNames, obj, isKeepKeyName, keys,
                           &arrEle, ignoreUndefined ) ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err:%s", e.what() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG ( PDERROR, "Failed to generate key from object: %s",
                     obj.toString().c_str() ) ;
            goto error ;
         }

         if ( keys.empty() && !ignoreUndefined )
         {
            keys.insert ( _keygen->_undefinedKey ) ;
         }

         if ( NULL != pArrEle && !arrEle.eoo() )
         {
            *pArrEle = arrEle ;
         }
      done :
         PD_TRACE_EXITRC ( SDB__IXMKEYGEN_GETKEYS, rc );
         return rc ;
      error :
         goto done ;
      }
   protected:
      // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMKEYGEN__GETKEYS, "$_ixmKeyGenerator::_getKeys" )
      INT32 _getKeys( ossPoolVector<const CHAR *> &fieldNames,
                      const BSONObj &obj,
                      BOOLEAN isKeepKeyName,
                      BSONObjSet &keys,
                      BSONElement *arrEle,
                      BOOLEAN ignoreUndefined = FALSE ) const
      {
         INT32 rc = SDB_OK ;
         PD_TRACE_ENTRY ( SDB__IXMKEYGEN__GETKEYS );
#define IXM_DEFAULT_FIELD_NUM 4
         BSONElement eleOnStack[IXM_DEFAULT_FIELD_NUM] ;
         BSONElement *keyEles = NULL ;
         const CHAR *arrEleName = NULL ;
         UINT32 arrElePos = -1 ;
         UINT32 eooNum = 0 ;

         if ( IXM_DEFAULT_FIELD_NUM < fieldNames.size() )
         {
            keyEles = new(std::nothrow) BSONElement[fieldNames.size()] ;
            if ( NULL == keyEles )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
               rc = SDB_OOM ;
               goto error ;
            }
         }
         else
         {
            keyEles = ( BSONElement* )eleOnStack ;
         }

         for ( UINT32 i = 0; i < fieldNames.size(); i++ )
         {
            const CHAR *name = fieldNames.at( i ) ;
            SDB_ASSERT( '\0' != name[0], "can not be empty" ) ;
            BSONElement &e = keyEles[i] ;
            e = obj.getFieldDottedOrArray( name ) ;
            if ( e.eoo() )
            {
               ++eooNum ;
               continue ;
            }
            else if ( Array == e.type() )
            {
               if ( !arrEle->eoo() )
               {
                  PD_LOG( PDERROR, "At most one array can be in the key:",
                          arrEle->fieldName(), e.fieldName() ) ;
                  rc = SDB_IXM_MULTIPLE_ARRAY ;
                  goto error ;
               }
               else
               {
                  *arrEle = e ;
                  arrEleName = name ;
                  arrElePos = i ;
               }
            }
            else
            {
               continue ;
            }
         }

         if ( fieldNames.size() == eooNum )
         {
            rc = SDB_OK ;
            goto done ;
         }
         else if ( !arrEle->eoo() )
         {
            rc = _genKeyWithArrayEle( keyEles, fieldNames,
                                      arrEle,
                                      arrEleName, arrElePos,
                                      isKeepKeyName, ignoreUndefined,
                                      keys ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to gen keys with array element:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            INT32 arrNameLen = -1 ;
            if ( NULL != arrEleName )
            {
               const CHAR *originalName = fieldNames.at( arrElePos ) ;
               CHAR* subname = (CHAR*)arrEleName ;

               if ( '\0' != *subname )
               {
                  while ( '.' != *subname && subname > originalName )
                  {
                     subname-- ;
                  }
               }

               // cut off key name from array name
               arrNameLen = subname - originalName;
            }

            rc = _genKeyWithNormalEle( keyEles, fieldNames,
                                       isKeepKeyName, ignoreUndefined,
                                       keys, arrElePos, arrNameLen ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to gen keys with normal element:%d", rc ) ;
               goto error ;
            }
         }
      done:
         if ( IXM_DEFAULT_FIELD_NUM < fieldNames.size() &&
              NULL != keyEles )
         {
            delete []keyEles ;
         }
         PD_TRACE_EXITRC ( SDB__IXMKEYGEN__GETKEYS, rc );
         return rc ;
      error:
         goto done ;
      }

      // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMKEYGEN__GENKEYSWITHARRELE, "$_ixmKeyGenerator::_genKeyWithArrayEle" )
      INT32 _genKeyWithArrayEle( BSONElement *keyEles,
                                 ossPoolVector<const CHAR *> &fieldNames,
                                 const BSONElement *arrElement,
                                 const CHAR *arrEleName,
                                 UINT32 arrElePos,
                                 BOOLEAN isKeepKeyName,
                                 BOOLEAN ignoreUndefined,
                                 BSONObjSet &keys ) const
      {
         PD_TRACE_ENTRY ( SDB__IXMKEYGEN__GENKEYSWITHARRELE );
         INT32 rc = SDB_OK ;
         BSONObj arrObj = arrElement->embeddedObject() ;
         /// the element must be an empty array key when it is a empty array
         if ( arrObj.firstElement().eoo() )
         {
            keyEles[arrElePos] = *arrElement ;
            rc = _genKeyWithNormalEle( keyEles, fieldNames,
                                       isKeepKeyName, ignoreUndefined,
                                       keys ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         /// hit the end of name.
         if ( '\0' == *arrEleName )
         {
            BSONObjIterator itr( arrObj ) ;
            BSONElement &e = keyEles[arrElePos] ;
            while ( itr.more() )
            {
               e = itr.next() ;
               rc = _genKeyWithNormalEle( keyEles, fieldNames,
                                          isKeepKeyName, ignoreUndefined,
                                          keys ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
         }
         else
         {
            BSONObjIterator itr( arrObj ) ;
            while ( itr.more() )
            {
               const CHAR *dottedName = arrEleName ;
               BSONElement next = itr.next() ;
               if ( Object == next.type() )
               {
                  BSONElement e =
                     next.embeddedObject()
                     .getFieldDottedOrArray( dottedName ) ;
                  if ( Array == e.type() )
                  {
                     rc = _genKeyWithArrayEle(keyEles, fieldNames,
                                              &e, dottedName, arrElePos,
                                              isKeepKeyName, ignoreUndefined,
                                              keys) ;
                     if ( SDB_OK != rc )
                     {
                        goto error ;
                     }
                     else
                     {
                        continue ;
                     }
                  }
                  else
                  {
                     keyEles[arrElePos] = e ;
                  }
               }
               else
               {
                  keyEles[arrElePos] = BSONElement() ;
               }

               rc = _genKeyWithNormalEle( keyEles, fieldNames,
                                          isKeepKeyName, ignoreUndefined,
                                          keys ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
         }
      done:
         PD_TRACE_EXITRC( SDB__IXMKEYGEN__GENKEYSWITHARRELE, rc ) ;
         return rc ;
      error:
         goto done ;
      }

      // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMKEYGEN__GENKEYSWITHNORMALELE, "$_ixmKeyGenerator::_genKeyWithNormalEle" )
      INT32 _genKeyWithNormalEle( BSONElement *keyELes,
                                  ossPoolVector<const CHAR *> &fieldNames,
                                  BOOLEAN isKeepKeyName,
                                  BOOLEAN ignoreUndefined,
                                  BSONObjSet &keys,
                                  INT32 arrElePos = -1,
                                  INT32 arrNameLen = -1 ) const
      {
         PD_TRACE_ENTRY ( SDB__IXMKEYGEN__GENKEYSWITHNORMALELE );
         INT32 rc = SDB_OK ;
         UINT32 eleNum = fieldNames.size() ;
         BSONObjBuilder builder ;
         BSONObj obj ;
         for ( UINT32 i = 0; i < eleNum; i++ )
         {
            BSONElement &e = keyELes[i] ;
            ossPoolString keyName ;
            if ( isKeepKeyName )
            {
               if ( (INT32)i == arrElePos )
               {
                  SDB_ASSERT( arrNameLen >= 0, "invalid arrNameLen" ) ;
                  keyName = ossPoolString( fieldNames[i], arrNameLen ) ;
               }
               else
               {
                  keyName = fieldNames[i] ;
               }
            }
            if ( e.eoo() )
            {
               if ( !ignoreUndefined )
               {
                  builder.appendAs( gUndefinedElt, keyName.c_str() ) ;
               }
            }
            else
            {
               builder.appendAs( e, keyName.c_str() ) ;
            }
         }

         obj = builder.obj() ;
         if ( !obj.isEmpty() )
         {
            keys.insert( obj ) ;
         }
         PD_TRACE_EXITRC ( SDB__IXMKEYGEN__GENKEYSWITHNORMALELE, rc );
         return rc ;
      }
   } ;
   typedef class _ixmKeyGenerator ixmKeyGenerator ;

   // return True if there are at least one element match the name
   static BOOLEAN anyElementNamesMatch( const BSONObj& a , const BSONObj& b )
   {
      BSONObjIterator x(a);
      while ( x.more() )
      {
         BSONElement e = x.next();
         BSONObjIterator y(b);
         while ( y.more() )
         {
            BSONElement f = y.next();
            FieldCompareResult res = compareDottedFieldNames( e.fieldName(),
                                                              f.fieldName()
                                                            ) ;
            if ( res == SAME || res == LEFT_SUBFIELD || res == RIGHT_SUBFIELD )
               return TRUE;
         }
      }
      return FALSE;
   }

   // create key generator from index control block
   _ixmIndexKeyGen::_ixmIndexKeyGen ( const _ixmIndexCB *indexCB,
                                      IXM_KEYGEN_TYPE genType )
   {
      SDB_ASSERT ( indexCB, "details can't be NULL" ) ;
      _keyPattern = indexCB->keyPattern() ;
      // whole _infoObj
      _info = indexCB->_infoObj ;
      _type = indexCB->getIndexType() ;
      _keyGenType = genType ;
      //_indexCB = indexCB ;
      _init() ;
   }
   // create key generator from key
   _ixmIndexKeyGen::_ixmIndexKeyGen ( const BSONObj &keyDef,
                                      IXM_KEYGEN_TYPE genType )
   {
      _keyPattern = keyDef.copy () ;
      _type = IXM_EXTENT_TYPE_NONE ;
      _keyGenType = genType ;
      _init () ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__IXMINXKEYGEN__INIT, "_ixmIndexKeyGen::_init" )
   void _ixmIndexKeyGen::_init()
   {
      PD_TRACE_ENTRY ( SDB__IXMINXKEYGEN__INIT );
      _nFields = _keyPattern.nFields () ;
      INT32 fieldNum = 0 ;
      {
         BSONObjIterator i(_keyPattern) ;
         while ( i.more())
         {
            BSONElement e = i.next() ;
            _fieldNames.push_back(e.fieldName()) ;
            _fixedElements.push_back(BSONElement()) ;
            ++fieldNum ;
         }
         _undefinedKey = ixmGetUndefineKeyObj( fieldNum ) ;
      }
      PD_TRACE_EXIT ( SDB__IXMINXKEYGEN__INIT );
   }

   INT32 _ixmIndexKeyGen::getKeys ( const BSONObj &obj, BSONObjSet &keys,
                                    BSONElement *pArrEle,
                                    BOOLEAN isKeepKeyName,
                                    BOOLEAN ignoreUndefined ) const
   {
      ixmKeyGenerator g (this) ;
      if ( pArrEle )
      {
         *pArrEle = BSONElement() ;
      }
      return g.getKeys ( obj, isKeepKeyName, keys,
                         pArrEle, ignoreUndefined ) ;
   }

   IndexSuitability _ixmIndexKeyGen::suitability( const BSONObj &query ,
                                                  const BSONObj &order ) const
   {
      return _suitability( query , order );
   }

   IndexSuitability _ixmIndexKeyGen::_suitability( const BSONObj& query ,
                                                   const BSONObj& order ) const
   {
       // TODO: optimize
       if ( anyElementNamesMatch( _keyPattern , query ) == 0 &&
            anyElementNamesMatch( _keyPattern , order ) == 0 )
          return USELESS;
       return HELPFUL;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_IXMINXKEYGEN, "ixmIndexKeyGen::reset" )
   INT32 _ixmIndexKeyGen::reset ( const BSONObj & info )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_IXMINXKEYGEN );
      _info = info ;
      try
      {
         _keyPattern = _info[IXM_KEY_FIELD].embeddedObjectUserCheck() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Unable to locate valid key in index: %s",
                  e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _keyPattern.isEmpty() )
      {
         PD_LOG ( PDERROR, "Empty key" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _init() ;
   done :
      PD_TRACE_EXITRC ( SDB_IXMINXKEYGEN, rc );
      return rc ;
   error :
      goto done ;
   }
   INT32 _ixmIndexKeyGen::reset ( const _ixmIndexCB *indexCB )
   {
      SDB_ASSERT ( indexCB, "details can't be NULL" ) ;
      //_indexCB = indexCB ;
      return reset ( indexCB->_infoObj ) ;
   }
   BSONElement _ixmIndexKeyGen::missingField() const
   {
      return gUndefinedElt ;
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
}
