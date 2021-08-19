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
*******************************************************************************/
#include <string.h>
#include "core.h"
#include "ossErr.h"
#include "sptParseMandoc.hpp"
SDB_EXTERN_C_START
#include "parseMandoc.h"
SDB_EXTERN_C_END

// class parseMandoc
_sptParseMandoc& _sptParseMandoc::getInstance()
{
   static _sptParseMandoc _instance ;
   return _instance ;
}

INT32 _sptParseMandoc::parse(const CHAR* filename)
{
   INT32 rc = SDB_OK ;
#if defined _WIN32
   const CHAR *argv[6] = { "sdb", "-K", "utf-8", "-T", "locale", filename } ;
#else
   const CHAR *argv[6] = { "sdb", "-K", "utf-8", "-T", "utf8", filename } ;
#endif
   rc = parse_mandoc(6, (const CHAR **)argv) ;
   if ( rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

