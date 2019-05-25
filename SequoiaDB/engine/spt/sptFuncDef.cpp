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

   const MAP_FUNC_DEF_INFO& _sptFuncDef::getFuncDefInfo()
   {
      return _map_func_def ;
   }

   INT32 _sptFuncDef::_init()
   {
      INT32 rc = SDB_OK ;
      SPT_VEC_OBJDESC vecObjs ;
      sptGetObjFactory()->getObjDescs( vecObjs ) ;
      for ( UINT32 i = 0 ; i < vecObjs.size() ; ++i )
      {
         sptObjDesc *desc = (sptObjDesc*)vecObjs[ i ] ;
         rc = _loadFuncInfo( desc ) ;
         if ( rc )
         {
            ossPrintf( "Load the functions of class[%s] failed, "
                       "rc: %d"OSS_NEWLINE, desc->getJSClassName(), rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptFuncDef::_loadFuncInfo( _sptObjDesc *desc )
   {
      const CHAR *objName = desc->getJSClassName() ;
      const _sptFuncMap &fMap = desc->getFuncMap() ;
      const sptFuncMap::NORMAL_FUNCS &memberFuncs = fMap.getMemberFuncs() ;
      const sptFuncMap::NORMAL_FUNCS &staticFuncs = fMap.getStaticFuncs() ;

      _insert( objName, objName, SPT_FUNC_CONSTRUCTOR ) ;
      sptFuncMap::NORMAL_FUNCS::const_iterator itr = memberFuncs.begin() ;
      for ( ; itr != memberFuncs.end() && !itr->first.empty() ; itr++ )
      {
         _insert( objName, itr->first.c_str(), SPT_FUNC_INSTANCE ) ;
      }
      itr = staticFuncs.begin() ;
      for ( ; itr != staticFuncs.end() && !itr->first.empty() ;  itr++ )
      {
         _insert( objName, itr->first.c_str(), SPT_FUNC_STATIC ) ;
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
