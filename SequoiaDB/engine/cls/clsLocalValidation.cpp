/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = clsLocalValidation.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/03/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsLocalValidation.hpp"
#include "pmdEDU.hpp"
#include "rtn.hpp"
#include "dmsCB.hpp"


using namespace bson ;

namespace engine
{
   /*static void func()
   {
      return ;
   }*/

   INT32 _clsLocalValidation::run()
   {
      INT32 rc = SDB_OK ;
      const UINT32 pLen = 512 ;
      MON_CS_LIST csList ;

      CHAR *p = (CHAR *)SDB_OSS_MALLOC( pLen ) ;
      if ( NULL == p )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      *p = '\0' ;
      *( p + pLen - 1 ) = '\0' ;

      /*try
      {
         boost::thread t( func ) ;
         t.join() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "failed to create new thread:%s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }*/


      pmdUpdateValidationTick() ;

   done:
      SAFE_OSS_FREE( p ) ;
      return rc ;
   error:
      goto done ;
   }

}

