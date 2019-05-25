/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = impHosts.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/8/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impHosts.hpp"
#include "pd.hpp"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace import
{
   static bool hostEqual(const Host& host1, const Host& host2)
   {
      string hostname1 = host1.hostname;
      string hostname2 = host2.hostname;

      if ("127.0.0.1" == hostname1)
      {
         hostname1 = "localhost";
      }

      if ("127.0.0.1" == hostname2)
      {
         hostname2 = "localhost";
      }

      if (hostname1 != hostname2)
      {
         return false;
      }

      return host1.svcname == host2.svcname;
   }

   class HostComparer
   {
   public:
      bool operator()(const Host& host1, const Host& host2)
      {
         string hostname1 = host1.hostname;
         string hostname2 = host2.hostname;


         if ("127.0.0.1" == hostname1)
         {
            hostname1 = "localhost";
         }

         if ("127.0.0.1" == hostname2)
         {
            hostname2 = "localhost";
         }

         if (hostname1 < hostname2)
         {
            return true;
         }

         return host1.svcname < host2.svcname;
      }
   };

   INT32 Hosts::parse(const string& hostList, vector<Host>& hosts)
   {
      INT32 rc = SDB_OK;

      boost::char_separator<char> hostSep(",");
      boost::char_separator<char> nameSep(":");
      typedef boost::tokenizer<boost::char_separator<char> > CustomTokenizer;
      CustomTokenizer hostTok(hostList, hostSep);

      hosts.clear();

      for (CustomTokenizer::iterator it = hostTok.begin();
           it != hostTok.end(); it++)
      {
         string host = *it;
         host = boost::algorithm::trim_copy_if(host, boost::is_space());
         if (host.empty())
         {
            continue;
         }

         {
            CustomTokenizer nameTok(host, nameSep);
            CustomTokenizer::iterator nameIt = nameTok.begin();
            string hostname;
            string svcname;

            if (nameIt == nameTok.end())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid host [%s]", host.c_str());
               goto error;
            }
            hostname = *nameIt;
            hostname = boost::algorithm::trim_copy_if(hostname, boost::is_space());
            if (hostname.empty())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "empty hostname of host [%s]", host.c_str());
               goto error;
            }

            *nameIt++;
            if (nameIt == nameTok.end())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid host [%s]", host.c_str());
               goto error;
            }
            svcname = *nameIt;
            svcname = boost::algorithm::trim_copy_if(svcname, boost::is_space());
            if (svcname.empty())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "empty svcname of host [%s]", host.c_str());
               goto error;
            }

            *nameIt++;
            if (nameIt != nameTok.end())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid host [%s]", host.c_str());
               goto error;
            }

            {
               Host h;
               h.hostname = hostname;
               h.svcname = svcname;
               hosts.push_back(h);
            }
         }
      }

      if (hosts.empty())
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "there is no host");
         goto error;
      }

   done:
      return rc;
   error:
      hosts.clear();
      goto done;
   }

   void Hosts::removeDuplicate(vector<Host>& hosts)
   {
      if (hosts.empty())
      {
         return;
      }

      std::sort(hosts.begin(), hosts.end(), HostComparer());
      hosts.erase(std::unique(hosts.begin(), hosts.end(), hostEqual), hosts.end());
   }
}
