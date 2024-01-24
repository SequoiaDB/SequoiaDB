/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = utilAddress.hpp

   Descriptive Name = Address utility

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_ADDRESS_HPP__
#define UTIL_ADDRESS_HPP__

#include "utilArray.hpp"
#include "ossSocket.hpp"

namespace engine
{
   /**
    * Address information, including host and service name.
    */
   class _utilAddrPair : public SDBObject
   {
   public:
      _utilAddrPair() ;
      ~_utilAddrPair() ;

      INT32 setHost( const CHAR *host ) ;
      const CHAR *getHost() const ;
      INT32 setService( const CHAR *service ) ;
      const CHAR *getService() const ;

      BOOLEAN operator==( const _utilAddrPair& r ) const ;

   private:
      CHAR _host[ OSS_MAX_HOSTNAME + 1 ] ;
      CHAR _service[ OSS_MAX_SERVICENAME + 1 ] ;
   } ;
   typedef _utilAddrPair utilAddrPair ;

   /**
    * Service address item container. Each item contains a host name or an IP
    * address and a service name.
    */
   class _utilAddrContainer : public SDBObject
   {
   public:
      _utilAddrContainer() {}
      ~_utilAddrContainer() {}

      INT32 append( const utilAddrPair &addr ) ;

      OSS_INLINE UINT32 size() const
      {
         return _addresses.size() ;
      }

      OSS_INLINE void clear()
      {
         _addresses.clear() ;
      }

      OSS_INLINE BOOLEAN empty() const
      {
         return _addresses.empty() ;
      }

      _utilArray<utilAddrPair>& getAddresses()
      {
         return _addresses ;
      }

      BOOLEAN contains( const utilAddrPair &addr ) ;

   private:
      _utilArray<utilAddrPair> _addresses ;
   } ;
   typedef _utilAddrContainer utilAddrContainer ;

   /**
    * @brief Parse an address list into address pair array.
    *
    * @param addrStr A string which contains one or more addresses.
    * @param addrArray Address pair array.
    * @param itemSeparator Separator of address items in the string.
    * @param innerSeparator Separator of host and service in an address.
    *
    * Each address item in the address list should be in the format of
    * <host>:<svcname>.
    */
   INT32 utilParseAddrList( const CHAR *addrStr,
                            utilAddrContainer &addrArray,
                            const CHAR *itemSeparator = ",",
                            const CHAR *innerSeparator = ":" ) ;
}

#endif /* UTIL_ADDRESS_HPP__ */
