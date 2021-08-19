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

   Source File Name = sptFuncDef.hpp

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

#ifndef SPT_FUNC_DEF_HPP__
#define SPT_FUNC_DEF_HPP__

#include "core.hpp"
#include "../bson/bson.h"
#include "sptObjDesc.hpp"
#include "sptSPScope.hpp"


#include <vector>
#include <map>
#include <string>

using namespace bson ;

namespace engine
{
   #define SPT_FUNC_CONSTRUCTOR       0x00000001
   #define SPT_FUNC_INSTANCE          0x00000002
   #define SPT_FUNC_STATIC            0x00000004
   #define SPT_FUNC_INS_STA           (SPT_FUNC_INSTANCE | SPT_FUNC_STATIC)

   struct _sptFuncDefInfo
   {
      string funcName ;
      INT32  funcType ;
   } ;
   typedef _sptFuncDefInfo sptFuncDefInfo ;

   typedef pair< string, vector<sptFuncDefInfo> > PAIR_FUNC_DEF_INFO ;
   typedef map< string, vector<sptFuncDefInfo> > MAP_FUNC_DEF_INFO ;
   typedef map< string, vector<sptFuncDefInfo> >::iterator MAP_FUNC_DEF_INFO_IT ;
   
   class _sptFuncDef : public SDBObject
   {
   private:
      _sptFuncDef( const _sptFuncDef& ) ;
      _sptFuncDef& operator=( const _sptFuncDef& ) ;

   public:
      _sptFuncDef() ;
      ~_sptFuncDef() {}

   public:
      static _sptFuncDef&         getInstance() ;
      
   public:
      const MAP_FUNC_DEF_INFO&    getFuncDefInfo() ;
      
   private:
      INT32                       _init() ;
      INT32                       _loadFuncInfo( string &className, 
                                       const set< string > & setFunc,
                                       const set< string > & setStaticFunc ) ;
      INT32                       _insert( const string &className, 
                                           const string &funcName,
                                           INT32 type ) ;
   private:
      MAP_FUNC_DEF_INFO           _map_func_def ;
   } ;
   typedef class _sptFuncDef sptFuncDef ;

} // namespace
#endif // SPT_FUNC_DEF_HPP__
