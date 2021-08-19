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

   Source File Name = utilOptions.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/9/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilOptions.hpp"
#include "utilParam.hpp"
#include "pd.hpp"
#include <iostream>

namespace engine
{
   utilOptions::utilOptions()
   {
      _parsed = FALSE;
   }

   utilOptions::~utilOptions()
   {
      utilOptionGroups::iterator it = _optGroups.begin();
      while (it != _optGroups.end())
      {
         utilOptionGroup* group = *it;
         SDB_OSS_DEL(group);
         it = _optGroups.erase(it);
      }
   }

   utilOptionGroup* utilOptions::_getOptGroup(const string& groupName)
   {
      utilOptionGroup* group = NULL;
      
      for (UINT32 i = 0; i < _optGroups.size(); i++)
      {
         utilOptionGroup* _group = _optGroups.at(i);
         if (groupName == _group->name)
         {
            group = _group;
            break;
         }
      }

      return group;
   }

   utilOptAdd utilOptions::addOptions(const string& groupName, BOOLEAN hidden)
   {
      utilOptionGroup* group = NULL;

      group = _getOptGroup(groupName);
      if (NULL == group)
      {
         group = SDB_OSS_NEW utilOptionGroup(groupName, hidden);
         if (NULL == group)
         {
            throw "Out of memory";
         }
         _optGroups.push_back(group);
      }

      return group->desc.add_options();
   }

   INT32 utilOptions::parse(INT32 argc, CHAR* argv[])
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(!_parsed, "already parsed");

      for (UINT32 i = 0; i < _optGroups.size(); i++)
      {
         utilOptionGroup* group = _optGroups.at(i);
         _allDesc.add(group->desc);
      }

      rc = utilReadCommandLine( argc, argv, _allDesc, _allOpt, FALSE );
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to parse arguments, rc=%d", rc);
         goto error;
      }

      _parsed = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   void utilOptions::print(BOOLEAN printHidden)
   {
      for (UINT32 i = 0; i < _optGroups.size(); i++)
      {
         utilOptionGroup* group = _optGroups.at(i);

         if (group->hidden && !printHidden)
         {
            continue;
         }

         if (!group->name.empty())
         {
            std::cout << group->name << ":" << std::endl;
         }
         std::cout << group->desc << std::endl;
      }
   }

   BOOLEAN utilOptions::has(const string& option)
   {
      SDB_ASSERT(_parsed, "must be parsed");
      SDB_ASSERT(!option.empty(), "empty option");

      return (_allOpt.count(option) > 0);
   }
}
