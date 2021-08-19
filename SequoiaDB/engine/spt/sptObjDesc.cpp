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

   Source File Name = sptObjDesc.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/06/2016  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptObjDesc.hpp"
#include "ossUtil.hpp"
#include "sptCommon.hpp"
#include "pd.hpp"
#include "sptObject.hpp"
using namespace std ;
using namespace bson ;
namespace engine
{

   /*
      _sptObjFactory implement
   */
   _sptObjFactory::_sptObjFactory()
   {
   }

   _sptObjFactory::~_sptObjFactory()
   {
   }

   BOOLEAN _sptObjFactory::_isExist( const sptObjDesc *desc ) const
   {
      return -1 != _find( desc, _vecObjs ) ? TRUE : FALSE ;
   }

   INT32 _sptObjFactory::_find( const string &objName,
                                const SPT_VEC_OBJDESC &vecObjs ) const
   {
      UINT32 size = vecObjs.size() ;
      UINT32 i = 0 ;

      if ( objName.empty() )
      {
         return -1 ;
      }

      for ( ; i < size ; ++i )
      {
         if ( !vecObjs[ i ] )
         {
            continue ;
         }
         if ( 0 == ossStrcmp( vecObjs[ i ]->getJSClassName(),
                                   objName.c_str() ) )
         {
            /// find
            return ( INT32 )i ;
         }
      }

      return -1 ;
   }

   INT32 _sptObjFactory::_find( const sptObjDesc *desc,
                                const SPT_VEC_OBJDESC &vecObjs ) const
   {
      UINT32 size = vecObjs.size() ;
      UINT32 i = 0 ;

      for ( ; i < size ; ++i )
      {
         if ( !vecObjs[ i ] )
         {
            continue ;
         }
         if ( desc == vecObjs[ i ] )
         {
            /// find
            return ( INT32 )i ;
         }
      }

      return -1 ;
   }

   void _sptObjFactory::registerObj( const sptObjDesc *desc )
   {
      SDB_ASSERT( desc, "desc can't be NULL" ) ;
      SDB_ASSERT( !_isExist( desc ), "desc already exist" ) ;

      if ( !desc || _isExist( desc ) )
      {
         return ;
      }
      _vecObjs.push_back( desc ) ;
   }

   const sptObjDesc* _sptObjFactory::findObj( const string &objName ) const
   {
      INT32 pos = _find( objName, _vecObjs ) ;
      if ( -1 != pos )
      {
         return _vecObjs[ pos ] ;
      }
      return NULL ;
   }

   const sptObjDesc*
      _sptObjFactory::findObjBySpecialFieldName( const string &fieldName ) const
   {
      for( UINT32 index = 0; index < _vecObjs.size(); index++ )
      {
         if( _vecObjs[index]->getSpecialFieldName() == fieldName )
         {
            return _vecObjs[index] ;
         }
      }
      return NULL ;
   }

