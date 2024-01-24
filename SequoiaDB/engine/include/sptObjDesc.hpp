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

   Source File Name = sptObjDesc.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_OBJDESC_HPP_
#define SPT_OBJDESC_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptFuncMap.hpp"
#include "jsapi.h"
#include <string>
#include <vector>
#include "client.hpp"
#include "pd.hpp"
using namespace std ;

namespace engine
{

   class _sptReturnVal ;
   class sptObject ;

   #define SPT_CVT_FLAGS_OBJ_TO_BOOL    0x1
   #define SPT_CVT_FLAGS_OBJ_TO_INT     0x2
   #define SPT_CVT_FLAGS_OBJ_TO_DOUBLE  0x4
   #define SPT_CVT_FLAGS_OBJ_TO_STRING  0x8
   #define SPT_CVT_FLAGS_OBJ_TO_BSON    0x10

   // Function type define
   typedef INT32 (*CVT_TO_BSON_FUNC)( const CHAR* key, const sptObject &value,
                                      BOOLEAN isSpecialObj,
                                      bson::BSONObjBuilder& builder,
                                      string &errMsg ) ;
   typedef INT32 (*CVT_TO_BOOL_FUNC)( const sptObject &obj, BOOLEAN isSpecialObj,
                                      BOOLEAN &retVal ) ;
   typedef INT32 (*CVT_TO_INT_FUNC)( const sptObject &obj, BOOLEAN isSpecialObj,
                                     INT32 &retVal ) ;
   typedef INT32 (*CVT_TO_DOUBLE_FUNC)( const sptObject &obj,
                                        BOOLEAN isSpecialObj,
                                        FLOAT64 &retVal ) ;
   typedef INT32 (*CVT_TO_STRING_FUNC)( const sptObject &obj,
                                        BOOLEAN isSpecialObj,
                                        string &retVal ) ;
   typedef INT32 (*FMP_TO_BSON_FUNC)( const sptObject &value,
                                      bson::BSONObj &retObj, string &errMsg ) ;
   typedef INT32 (*FMP_TO_CURSOR_FUNC) ( const sptObject &value,
                                         sdbclient::_sdbCursor** pCursor,
                                         string &errMsg ) ;

   typedef INT32 (*BSON_TO_JSOBJ_FUNC) ( sdbclient::sdb &db,
                                         const bson::BSONObj &data,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail ) ;
   /*
      _sptObjDesc define
   */
   class _sptObjDesc : public SDBObject
   {
   public:
      _sptObjDesc()
      :_init(FALSE), _parent( NULL ),
       _isHide( FALSE ), _isGlobal( FALSE ), _cvtFlags( 0 )
      {
         _cvtToBSONFunc = NULL , 
         _cvtToBoolFunc = NULL ;
         _cvtToIntFunc = NULL ;
         _cvtToDoubleFunc = NULL ;
         _cvtToStringFunc = NULL ;
         _fmpToBSONFunc = NULL ;
         _fmpToCursorFunc = NULL ;
         _bsonToJSObjFunc = NULL ;
      }

      virtual ~_sptObjDesc(){}
   public:
      const CHAR *getJSClassName() const
      {
         return _jsClassName.c_str() ;
      }
      const _sptObjDesc *getParent() const
      {
         return _parent ;
      }
      const _sptFuncMap &getFuncMap()const
      {
         return _funcMap ;
      }

      const JSClass *getClassDef() const
      {
         return _init ? &_classDef : NULL ;
      }

      void setClassName( const CHAR *name )
      {
         SDB_ASSERT( name && *name, "name can't be empty" ) ;
         _jsClassName.assign( name ) ;
      }
      void setParent( const _sptObjDesc *parent )
      {
         _parent = parent ;
      }
      void setHide( BOOLEAN hide )
      {
         _isHide = hide ;
      }
      void setGlobal( BOOLEAN isGlobal )
      {
         _isGlobal = isGlobal ;
      }

      void setClassDef( const JSClass &def )
      {
         _classDef = def ;
         _init = TRUE ;
      }

      BOOLEAN isIgnoredParent() const
      {
         return !_parent ? TRUE : FALSE ;
      }
      BOOLEAN isHide() const
      {
         return _isHide ;
      }
      BOOLEAN isGlobal() const
      {
         return _isGlobal ;
      }

