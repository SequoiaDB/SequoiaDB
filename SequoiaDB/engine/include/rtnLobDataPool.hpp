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

   Source File Name = rtnLobDataPool.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_LOBDATAPOOL_HPP_
#define RTN_LOBDATAPOOL_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <vector>
#include "ossMemPool.hpp"
#include "pmdDef.hpp"

namespace engine
{
   class _rtnLobDataPool : public SDBObject
   {
   public:
      _rtnLobDataPool() ;
      virtual ~_rtnLobDataPool() ;

   public:
      struct tuple
      {
         SINT64 offset ;
         UINT32 len ;
         const CHAR *data ;

         tuple()
         :offset( -1 ),
          len( 0 ),
          data( NULL )
         {

         }

         tuple( SINT64 o,
                UINT32 l,
                const CHAR *d )
         :offset( o ),
          len( l ),
          data( d )
         {

         }

         tuple( const tuple &t )
         :offset( t.offset ),
          len( t.len ),
          data( t.data )
         {

         }

         tuple &operator=( const tuple &t  )
         {
            offset = t.offset ;
            len = t.len ;
            data = t.data ;
            return *this ;
         }

         void clear()
         {
            offset = -1 ;
            len = 0 ;
            data = NULL ;
         }
      } ;

   public:
      /// buf will be freed by pool
      /// the last buf allocated will be invalid when allocate mem.
      INT32 allocate( UINT32 len, CHAR **buf ) ;

      /// realBuf will be freed when do clear
      INT32 push( const CHAR *data, UINT32 len,
                  SINT64 offset ) ;

      void pushDone() ;

      void entrust( const pmdEDUEvent &event ) ;

      UINT32 getLastDataSize() const
      {
         return _lastDataSz ;
      }

      BOOLEAN match( SINT64 offset ) ;

      BOOLEAN next( UINT32 len, const CHAR **data, UINT32 &read ) ;

      void clear() ;

   private:
      void _seek( SINT64 ) ;

   private:
      struct compare
      {
         BOOLEAN operator()( const tuple &l, const tuple &r )
         {
            return l.offset < r.offset ;
         }
      } ;

      CHAR *_buf ;
      UINT32 _bufSz ;
      std::vector<tuple>         _pool ;
      ossPoolList<pmdEDUEvent>   _toBeFreed ;
      UINT32 _lastDataSz ;
      UINT32 _dataSz ;
      SINT32 _current ;
      tuple _currentTuple ;
      friend class _rtnLobStream ;
   } ;
   typedef class _rtnLobDataPool rtnLobDataPool ;
}

#endif