   INT32 _sptObjFactory::getObjDesc( JSContext *cx, JSObject *obj,
                                     BOOLEAN &isSpecialObj,
                                     const sptObjDesc **pDesc ) const
   {
      INT32 rc = SDB_OK ;
      const sptObjDesc *objDesc = NULL ;
      JSIdArray *properties = NULL ;
      string className = getClassName( cx, obj ) ;

      if( !className.empty() )
      {
         objDesc = this->findObj( className ) ;
         isSpecialObj = FALSE ;
      }
      else
      {
         jsid id ;
         jsval fieldName ;
         std::string name ;
         CHAR *pFieldName = NULL ;
         properties = JS_Enumerate( cx, obj ) ;
         if ( NULL == properties || 0 == properties->length )
         {
            // Object is normal js object
            goto done ;
         }
         /// get the first ele
         id = properties->vector[0] ;
         if ( !JS_IdToValue( cx, id, &fieldName ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         if( !JSVAL_IS_STRING( fieldName ) )
         {
            goto done ;
         }

         pFieldName = JS_EncodeString( cx, JSVAL_TO_STRING( fieldName ) ) ;
         if ( !pFieldName )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         name.assign( pFieldName ) ;
         /// free
         SAFE_JS_FREE( cx, pFieldName ) ;

         if ( name.length() <= 1 || SPT_SPE_OBJSTART != name.at(0) )
         {
            // Object is normal js object
            goto done ;
         }
         objDesc = this->findObjBySpecialFieldName( name ) ;
         isSpecialObj = TRUE ;
      }
   done:
      if( SDB_OK == rc )
      {
         *pDesc = objDesc ;
      }
      /// free
      if ( properties )
      {
         JS_DestroyIdArray( cx, properties ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   UINT32 _sptObjFactory::getObjDescs( SPT_VEC_OBJDESC &vecObjDesc ) const
   {
      vecObjDesc = _vecObjs ;
      return vecObjDesc.size() ;
   }

   BOOLEAN _sptObjFactory::isInstanceOf( JSContext *cx,
                                         JSObject *obj,
                                         const string &objName )
   {
      sptObjDesc *desc = ( sptObjDesc* )findObj( objName ) ;
      if ( desc )
      {
         return desc->isInstanceOf( cx, obj ) ;
      }
      return FALSE ;
   }

   string _sptObjFactory::getClassName( JSContext *cx, JSObject *obj ) const
   {
      jsval val = JSVAL_VOID ;
      string name ;

      if ( JS_GetProperty( cx, obj, SPT_OBJ_CNAME_PROPNAME, &val ) &&
           JSVAL_IS_STRING( val ) )
      {
         JSString *jsStr = JSVAL_TO_STRING( val ) ;
         CHAR* str = JS_EncodeString ( cx , jsStr ) ;
         if ( str )
         {
            name = str ;
            JS_free( cx, str ) ;
         }
      }

      return name ;
   }

   void _sptObjFactory::_getClassMemFuncNamesByNative( const string &className,
                                                       set<string> &setFuns,
                                                       BOOLEAN showHide )
   {
      const _sptObjDesc *desc = findObj( className ) ;
      while ( desc )
      {
         desc->getFuncMap().getMemberFuncNames( setFuns, showHide ) ;
         desc = desc->getParent() ;
      }
   }

   void _sptObjFactory::_getClassStaticFuncNamesByNative( const string &className,
                                                          set < string > &setFuns,
                                                          BOOLEAN showHide )
   {
      const _sptObjDesc *desc = findObj( className ) ;
      if ( desc )
      {
         desc->getFuncMap().getStaticFuncNames( setFuns, showHide ) ;
      }
   }

   void _sptObjFactory::getObjFuncNames( JSContext *cx,
                                         JSObject *obj,
                                         set< string > &setFuns,
                                         BOOLEAN showHide )
   {
      /// 1. to get native funcs
      string className = getClassName( cx, obj ) ;
      _getClassMemFuncNamesByNative( className, setFuns, showHide ) ;

      /// 2. to get self func2
      _getObjFuncNames( cx, obj, setFuns ) ;
      /// 3. get prototype's funcs
      JSObject *prototype = JS_GetPrototype( cx, obj ) ;
      while( prototype )
      {
         _getObjFuncNames( cx, prototype, setFuns ) ;
         prototype = JS_GetPrototype( cx, prototype ) ;
      }
   }

   void _sptObjFactory::getObjStaticFunNames( JSContext *cx,
                                              JSObject *obj,
                                              set < string > &setFuns,
                                              BOOLEAN showHide )
   {
      /// 1. to get native static funcs
      string className = getClassName( cx, obj ) ;
      _getClassStaticFuncNamesByNative( className, setFuns, showHide ) ;

      /// 2. to get constructor's static functions
      JSObject *constructor = JS_GetConstructor( cx, obj ) ;
      if ( constructor )
      {
         _getObjFuncNames( cx, constructor, setFuns ) ;
      }
   }

   void _sptObjFactory::getClassFuncNames( JSContext *cx,
                                           const string &className,
                                           set< string > &setFuns,
                                           BOOLEAN showHide )
   {
      JSObject *jsObj = _newObjectByName( cx, className ) ; ;
      if ( jsObj )
      {
         getObjFuncNames( cx, jsObj, setFuns, showHide ) ;
         JS_MaybeGC( cx ) ;
      }
   }

   void _sptObjFactory::getClassStaticFuncNames( JSContext *cx,
                                                 const string &className,
                                                 set < string > &setFuns,
                                                 BOOLEAN showHide )
   {
      JSObject *jsObj = _newObjectByName( cx, className ) ;
      if ( jsObj )
      {
         getObjStaticFunNames( cx, jsObj, setFuns, showHide ) ;
         JS_MaybeGC( cx ) ;
      }
   }

   void _sptObjFactory::getObjPropNames( JSContext *cx,
                                         JSObject *obj,
                                         set< string > &setProp )
   {
      /// first get self prop
      _getObjPropNames( cx, obj, setProp ) ;
      /// then get prototype's prop
      JSObject *prototype = NULL ;
      prototype = JS_GetPrototype( cx, obj ) ;
      while( prototype )
      {
         _getObjPropNames( cx, prototype, setProp ) ;
         prototype = JS_GetPrototype( cx, prototype ) ;
      }
   }

   void _sptObjFactory::getObjStaticPropNames( JSContext *cx,
                                               JSObject *obj,
                                               set < string > &setProp )
   {
      JSObject *constructor = JS_GetConstructor( cx, obj ) ;
      if ( constructor )
      {
         _getObjPropNames( cx, constructor, setProp ) ;
      }
   }

   void _sptObjFactory::getClassStaticPropNames( JSContext *cx,
                                                 const string &className,
                                                 set < string > &setProp )
   {
      JSObject *jsObj = _newObjectByName( cx, className ) ;
      if ( jsObj )
      {
         getObjStaticPropNames( cx, jsObj, setProp ) ;
         JS_MaybeGC( cx ) ;
      }
   }

   void _sptObjFactory::_getObjFuncNames( JSContext *cx,
                                          JSObject *obj,
                                          set< string > &setFuns )
   {
      JSIdArray *properties = NULL ;
      properties = JS_Enumerate( cx, obj ) ;
      if ( !properties )
      {
         goto done ;
      }

      for ( jsint i = 0; i < properties->length; i++ )
      {
         jsid id = properties->vector[i] ;
         jsval fieldName = JSVAL_VOID ;
         jsval fieldValue = JSVAL_VOID ;

         if ( !JS_GetPropertyById( cx, obj, id, &fieldValue ) )
         {
            continue ;
         }
         if ( !JSVAL_IS_OBJECT( fieldValue ) )
         {
            continue ;
         }
         JSObject *objvalue = JSVAL_TO_OBJECT( fieldValue ) ;
         if ( !objvalue )
         {
            continue ;
         }
         if ( !JS_ObjectIsFunction( cx, objvalue ) )
         {
            continue ;
         }
         if ( !JS_IdToValue( cx, id, &fieldName ) )
         {
            continue ;
         }
         JSString *jsStr = JS_ValueToString( cx, fieldName ) ;
         if ( !jsStr )
         {
            continue ;
         }
         CHAR *str = JS_EncodeString( cx, jsStr ) ;
         if ( !str )
         {
            continue ;
         }
         setFuns.insert( str ) ;
         JS_free( cx, str ) ;
      }

      /// free
      JS_DestroyIdArray( cx, properties ) ;

   done:
      return ;
   }

   void _sptObjFactory::_getObjPropNames( JSContext *cx,
                                          JSObject *obj,
                                          set< string > &setProp )
   {
      JSIdArray *properties = NULL ;
      properties = JS_Enumerate( cx, obj ) ;
      if ( !properties )
      {
         goto done ;
      }

      for ( jsint i = 0; i < properties->length; i++ )
      {
         jsid id = properties->vector[i] ;
         jsval fieldName = JSVAL_VOID ;
         jsval fieldValue = JSVAL_VOID ;

         if ( !JS_GetPropertyById( cx, obj, id, &fieldValue ) )
         {
            continue ;
         }
         JSObject *objvalue = NULL ;
         if ( JSVAL_IS_OBJECT( fieldValue ) &&
              NULL != ( objvalue = JSVAL_TO_OBJECT( fieldValue ) ) &&
              JS_ObjectIsFunction( cx, objvalue ) )
         {
            continue ;
         }
         if ( !JS_IdToValue( cx, id, &fieldName ) )
         {
            continue ;
         }
         JSString *jsStr = JS_ValueToString( cx, fieldName ) ;
         if ( !jsStr )
         {
            continue ;
         }
         CHAR *str = JS_EncodeString( cx, jsStr ) ;
         if ( !str )
         {
            continue ;
         }
         setProp.insert( str ) ;
         JS_free( cx, str ) ;
      }
      /// free
      JS_DestroyIdArray( cx, properties ) ;

   done:
      return ;
   }

   void _sptObjFactory::_sortAndAssert( SPT_VEC_OBJDESC &vecObj,
                                        const sptObjDesc *desc )
   {
      /// no parent
      if ( desc->isIgnoredParent() )
      {
         vecObj.push_back( desc ) ;
      }
      /// parent existed
      else if ( -1 != _find( desc->getParent(), vecObj ) )
      {
         vecObj.push_back( desc ) ;
      }
      /// parent is not existed
      else
      {
         INT32 tmpPos = _find( desc->getParent(), _vecObjs ) ;
         if ( -1 == tmpPos )
         {
            SDB_ASSERT( FALSE, "Parent is not existed" ) ;
         }
         else
         {
            const sptObjDesc *parent = _vecObjs[ tmpPos ] ;
            _vecObjs[ tmpPos ] = NULL ;
            _sortAndAssert( vecObj, parent ) ;
         }
         vecObj.push_back( desc ) ;
      }
   }

   void _sptObjFactory::sortAndAssert()
   {
      SPT_VEC_OBJDESC vecTmp ;
      UINT32 size = _vecObjs.size() ;
      UINT32 i = 0 ;
      const sptObjDesc *desc = NULL ;

      for ( ; i < size ; ++i )
      {
         desc = _vecObjs[ i ] ;

         if ( !desc )
         {
            continue ;
         }

         _sortAndAssert( vecTmp, desc ) ;
      }

      _vecObjs = vecTmp ;
   }

   void _sptObjFactory::getClassNames( set< string > &setClass,
                                       BOOLEAN showHide )
   {
      const sptObjDesc *desc = NULL ;
      UINT32 i = 0 ;
      UINT32 size = _vecObjs.size() ;

      for ( ; i < size ; ++i )
      {
         desc = _vecObjs[ i ] ;

         if ( !desc || desc->isGlobal() )
         {
            continue ;
         }
         if ( showHide || !desc->isHide() )
         {
            setClass.insert( desc->getJSClassName() ) ;
         }
      }
   }

   JSObject* _sptObjFactory::_newObjectByName( JSContext *cx,
                                               const string &className )
   {
      JSObject *jsObj = NULL ;

      if ( className.empty() )
      {
         jsObj = JS_GetGlobalObject( cx ) ;
      }
      else
      {
         const _sptObjDesc *desc = findObj( className ) ;
         if ( desc )
         {
            jsObj = JS_NewObject ( cx, (JSClass *)(desc->getClassDef()),
                                   0 , 0 ) ;
         }
         if ( jsObj )
         {
            jsval val = JSVAL_VOID ;
            JSString *jsstr = JS_NewStringCopyN( cx, className.c_str(),
                                                 className.length() ) ;
            if ( jsstr )
            {
               val = STRING_TO_JSVAL( jsstr ) ;
               JS_DefineProperty( cx, jsObj, SPT_OBJ_CNAME_PROPNAME,
                                  val, 0, 0,
                                  SPT_PROP_READONLY|SPT_PROP_PERMANENT ) ;
            }
         }
      }

      return jsObj ;
   }

   INT32 _sptObjDesc::cvtToBSON( const CHAR* key, const sptObject &value,
                                 BOOLEAN isSpecialObj, BSONObjBuilder& builder,
                                 string &errMsg ) const
   {
      INT32 rc = SDB_OK ;
      if( NULL == _cvtToBSONFunc )
      {
         BSONObj tmpObj ;
         rc = value.toBSON( tmpObj ) ;
         if( SDB_OK != rc )
         {
            errMsg = "Failed to convert js obj to bson obj" ;
            goto error ;
         }
         builder.append( key, tmpObj ) ;
      }
      else
      {
         rc = _cvtToBSONFunc( key, value, isSpecialObj, builder, errMsg ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptObjDesc::setCVTToBSONFunc( CVT_TO_BSON_FUNC pFunc )
   {
      if( NULL == pFunc )
      {
         _cvtFlags &= ~SPT_CVT_FLAGS_OBJ_TO_BSON ;
      }
      else
      {
         _cvtFlags |= SPT_CVT_FLAGS_OBJ_TO_BSON ;
      }
      _cvtToBSONFunc = pFunc ;
   }

   BOOLEAN _sptObjDesc::hasCVTToBSONFunc() const
   {
      return _cvtToBSONFunc ? TRUE : FALSE ;
   }

   INT32 _sptObjDesc::cvtToBool( const sptObject &obj, BOOLEAN isSpecialObj,
                                 BOOLEAN &retVal ) const
   {
      INT32 rc = SDB_OK ;
      if( !( _cvtFlags & SPT_CVT_FLAGS_OBJ_TO_BOOL ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( NULL == _cvtToBoolFunc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _cvtToBoolFunc( obj, isSpecialObj, retVal ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptObjDesc::setCVTToBoolFunc( CVT_TO_BOOL_FUNC pFunc )
   {
      if( NULL == pFunc )
      {
         _cvtFlags &= ~SPT_CVT_FLAGS_OBJ_TO_BOOL ;
      }
      else
      {
         _cvtFlags |= SPT_CVT_FLAGS_OBJ_TO_BOOL ;
      }
      _cvtToBoolFunc = pFunc ;
   }

   INT32 _sptObjDesc::cvtToDouble( const sptObject &obj, BOOLEAN isSpecialObj,
                                   FLOAT64 &retVal ) const
   {
      INT32 rc = SDB_OK ;
      if( !( _cvtFlags & SPT_CVT_FLAGS_OBJ_TO_DOUBLE ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( NULL == _cvtToDoubleFunc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _cvtToDoubleFunc( obj, isSpecialObj, retVal ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptObjDesc::setCVTToDoubleFunc( CVT_TO_DOUBLE_FUNC pFunc )
   {
      if( NULL == pFunc )
      {
         _cvtFlags &= ~SPT_CVT_FLAGS_OBJ_TO_DOUBLE ;
      }
      else
      {
         _cvtFlags |= SPT_CVT_FLAGS_OBJ_TO_DOUBLE ;
      }
      _cvtToDoubleFunc = pFunc ;
   }

   INT32 _sptObjDesc::cvtToInt( const sptObject &obj, BOOLEAN isSpecialObj,
                                INT32 &retVal ) const
   {
      INT32 rc = SDB_OK ;
      if( !( _cvtFlags & SPT_CVT_FLAGS_OBJ_TO_INT ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( NULL == _cvtToIntFunc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _cvtToIntFunc( obj, isSpecialObj, retVal ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptObjDesc::setCVTToIntFunc( CVT_TO_INT_FUNC pFunc )
   {
      if( NULL == pFunc )
      {
         _cvtFlags &= ~SPT_CVT_FLAGS_OBJ_TO_INT ;
      }
      else
      {
         _cvtFlags |= SPT_CVT_FLAGS_OBJ_TO_INT ;
      }
      _cvtToIntFunc = pFunc ;
   }

   INT32 _sptObjDesc::cvtToString( const sptObject &obj, BOOLEAN isSpecialObj,
                                   string &retVal ) const
   {
      INT32 rc = SDB_OK ;
      if( !( _cvtFlags & SPT_CVT_FLAGS_OBJ_TO_STRING ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if( NULL == _cvtToStringFunc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _cvtToStringFunc( obj, isSpecialObj, retVal ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _sptObjDesc::setCVTToStringFunc( CVT_TO_STRING_FUNC pFunc )
   {
      if( NULL == pFunc )
      {
         _cvtFlags &= ~SPT_CVT_FLAGS_OBJ_TO_STRING ;
      }
      else
      {
         _cvtFlags |= SPT_CVT_FLAGS_OBJ_TO_STRING ;
      }
      _cvtToStringFunc = pFunc ;
   }

   INT32 _sptObjDesc::fmpToBSON( const sptObject &value, BSONObj &retObj,
                                 string &errMsg ) const
   {
      INT32 rc = SDB_OK ;
      if( NULL == _fmpToBSONFunc )
      {
         goto done ;
      }
      rc = _fmpToBSONFunc( value, retObj, errMsg ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failde to convert js obj to BSON" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptObjDesc::fmpToCursor( const sptObject &value,
                                   sdbclient::_sdbCursor** pCursor,
                                   string &errMsg ) const
   {
      INT32 rc = SDB_OK ;
      if( NULL == _fmpToCursorFunc )
      {
         goto done ;
      }
      rc = _fmpToCursorFunc( value, pCursor, errMsg ) ;
      if( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failde to convert js obj to cursor" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptObjDesc::bsonToJSObj( sdbclient::sdb &db, const BSONObj &data,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) const
   {
      INT32 rc = SDB_OK ;
      if( NULL == _bsonToJSObjFunc )
      {
         goto done ;
      }
      else
      {
         rc = _bsonToJSObjFunc( db, data, rval, detail ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   sptObjFactory* sptGetObjFactory()
   {
      static sptObjFactory s_objFactory ;
      return &s_objFactory ;
   }

   BOOLEAN sptIsInstanceOf( JSContext *cx,
                            JSObject *obj,
                            const string &objName )
   {
      return sptGetObjFactory()->isInstanceOf( cx, obj, objName ) ;
   }

   /*
      _sptObjAssist implement
   */
   _sptObjAssist::_sptObjAssist( const sptObjDesc *desc )
   {
      sptGetObjFactory()->registerObj( desc ) ;
   }

   _sptObjAssist::~_sptObjAssist()
   {
   }

}


