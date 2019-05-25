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

   Source File Name = mthNodePool.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_NODEPOOL_HPP_
#define MTH_NODEPOOL_HPP_

#include <set>
using namespace std;

#define MTH_NODE_POOL_DEFAULT_SZ 4 

namespace engine
{
   template< typename TYPE, UINT32 nodeSize = MTH_NODE_POOL_DEFAULT_SZ >
   class _mthNodePool : public SDBObject
   {
   public:
      _mthNodePool()
      {
         UINT32 index = 0 ;
         for ( ; index < nodeSize ; index++ )
         {
            _staticIdleSet.insert( &_static[ index ] ) ;
         }
      }

      ~_mthNodePool()
      {
         clear() ;
      }

   public:
      INT32 allocate( TYPE *&tp )
      {
         INT32 rc = SDB_OK ;
         if ( !_staticIdleSet.empty() )
         {
            typename set<TYPE*>::iterator iter = _staticIdleSet.begin() ;
            tp = *iter ;
            _staticIdleSet.erase( iter ) ;
         }
         else
         {
            TYPE *node = SDB_OSS_NEW TYPE ;
            if ( NULL == node )
            {
               rc = SDB_OOM ;
               goto error ;
            }

            _dynamic.insert( node ) ;
            tp = node ;
         }
      done:
         return rc ;
      error:
         goto done ;
      }

      void release( TYPE *tp )
      {
         if ( tp >= &_static[0] && tp <= &_static[ nodeSize -1 ] )
         {
            SDB_ASSERT( _staticIdleSet.find( tp ) == _staticIdleSet.end() ,
                        "must be allocate before!" ) ;
            SDB_ASSERT( ( ( tp - (&_static[0]) ) % sizeof( TYPE ) ) == 0,
                          "address must be align!") ;

            _staticIdleSet.insert( tp ) ;
         }
         else
         {
            _dynamic.erase( tp ) ;
            SDB_OSS_DEL tp ;
         }
      }

      void clear()
      {
         typename set<TYPE*>::iterator iter = _dynamic.begin() ;
         for ( ; iter != _dynamic.end(); ++iter )
         {
            SDB_OSS_DEL *iter ;
         }

         _dynamic.clear() ;
         _staticIdleSet.clear() ;
         return ;
      }

   private:
      TYPE _static[ nodeSize ] ;
      set<TYPE*> _staticIdleSet ;
      set<TYPE*> _dynamic ;
   } ;
}

#endif

