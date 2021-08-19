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

   Source File Name = expHosts.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2020  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_HOSTS_HPP_
#define EXP_HOSTS_HPP_

#include "core.hpp"
#include "oss.hpp"
#include <string>
#include <vector>

using namespace std;

namespace exprt
{
   struct Host
   {
      string hostname ;
      string svcname ;
   } ;

   class Hosts : public SDBObject
   {
   public:
      static INT32 parse( const string& hostList, vector<Host>& hosts ) ;
      static void removeDuplicate( vector<Host>& hosts ) ;
   } ;
}

#endif /* EXP_HOSTS_HPP_ */
