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

   Source File Name = utilPath.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/
#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilPath.hpp"
#include "ossProc.hpp"
#include "pd.hpp"

INT32 getProgramPath( CHAR *pOutputPath, INT32 maxLen )
{
   INT32 rc = SDB_OK ;

   rc = ossGetEWD( pOutputPath, maxLen ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to get the path of excutable file" ) ;
      goto error ;
   }

   ossStrncat( pOutputPath, OSS_FILE_SEP, ossStrlen(OSS_FILE_SEP) ) ;

done :
   return rc ;
error :
   goto done ;
}