      void  ignoredName()
      {
         _jsClassName = "" ;
      }
      void ignoredParent()
      {
         _parent = NULL ;
      }

      BOOLEAN isInstanceOf( JSContext *cx, JSObject *obj )
      {
         if ( !_init )
         {
            return FALSE ;
         }
         return JS_InstanceOf( cx, obj, &_classDef, NULL ) ;
      }

      std::string getSpecialFieldName() const
      {
         return _specialFieldName ;
      }

      void setSpecialFieldName( std::string fieldName )
      {
         _specialFieldName = fieldName ;
      }

      INT32 cvtToBSON( const CHAR* key,
                       const sptObject &value,
                       BOOLEAN isSpecialObj,
                       bson::BSONObjBuilder& builder,
                       string &errMsg ) const ;

      void setCVTToBSONFunc( CVT_TO_BSON_FUNC pFunc ) ;

      BOOLEAN hasCVTToBSONFunc() const ;

      INT32 cvtToBool( const sptObject &obj,
                       BOOLEAN isSpecialObj,
                       BOOLEAN &retVal ) const ;

      void setCVTToBoolFunc( CVT_TO_BOOL_FUNC pFunc ) ;

      INT32 cvtToDouble( const sptObject &obj,
                         BOOLEAN isSpecialObj,
                         FLOAT64 &retVal ) const ;

      void setCVTToDoubleFunc( CVT_TO_DOUBLE_FUNC pFunc ) ;

      INT32 cvtToInt( const sptObject &obj,
                      BOOLEAN isSpecialObj,
                      INT32 &retVal ) const ;

      void setCVTToIntFunc( CVT_TO_INT_FUNC pFunc ) ;

      INT32 cvtToString( const sptObject &obj,
                         BOOLEAN isSpecialObj,
                         string &retVal ) const ;

      void setCVTToStringFunc( CVT_TO_STRING_FUNC pFunc ) ;

      INT32 fmpToBSON( const sptObject &value,
                       bson::BSONObj &retObj,
                       string &errMsg ) const ;

      void setFMPToBSONFunc( FMP_TO_BSON_FUNC pFunc )
      {
         _fmpToBSONFunc = pFunc ;
      }

      INT32 fmpToCursor( const sptObject &value,
                         sdbclient::_sdbCursor** pCursor,
                         string &errMsg ) const ;

      void setFMPToCursorFunc( FMP_TO_CURSOR_FUNC pFunc )
      {
         _fmpToCursorFunc = pFunc ;
      }

      INT32 bsonToJSObj( sdbclient::sdb &db,
                         const bson::BSONObj &data,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) const ;

      void setBSONToJSObjFunc( BSON_TO_JSOBJ_FUNC pFunc )
      {
         _bsonToJSObjFunc = pFunc ;
      }

   protected:
      std::string _jsClassName ;
      _sptFuncMap _funcMap ;
      BOOLEAN     _init ;
      JSClass     _classDef ;
      const _sptObjDesc *_parent ;
      BOOLEAN     _isHide ;
      BOOLEAN     _isGlobal ;
      // toBSON module
      UINT32 _cvtFlags ;
      std::string _specialFieldName ;
      CVT_TO_BSON_FUNC _cvtToBSONFunc ;
      CVT_TO_BOOL_FUNC _cvtToBoolFunc ;
      CVT_TO_INT_FUNC _cvtToIntFunc ;
      CVT_TO_DOUBLE_FUNC _cvtToDoubleFunc ;
      CVT_TO_STRING_FUNC _cvtToStringFunc ;
      FMP_TO_BSON_FUNC _fmpToBSONFunc ;
      FMP_TO_CURSOR_FUNC _fmpToCursorFunc ;
      BSON_TO_JSOBJ_FUNC _bsonToJSObjFunc ;
   } ;
   typedef class _sptObjDesc sptObjDesc ;

   /*
      _sptObjFactory define
   */
   typedef vector< const sptObjDesc* >        SPT_VEC_OBJDESC ;

   class _sptObjFactory : public SDBObject
   {
      public:
         _sptObjFactory() ;
         ~_sptObjFactory() ;

         void                 sortAndAssert() ;

