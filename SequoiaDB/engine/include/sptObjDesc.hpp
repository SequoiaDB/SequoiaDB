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

using namespace std ;

namespace engine
{
   /*
      _sptObjDesc define
   */
   class _sptObjDesc : public SDBObject
   {
   public:
      _sptObjDesc()
      :_init(FALSE), _prototypeDef( NULL ), _parent( NULL ), _isHide( FALSE )
      {}

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
      const JSObject *getPrototypeDef() const
      {
         return _prototypeDef ;
      }

      void setClassName( const CHAR *name )
      {
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

      void setClassDef( const JSClass &def )
      {
         _classDef = def ;
         _init = TRUE ;
      }
      void setClassPrototype( const JSObject *proto )
      {
         _prototypeDef = proto ;
      }

      BOOLEAN isIgnoredName() const
      {
         return _jsClassName.empty() ;
      }
      BOOLEAN isIgnoredParent() const
      {
         return !_parent ? TRUE : FALSE ;
      }
      BOOLEAN isHide() const
      {
         return _isHide ;
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

   protected:
      std::string _jsClassName ;
      _sptFuncMap _funcMap ;
      BOOLEAN     _init ;
      JSClass     _classDef ;
      const JSObject*   _prototypeDef ;
      const _sptObjDesc *_parent ;
      BOOLEAN     _isHide ;
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

         UINT32         getObjDescs( SPT_VEC_OBJDESC &vecObjDesc ) const ;
         BOOLEAN        isInstanceOf( JSContext *cx,
                                      JSObject *obj,
                                      const string &objName ) ;
         string         getClassName( JSContext *cx, JSObject *obj ) ;


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

