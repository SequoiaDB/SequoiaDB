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

   Source File Name = impUtil.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impUtil.hpp"
#include "utilCommon.hpp"
#include "pd.hpp"
#include <algorithm>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace import
{
   UINT32 RC2ShellRC(INT32 rc)
   {
      return engine::utilRC2ShellRC(rc);
   }

   static BOOLEAN _hasFile(vector<string>& files, const string& file)
   {
      for (vector<string>::iterator it = files.begin(); it != files.end(); it++)
      {
         if (fs::equivalent(*it, file))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   INT32 parseFileList(const string& fileList, vector<string>& files)
   {
      INT32 rc = SDB_OK;

      try
      {
         boost::char_separator<char> hostSep(",");
         typedef boost::tokenizer<boost::char_separator<char> > CustomTokenizer;
         CustomTokenizer fileTok(fileList, hostSep);

         files.clear();

         for (CustomTokenizer::iterator it = fileTok.begin();
              it != fileTok.end(); it++)
         {
            string file = *it;
            file = boost::algorithm::trim_copy_if(file, boost::is_space());
            if (file.empty())
            {
               continue;
            }

            if (!fs::exists(file))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if (fs::is_directory(file))
            {
               fs::directory_iterator it(file);
               fs::directory_iterator file_end;

               for (; it != file_end; it++)
               {
                  if (fs::is_directory(it->status()))
                  {
                     continue;
                  }

                  if (!_hasFile(files, it->path().string()))
                  {
                     files.push_back(it->path().string());
                  }
               }
            }
            else
            {
               if (!_hasFile(files, file))
               {
                  files.push_back(file);
               }
            }
         }
      }
      catch(std::exception& e)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "unexpected error happened: %s", e.what());
         goto error;
      }

      if (files.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
}