      public:
         void                 registerObj( const sptObjDesc *desc ) ;
         const sptObjDesc*    findObj( const string &objName ) const ;
         INT32                getObjDesc( JSContext *cx, JSObject *obj,
                                          BOOLEAN &isSpecialObj,
                                          const sptObjDesc **pDesc ) const ;
         const sptObjDesc*    findObjBySpecialFieldName( const string &fieldName ) const ;
         UINT32         getObjDescs( SPT_VEC_OBJDESC &vecObjDesc ) const ;
         BOOLEAN        isInstanceOf( JSContext *cx,
                                      JSObject *obj,
                                      const string &objName ) ;
         string         getClassName( JSContext *cx, JSObject *obj ) const ;


         /*
            Get the Object's all functions, include prototype's function
            implement in js code.
         */
         void           getObjFuncNames( JSContext *cx,
                                         JSObject *obj,
                                         set<string> &setFuns,
                                         BOOLEAN showHide = FALSE ) ;

         /*
            Get the Object's all static functions in its constructor.
         */
         void           getObjStaticFunNames( JSContext *cx,
                                              JSObject *obj,
                                              set<string> &setFuns,
                                              BOOLEAN showHide = FALSE ) ;

         void           getClassFuncNames( JSContext *cx,
                                           const string &className,
                                           set<string> &setFuns,
                                           BOOLEAN showHide = FALSE ) ;

         void           getClassStaticFuncNames( JSContext *cx,
                                                 const string &className,
                                                 set<string> &setFuns,
                                                 BOOLEAN showHide = FALSE ) ;

         /*
            Get the Object's all properties, include prototype's prop
            implement in js code.
         */
         void           getObjPropNames( JSContext *cx,
                                         JSObject *obj,
                                         set<string> &setProp ) ;
         /*
            Get the Object's all static properties in its constructor
         */
         void           getObjStaticPropNames( JSContext *cx,
                                               JSObject *obj,
                                               set<string> &setProp ) ;
         /*
            Get the static properties in its constructor by className
         */
         void           getClassStaticPropNames( JSContext *cx,
                                                 const string &className,
                                                 set<string> &setProp ) ;

         void           getClassNames( set<string> &setClass,
                                       BOOLEAN showHide = FALSE ) ;

      protected:
         BOOLEAN        _isExist( const sptObjDesc *desc ) const ;
         INT32          _find( const string &objName,
                               const SPT_VEC_OBJDESC &vecObjs ) const ;
         INT32          _find( const sptObjDesc *desc,
                               const SPT_VEC_OBJDESC &vecObjs ) const ;

         void           _sortAndAssert( SPT_VEC_OBJDESC &vecObj,
                                        const sptObjDesc *desc ) ;

         /*
            Only get the object's funcs
         */
         void           _getObjFuncNames( JSContext *cx,
                                          JSObject *obj,
                                          set<string> &setFuns ) ;

         /*
            Only get the object's propertys
         */
         void           _getObjPropNames( JSContext *cx,
                                          JSObject *obj,
                                          set<string> &setProp ) ;

         /*
            Get the class Native's mem/static Functions with parent
         */
         void           _getClassMemFuncNamesByNative( const string &className,
                                                       set<string> &setFuns,
                                                       BOOLEAN showHide = FALSE ) ;
         void           _getClassStaticFuncNamesByNative( const string &className,
                                                          set<string> &setFuns,
                                                          BOOLEAN showHide = FALSE ) ;

         JSObject*      _newObjectByName( JSContext *cx,
                                          const string &className ) ;

      private:
         SPT_VEC_OBJDESC         _vecObjs ;

   } ;
   typedef _sptObjFactory sptObjFactory ;

   /*
      Function define
   */
   sptObjFactory* sptGetObjFactory() ;

   BOOLEAN        sptIsInstanceOf( JSContext *cx,
                                   JSObject *obj,
                                   const string &objName ) ;

   /*
      _sptObjAssist define
   */
   class _sptObjAssist : public SDBObject
   {
      public:
         _sptObjAssist( const sptObjDesc *desc ) ;
         ~_sptObjAssist() ;
   } ;
   typedef _sptObjAssist sptObjAssist ;

}

#endif // SPT_OBJDESC_HPP_

