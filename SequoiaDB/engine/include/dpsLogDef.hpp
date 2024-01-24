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

   Source File Name = dpsLogDef.hpp 

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSLOGDEF_HPP_
#define DPSLOGDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dpsDef.hpp"

namespace engine
{

   class DPS_LSN : public SDBObject
   {
   public :
      /// 0x00 - 0x07
      DPS_LSN_OFFSET offset ;
      /// 0x08 - 0x0B
      DPS_LSN_VER  version ;

      DPS_LSN()
      {
         offset = DPS_INVALID_LSN_OFFSET ;
         version = DPS_INVALID_LSN_VERSION ;
      }

      DPS_LSN( const DPS_LSN &lsn )
      {
         offset = lsn.offset ;
         version = lsn.version ;
      }

      DPS_LSN &operator=( const DPS_LSN &lsn )
      {
         offset = lsn.offset ;
         version = lsn.version ;
         return *this ;
      }

      BOOLEAN invalid() const
      {
         return ( DPS_INVALID_LSN_OFFSET == offset ) ||
                ( DPS_INVALID_LSN_VERSION == version ) ;
      }

      void set( DPS_LSN_OFFSET offset, DPS_LSN_VER  version )
      {
         this->offset = offset ;
         this->version = version ;
      }

      void reset()
      {
         offset = DPS_INVALID_LSN_OFFSET ;
         version = DPS_INVALID_LSN_VERSION ;
      }

      INT32 compareVersion( const DPS_LSN_VER &version ) const
      {
         INT32 rc = 0 ;
         if ( DPS_INVALID_LSN_VERSION == this->version &&
              DPS_INVALID_LSN_VERSION == version )
         {
            goto done ;
         }
         else if ( DPS_INVALID_LSN_VERSION != this->version &&
                   DPS_INVALID_LSN_VERSION == version )
         {
            rc = 1 ;
            goto done ;
         }
         else if ( DPS_INVALID_LSN_VERSION == this->version &&
                   DPS_INVALID_LSN_VERSION != version )
         {
            rc = -1 ;
            goto done ;
         }
         else if ( this->version < version )
         {
            rc = -1 ;
            goto done ;
         }
         else if ( this->version > version )
         {
            rc = 1 ;
            goto done ;
         }
         else
         {
            goto done ;
         }
      done:
         return rc ;
      }

      INT32 compareOffset( const DPS_LSN_OFFSET &offset ) const
      {
         INT32 rc = 0 ;
         if ( DPS_INVALID_LSN_OFFSET == this->offset &&
              DPS_INVALID_LSN_OFFSET == offset )
         {
            goto done ;
         }
         else if ( DPS_INVALID_LSN_OFFSET != this->offset &&
                   DPS_INVALID_LSN_OFFSET == offset )
         {
            rc = 1 ;
            goto done ;
         }
         else if ( DPS_INVALID_LSN_OFFSET == this->offset &&
                   DPS_INVALID_LSN_OFFSET != offset )
         {
            rc = -1 ;
            goto done ;
         }
         else if ( this->offset < offset )
         {
            rc = -1 ;
            goto done ;
         }
         else if ( this->offset > offset )
         {
            rc = 1 ;
            goto done ;
         }
         else
         {
            goto done ;
         }
      done:
         return rc ;
      }

      INT32 compareOffset( const DPS_LSN &lsn ) const
      {
         return compareOffset( lsn.offset ) ;
      }

      /// 0 means this = lsn
      /// < 0 means this < lsn
      /// > 0 means this > lsn
      INT32 compare( const DPS_LSN &lsn ) const
      {
         INT32 rc = 0 ;
         rc = compareVersion( lsn.version ) ;
         if ( 0 != rc )
         {
            goto done ;
         }
         else
         {
            rc = compareOffset( lsn.offset ) ;
            goto done ;
         }
      done:
         return rc ;
      }
   } ;
}

#endif

