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

   Source File Name = sptScope.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_SCOPE_HPP_
#define SPT_SCOPE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptSPDef.hpp"
#include "sptSPVal.hpp"
#include "../bson/bson.hpp"
#include <list>
#include <string>

namespace engine
{
   class _sptObjDesc ;

   enum SPT_SCOPE_TYPE
   {
      SPT_SCOPE_TYPE_SP = 0,
      SPT_SCOPE_TYPE_V8 = 1,
   } ;

   #define SPT_OBJ_MASK_STANDARD                0x0001
   #define SPT_OBJ_MASK_USR                     0x0002
   #define SPT_OBJ_MASK_INNER_JS                0x0004

   #define SPT_OBJ_MASK_ALL                     0xFFFF

   /*
      _sptResultVal define
   */
   class _sptResultVal : public SDBObject
   {
      public:
         _sptResultVal() ;
         virtual ~_sptResultVal() ;

         virtual _sptResultVal*  copy() const = 0 ;

         virtual const sptSPVal* getVal() const = 0 ;
         virtual const void*     rawPtr() const = 0 ;
         virtual bson::BSONObj   toBSON() const = 0 ;

         BOOLEAN                 hasError() const ;
         const CHAR*             getErrrInfo() const ;
         void                    setError( const std::string &err ) ;

      protected:
         std::string             _errStr ;

   } ;
   typedef _sptResultVal sptResultVal ;

   /*
      _sptScope define
   */
   class _sptScope : public SDBObject
   {
   public:
      _sptScope() ;
      virtual ~_sptScope() ;

      virtual SPT_SCOPE_TYPE getType() const = 0 ;

      UINT32         getLoadMask() const { return _loadMask ; }
      INT32          getLastError() const ;
      const CHAR*    getLastErrMsg() const ;
      bson::BSONObj  getLastErrObj() const ;

      // be used to implement import function
      void     pushJSFileNameToStack( const string &filename ) ;
      void     popJSFileNameFromStack() ;
      INT32    getStackSize() ;
      void     addJSFileNameToList( const string &filename ) ;
      void     clearJSFileNameList() ;
      BOOLEAN  isJSFileNameExistInStack( const string &filename ) ;
      BOOLEAN  isJSFileNameExistInList( const string &filename ) ;

      string   calcImportPath( const string &filename ) ;

   public:
      virtual INT32  start( UINT32 loadMask = SPT_OBJ_MASK_ALL ) = 0 ;

      virtual void   shutdown() = 0 ;

      virtual INT32  eval( const CHAR *code, UINT32 len,
                           const CHAR *filename,
                           UINT32 lineno,
                           INT32 flag, // SPT_EVAL_FLAG_NONE/SPT_EVAL_FLAG_PRINT
                           const sptResultVal **ppRval ) = 0 ;

      virtual void   getGlobalFunNames( set<string> &setFunc,
                                        BOOLEAN showHide = FALSE ) = 0 ;

      virtual void   getObjStaticFunNames( const string &objName,
                                           set<string> &setFunc,
                                           BOOLEAN showHide = FALSE ) = 0 ;

      virtual void   getObjFunNames( const string &className,
                                     set< string > &setFunc,
                                     BOOLEAN showHide = FALSE ) = 0 ;

      virtual void   getObjFunNames( const void *pObj,
                                     set<string> &setFunc,
                                     BOOLEAN showHide = FALSE ) = 0 ;

      virtual void   getObjPropNames( const void *pObj,
                                      set<string> &setProp ) = 0 ;

      virtual BOOLEAN isInstanceOf( const void *pObj,
                                   const string &objName ) = 0 ;

      virtual string getObjClassName( const void *pObj ) = 0 ;

   public:
      INT32          loadUsrDefObj( _sptObjDesc *desc ) ;

   private:
      virtual INT32 _loadUsrDefObj( _sptObjDesc *desc ) = 0 ;

   private:
      std::list< std::string > _fileNameList ;
      std::list< std::string > _fileNameStack ;

   protected:
      UINT32         _loadMask ;

   } ;
   typedef class _sptScope sptScope ;

}

#endif // SPT_SCOPE_HPP_

