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

   Source File Name = impCoord.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_COORD_HPP_
#define IMP_COORD_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impHosts.hpp"
#include "../client/client.h"
#include <string>
#include <vector>

using namespace std;

namespace import
{
   class Coords: public SDBObject
   {
   public:
      Coords();
      ~Coords();
      INT32 init(const vector<Host>& hosts,
                 const string& user,
                 const string& password,
                 BOOLEAN useSSL);
      INT32 getRandomCoord(string& hostname, string& svcname);
      static INT32 getRandomCoord(vector<Host>& hosts, UINT32& refCount,
                                  string& hostname, string& svcname);

   private:
      INT32 _connect(const string& hostname,
                     const string& svcname,
                     sdbConnectionHandle& conn);
      void  _disconnect(sdbConnectionHandle& conn);
      INT32 _checkCoord(const string& hostname, const string& svcname);

   private:
      string   _user;
      string   _password;
      BOOLEAN  _useSSL;
      BOOLEAN  _inited;
      UINT32   _refCount;

      vector<Host> _coords;
   };
}

#endif /* IMP_COORD_HPP_ */
