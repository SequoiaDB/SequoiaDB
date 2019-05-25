/*******************************************************************************

   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobMetaCache.hpp

   Descriptive Name = LOB meta cache

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/22/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOB_META_CACHE_HPP_
#define RTN_LOB_META_CACHE_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "dmsLobDef.hpp"
#include "rtnLobPieces.hpp"

namespace engine
{
   class _rtnLobMetaCache: public SDBObject
   {
   public:
      _rtnLobMetaCache() ;
      ~_rtnLobMetaCache() ;

   private:
      _rtnLobMetaCache( const _rtnLobMetaCache& ) ;
      void operator=( const _rtnLobMetaCache& ) ;

   public:
      INT32 cache( const _dmsLobMeta& meta ) ;
      INT32 merge( const _dmsLobMeta& meta, INT32 lobPageSize ) ;

   public:
      OSS_INLINE const _dmsLobMeta* lobMeta()
      {
         return (_dmsLobMeta*)_metaBuf ;
      }
      OSS_INLINE BOOLEAN needMerge() const
      {
         return _needMerge ;
      }
      OSS_INLINE void setNeedMerge( BOOLEAN needMerge )
      {
         _needMerge = needMerge ;
      }

   private:
      CHAR*       _metaBuf ;
      BOOLEAN     _needMerge ;
   } ;
}

#endif /* RTN_LOB_META_CACHE_HPP_ */

