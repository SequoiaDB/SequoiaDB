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

   Source File Name = utilReplSizePlan.hpp

   Descriptive Name =

   When/how to use: write operation

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          28/11/2022  YJF Initial Draft

   Last Changed =

******************************************************************************/

#ifndef UTIL_REPL_SIZE_PLAN_HPP__
#define UTIL_REPL_SIZE_PLAN_HPP__

#include "ossTypes.h"
#include "dpsDef.hpp"

namespace engine
{

   /*
      _utilReplSizePlan define
   */
   struct _utilReplSizePlan
   {
      DPS_LSN_OFFSET offset ;
      mutable UINT8  affinitiveLocations ;
      mutable UINT8  primaryLocationNodes ;
      mutable UINT8  locations ;
      mutable UINT8  affinitiveNodes ;

      _utilReplSizePlan()
      {
         reset();
      }

      _utilReplSizePlan( const DPS_LSN_OFFSET offset,
                         const UINT8 affinitiveLocations,
                         const UINT8 primaryLocationNodes,
                         const UINT8 locations,
                         const UINT8 affinitiveNodes )
      {
         this->offset = offset ;
         this->affinitiveLocations = affinitiveLocations ;
         this->primaryLocationNodes = primaryLocationNodes ;
         this->locations = locations ;
         this->affinitiveNodes = affinitiveNodes ;
      }

      ~_utilReplSizePlan()
      {
      }

      void reset()
      {
         offset = DPS_INVALID_LSN_OFFSET ;
         affinitiveLocations = 0 ;
         primaryLocationNodes = 0 ;
         locations = 0 ;
         affinitiveNodes = 0 ;
      }

      void setNodeReplSizePlan( const UINT8 nodes,
                                const UINT8 affinitiveNodes )
      {
         reset() ;
         this->affinitiveNodes = OSS_MIN( nodes, affinitiveNodes ) ;
      }

      void setLocMajorReplSizePlan( const UINT8 nodes,
                                    const UINT8 affinitiveLocations,
                                    const UINT8 primaryLocationNodes,
                                    const UINT8 locations,
                                    const UINT8 affinitiveNodes )
      {
         offset = DPS_INVALID_LSN_OFFSET ;
         this->locations = OSS_MIN( nodes, ( locations + 1 ) / 2 ) ;
         this->affinitiveLocations
                  = OSS_MIN( this->locations, affinitiveLocations ) ;
         this->primaryLocationNodes
                  = OSS_MIN( nodes - this->locations,
                             ( primaryLocationNodes + 1 ) / 2 ) ;
         this->affinitiveNodes = OSS_MIN( nodes, affinitiveNodes ) ;
      }

      void setPryLocMajorReplSizePlan( const UINT8 nodes,
                                       const UINT8 affinitiveLocations,
                                       const UINT8 primaryLocationNodes,
                                       const UINT8 locations,
                                       const UINT8 affinitiveNodes )
      {
         offset = DPS_INVALID_LSN_OFFSET ;
         this->primaryLocationNodes
                  = OSS_MIN( nodes, ( primaryLocationNodes + 1 ) / 2 ) ;
         this->locations
                  = OSS_MIN( nodes - this->primaryLocationNodes,
                             ( locations + 1 ) / 2 ) ;
         this->affinitiveLocations
                  = OSS_MIN( this->locations, affinitiveLocations ) ;
         this->affinitiveNodes = OSS_MIN( nodes, affinitiveNodes ) ;
      }

      BOOLEAN isPassed( const _utilReplSizePlan &item ) const
      {
         // if current session don't need to wait, the LSN of
         // current session must be less than expect LSN
         return offset < item.offset &&
                primaryLocationNodes <= item.primaryLocationNodes &&
                affinitiveLocations <= item.affinitiveLocations &&
                locations <= item.locations &&
                affinitiveNodes <= item.affinitiveNodes ;
      }

      BOOLEAN operator <=( const _utilReplSizePlan &right ) const
      {
         BOOLEAN result = FALSE ;
         if ( right.offset < offset )
         {
            /// do nothting
         }
         else if ( right.offset > offset )
         {
            result = TRUE ;
         }
         else if ( right.primaryLocationNodes > primaryLocationNodes )
         {
            result = TRUE ;
         }
         else if ( right.affinitiveLocations > affinitiveLocations )
         {
            result = TRUE ;
         }
         else if ( right.locations > locations )
         {
            result = TRUE ;
         }
         else if ( right.affinitiveNodes > affinitiveNodes )
         {
            result = TRUE ;
         }
         else
         {
            result = TRUE ;
         }
         return result ;
      }

      BOOLEAN operator <( const _utilReplSizePlan &right  ) const
      {
         BOOLEAN result = FALSE ;
         if ( right.offset < offset )
         {
            /// do nothting
         }
         else if ( right.offset > offset )
         {
            result = TRUE ;
         }
         else if ( right.primaryLocationNodes > primaryLocationNodes )
         {
            result = TRUE ;
         }
         else if ( right.affinitiveLocations > affinitiveLocations )
         {
            result = TRUE ;
         }
         else if ( right.locations > locations )
         {
            result = TRUE ;
         }
         else if ( right.affinitiveNodes > affinitiveNodes )
         {
            result = TRUE ;
         }
         return result ;
      }

      BOOLEAN operator ==( const _utilReplSizePlan &right  ) const
      {
         return ( right.offset == offset ) &&
                ( right.primaryLocationNodes == primaryLocationNodes ) &&
                ( right.affinitiveLocations == affinitiveLocations ) &&
                ( right.locations == locations ) &&
                ( right.affinitiveNodes == affinitiveNodes ) ;
      }

      BOOLEAN operator >=( const _utilReplSizePlan &right  ) const
      {
         return right <= *this ;
      }

      BOOLEAN operator >( const _utilReplSizePlan &right  ) const
      {
         return right < *this ;
      }

   } ;
   typedef _utilReplSizePlan utilReplSizePlan ;
}

#endif // UTIL_REPL_SIZE_PLAN_HPP__

