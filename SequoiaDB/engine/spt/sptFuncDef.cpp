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

   Source File Name = sptFuncDef.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptFuncDef.hpp"
#include "sptSPScope.hpp"
#include "sptContainer.hpp"

#include "ossUtil.hpp"

namespace engine
{

   _sptFuncDef::_sptFuncDef()
   {
      _init() ;
   }

   _sptFuncDef& _sptFuncDef::getInstance()
   {
      static _sptFuncDef obj ;
      return obj ;
   }

   INT32 _sptFuncDef::_init()
   {
      INT32 rc = SDB_OK ;      
      engine::sptContainer container ;
      engine::sptScope *scope = NULL ;
      set< string > setClass ;
      set< string >::iterator it ;

      // get a temp scope
      rc = container.init() ;
      if ( rc )
      {
         ossPrintf( "Failed to init container, rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }
      scope = container.newScope() ;
      if ( !scope )
      {
         rc = SDB_SYS ;
         ossPrintf( "Failed to new scope in container, rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;

      }

      // get functions in all the js class
      sptGetObjFactory()->getClassNames( setClass, FALSE ) ;
      it = setClass.begin() ;
      for ( ; it != setClass.end(); ++it )
      {
         string className = *it ;
         set< string > setFunc ;
         set< string > setStaticFunc ;
         scope->getObjFunNames( className, setFunc, FALSE ) ;
         scope->getObjStaticFunNames( className, setStaticFunc, FALSE ) ;
         rc = _loadFuncInfo( className, setFunc, setStaticFunc ) ;
         if ( rc )
         {
            ossPrintf( "Load the functions of class[%s] failed, "
                       "rc: %d"OSS_NEWLINE, className.c_str(), rc ) ;
            goto error ;
         }
      }
      
   done:
      if ( scope )
      {
         container.releaseScope( scope ) ;
      }
      container.fini() ;
      return rc ;
   error:
      goto done ;
   }

   const MAP_FUNC_DEF_INFO& _sptFuncDef::getFuncDefInfo()
   {
      return _map_func_def ;
   }

   INT32 _sptFuncDef::_loadFuncInfo( string &className, 
                                       const set< string > & setFunc,
                                       const set< string > & setStaticFunc )
   {
      // register constructor
      _insert( className, className, SPT_FUNC_CONSTRUCTOR ) ;
      // register instance method for help
      set< string >::iterator itr = setFunc.begin() ;
      for ( ; itr != setFunc.end() ; ++itr )
      {
         _insert( className, *itr, SPT_FUNC_INSTANCE ) ;
      }
      // register static method for help
      itr = setStaticFunc.begin() ;
      for ( ; itr != setStaticFunc.end() ; ++itr )
      {
         _insert( className, *itr, SPT_FUNC_STATIC ) ;
      }
      return SDB_OK ;
   }

   INT32 _sptFuncDef::_insert( const string &className, 
                               const string &funcName,
                               INT32 type )
   {
      sptFuncDefInfo defInfo ;
      MAP_FUNC_DEF_INFO_IT it ;

      it = _map_func_def.find( className ) ;
      if ( it == _map_func_def.end() )
      {
         vector<sptFuncDefInfo> vec ;
         defInfo.funcName = funcName ;
         defInfo.funcType = type ;
         vec.push_back( defInfo ) ;
         _map_func_def.insert( PAIR_FUNC_DEF_INFO( className, vec ) ) ;
      }
      else
      {
         vector<sptFuncDefInfo> &vec = it->second ;
         vector<sptFuncDefInfo>::iterator itr = vec.begin() ;
         for ( ; itr != vec.end(); itr++ )
         {
            if ( itr->funcName == funcName )
            {
               itr->funcType |= type ;
               break ;
            }
         }
         if ( itr == vec.end() )
         {
            defInfo.funcName = funcName ;
            defInfo.funcType = type ;
            vec.push_back( defInfo ) ;
         }
      }

      return SDB_OK ;
   }

} // namespace
