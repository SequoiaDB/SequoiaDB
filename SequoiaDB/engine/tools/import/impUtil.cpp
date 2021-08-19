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
               // ignore empty string or white space
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
                     // ignore sub-directory
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

   INT32 checkDateTimeFormat(const string& format)
   {
      INT32 rc = SDB_OK;
      const CHAR* fmt = format.c_str();
      INT32 len = format.length();
      BOOLEAN hasYear = FALSE;
      BOOLEAN hasMonth = FALSE;
      BOOLEAN hasDay = FALSE;
      BOOLEAN hasHour = FALSE;
      BOOLEAN hasMinute = FALSE;
      BOOLEAN hasSecond = FALSE;
      BOOLEAN hasMillisecond = FALSE;
      BOOLEAN hasMicrosecond = FALSE;

      while (len > 0)
      {
         switch(*fmt)
         {
         // year: YYYY
         case 'Y':
            if (hasYear)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('Y' != fmt[1] ||
                'Y' != fmt[2] ||
                'Y' != fmt[3])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasYear = TRUE;
            fmt += 4;
            len -= 4;
            break;
         // month: MM
         case 'M':
            if (hasMonth)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('M' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMonth = TRUE;
            fmt += 2;
            len -= 2;
            break;
         // day: DD
         case 'D':
            if (hasDay)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('D' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasDay = TRUE;
            fmt += 2;
            len -= 2;
            break;
         // hour: HH
         case 'H':
            if (hasHour)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('H' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasHour = TRUE;
            fmt += 2;
            len -= 2;
            break;
         // minute: mm
         case 'm':
            if (hasMinute)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('m' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMinute = TRUE;
            fmt += 2;
            len -= 2;
            break;
         // second: ss
         case 's':
            if (hasSecond)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('s' != fmt[1])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasSecond = TRUE;
            fmt += 2;
            len -= 2;
            break;
         // millisecond: SSS
         case 'S':
            if (hasMillisecond || hasMicrosecond)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('S' != fmt[1] ||
                'S' != fmt[2])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMillisecond = TRUE;
            fmt += 3;
            len -= 3;
            break;
         // microsecond: ffffff
         case 'f':
            if (hasMillisecond || hasMicrosecond)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ('f' != fmt[1] ||
                'f' != fmt[2] ||
                'f' != fmt[3] ||
                'f' != fmt[4] ||
                'f' != fmt[5])
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            hasMicrosecond = TRUE;
            fmt += 6;
            len -= 6;
            break;
         // time zone: +/-XXXX
         case '+':
         case '-':
         {
            INT32 hour = 0 ;
            INT32 minute = 0 ;

            if ( !isdigit( fmt[1] ) )
            {
               fmt++;
               len--;
               break ;
            }

            if ( !isdigit( fmt[2] ) || !isdigit( fmt[3] ) )
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if ( isdigit( fmt[4] ) )
            {
               hour = ( fmt[1] - '0' ) * 10 + ( fmt[2] - '0' ) ;
               minute = ( fmt[3] - '0' ) * 10 + ( fmt[4] - '0' ) ;

               fmt += 4 ;
               len -= 4 ;
            }
            else
            {
               hour = fmt[1] - '0' ;
               minute = ( fmt[2] - '0' ) * 10 + ( fmt[3] - '0' ) ;

               fmt += 3 ;
               len -= 3 ;
            }

            if ( hour * 60 + minute > IMP_UTIL_TIMEZONE_MAX )
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            break;
         }
         case 'Z':
         // any charcater: *
         case '*':
         default:
            fmt++;
            len--;
            break;
         }
      }

      if (!hasYear)
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
