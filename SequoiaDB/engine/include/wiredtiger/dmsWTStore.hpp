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

   Source File Name = dmsWTStore.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_WT_STORE_HPP_
#define DMS_WT_STORE_HPP_

#include "wiredtiger/dmsWTUtil.hpp"

namespace engine
{
namespace wiredtiger
{

   /*
      _dmsWTStore define
    */
   class _dmsWTStore : public utilPooledObject
   {
   public:
      _dmsWTStore() = default ;
      ~_dmsWTStore() = default ;

      _dmsWTStore( const ossPoolString &uri )
      : _uri( uri )
      {
      }

      _dmsWTStore( const _dmsWTStore &&o )
      : _uri( std::move( o._uri ) )
      {
      }

      _dmsWTStore( const _dmsWTStore &o ) = default ;
      _dmsWTStore &operator =( const _dmsWTStore & ) = default ;

      const ossPoolString &getURI() const
      {
         return _uri ;
      }

      const ossPoolString &getStatsURI() const
      {
         return _statsURI ;
      }

      void setURI( const ossPoolString &uri )
      {
         _uri = uri ;
         _statsURI = "statistics:" + _uri ;
      }

      void setURI( const CHAR *uri )
      {
         _uri.assign( uri ) ;
         _statsURI = "statistics:" + _uri ;
      }

   protected:
      ossPoolString _uri ;
      ossPoolString _statsURI ;
   } ;

   typedef class _dmsWTStore dmsWTStore ;

}
}

#endif // DMS_WT_STORE_HPP_
