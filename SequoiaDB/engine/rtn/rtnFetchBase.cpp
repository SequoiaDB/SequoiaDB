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

   Source File Name = rtnFetchBase.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/


#include "rtnFetchBase.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   /*
      _rtnFetchBuilder implement
   */
   _rtnFetchBuilder::_rtnFetchBuilder ()
   {
   }

   _rtnFetchBuilder::~_rtnFetchBuilder ()
   {
      _mapFuncs.clear() ;
   }

   INT32 _rtnFetchBuilder::_register( INT32 type, FETCH_NEW_FUNC pFunc )
   {
      INT32 rc = SDB_OK ;

      if ( !_mapFuncs.insert( std::make_pair( type, pFunc ) ).second )
      {
         PD_LOG ( PDERROR, "Register fetch [%d] failed", type ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _rtnFetchBase *_rtnFetchBuilder::create( INT32 type )
   {
      MAP_FUNCS::iterator it = _mapFuncs.find( type ) ;
      if ( it != _mapFuncs.end() && NULL != it->second )
      {
         return (*( it->second ) )() ;
      }

      return NULL ;
   }

   void _rtnFetchBuilder::release( _rtnFetchBase *pFetch )
   {
      if ( pFetch )
      {
         SDB_OSS_DEL pFetch ;
      }
   }

   _rtnFetchBuilder *getRtnFetchBuilder ()
   {
      static _rtnFetchBuilder s_fetchBuilder ;
      return &s_fetchBuilder ;
   }

   /*
      _rtnFetchAssit implement
   */
   _rtnFetchAssit::_rtnFetchAssit ( FETCH_NEW_FUNC pFunc )
   {
      if ( pFunc )
      {
         _rtnFetchBase *pFetch = (*pFunc)() ;
         if ( pFetch )
         {
            getRtnFetchBuilder()->_register ( pFetch->getType(), pFunc ) ;
            SDB_OSS_DEL pFetch ;
            pFetch = NULL ;
         }
      }
   }

   _rtnFetchAssit::~_rtnFetchAssit ()
   {
   }


}


