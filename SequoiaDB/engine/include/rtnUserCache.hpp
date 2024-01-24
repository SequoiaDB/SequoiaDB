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

   Source File Name = rtnUserCache.hpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for update
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/25/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_USER_CACHE_HPP__
#define RTN_USER_CACHE_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "ossRWMutex.hpp"
#include "auth.hpp"
#include "pmdEDU.hpp"
#include <boost/smart_ptr.hpp>

namespace engine
{
   class _rtnUserCache : public SDBObject
   {
   public:
      typedef ossPoolString KEY_TYPE;
      typedef boost::shared_ptr< const authAccessControlList > VALUE_TYPE;
      typedef std::map< KEY_TYPE, VALUE_TYPE > DATA_TYPE;

   public:
      _rtnUserCache() {}
      ~_rtnUserCache() {}

      INT32 getACL( pmdEDUCB *cb, const KEY_TYPE &userName, VALUE_TYPE &acl );
      void remove( const KEY_TYPE &userName);
      void clear();

   private:
      VALUE_TYPE _get( const KEY_TYPE &userName );
      std::pair< DATA_TYPE::iterator, bool > _insert( const KEY_TYPE &userName,
                                                      const VALUE_TYPE &acl );
      INT32 _fetch( pmdEDUCB *cb, const KEY_TYPE &userName, VALUE_TYPE &acl );
      INT32 _fetchForCoord( pmdEDUCB *cb,
                            const KEY_TYPE &userName,
                            const CHAR *pMsgBuffer,
                            BSONObj &privsObj );

   private:
      ossSpinSLatch _latch;
      ossSpinXLatch _fetchLatch;
      DATA_TYPE _data;
   };
   typedef _rtnUserCache rtnUserCache;

} // namespace engine

#endif
