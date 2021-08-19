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

   Source File Name = utilOptions.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_OPTIONS_HPP_
#define UTIL_OPTIONS_HPP_

#include "ossTypes.h"
#include "oss.hpp"
#include <boost/program_options.hpp>
#include <vector>

namespace po = boost::program_options;

using namespace std;

#define utilOptType(T) po::value<T>()

namespace engine
{

   typedef po::options_description_easy_init utilOptAdd;
   typedef po::options_description utilOptDesc;
   typedef po::variables_map utilOptMap;

   struct utilOptionGroup: public SDBObject
   {
      string name;
      utilOptDesc desc;
      BOOLEAN hidden;

      utilOptionGroup()
      {
         hidden = FALSE;
      }

      utilOptionGroup(const string& name, BOOLEAN hidden = FALSE)
      {
         this->name = name;
         this->hidden = hidden;
      }
   };

   typedef vector<utilOptionGroup*> utilOptionGroups;

   class utilOptions: public SDBObject
   {
   public:
      utilOptions();
      virtual ~utilOptions();

   public:
      utilOptAdd addOptions(const string& groupName = "", BOOLEAN hidden = FALSE);
      INT32 parse(INT32 argc, CHAR* argv[]);
      void print(BOOLEAN printHidden = FALSE);

      BOOLEAN has(const string& option);

      template<typename T>
      T get(const string& option)
      {
         return _allOpt[option].as<T>();
      }

   protected:
      utilOptionGroup* _getOptGroup(const string& groupName);

   protected:
      utilOptDesc             _allDesc;
      utilOptMap              _allOpt;
      utilOptionGroups        _optGroups;
      BOOLEAN                 _parsed;
   };
}

#endif /* UTIL_OPTIONS_HPP_ */
