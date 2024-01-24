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

   Source File Name = pmdOptionsParse.hpp

   Descriptive Name = Pmd Options Parse Header

   When/how to use: this program may be used on binary and text-formatted
   versions of RunTime component. This file contains structure for RunTime
   control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==========================================
          08/07/2020  FangJiabin Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_OPTIONS_PARSE_HPP_
#define PMD_OPTIONS_PARSE_HPP_

#include "core.hpp"
#include "ossMemPool.hpp"

typedef enum PMD_PREFER_INSTANCE_TYPE
{
   // Note : secondary choice means lower priority than numeric instances
   PMD_PREFER_INSTANCE_TYPE_MASTER_SND  = -6, // master node in secondary choice
   PMD_PREFER_INSTANCE_TYPE_SLAVE_SND   = -5, // any slave node in secondary choice
   PMD_PREFER_INSTANCE_TYPE_ANYONE_SND  = -4, // any node( include master ) in secondary choice
   PMD_PREFER_INSTANCE_TYPE_MASTER      = -3, // master node
   PMD_PREFER_INSTANCE_TYPE_SLAVE       = -2, // any slave node
   PMD_PREFER_INSTANCE_TYPE_ANYONE      = -1, // any node( include master )
   PMD_PREFER_INSTANCE_TYPE_MIN         = 0,
   PMD_PREFER_INSTANCE_TYPE_UNKNOWN     = PMD_PREFER_INSTANCE_TYPE_MIN,
   PMD_PREFER_INSTANCE_TYPE_MAX         = 256
} PMD_PREFER_INSTANCE_TYPE ;

typedef enum PMD_PREFER_INSTANCE_MODE
{
   PMD_PREFER_INSTANCE_MODE_UNKNOWN = 0,
   PMD_PREFER_INSTANCE_MODE_RANDOM = 1,
   PMD_PREFER_INSTANCE_MODE_ORDERED,
} PMD_PREFER_INSTANCE_MODE ;

typedef enum PMD_PREFER_CONSTRAINT
{
   PMD_PREFER_CONSTRAINT_UNKNOWN = 0,
   PMD_PREFER_CONSTRAINT_NONE,
   PMD_PREFER_CONSTRAINT_PRY_ONLY,
   PMD_PREFER_CONSTRAINT_SND_ONLY
} PMD_PREFER_CONSTRAINT ;

namespace engine
{
   const CHAR* pmdPreferInstInt2String( PMD_PREFER_INSTANCE_TYPE preferInstInt ) ;

   INT32 pmdParsePreferInstStr( const CHAR *instanceStr,
                                ossPoolList< UINT8 > &instanceList,
                                PMD_PREFER_INSTANCE_TYPE &specInstance,
                                BOOLEAN &hasInvalidChar ) ;

   INT32 pmdParsePreferInstModeStr( const CHAR *instanceModeStr,
                                    PMD_PREFER_INSTANCE_MODE &instanceMode ) ;

   INT32 pmdParsePreferConstraintStr( const CHAR *constraintStr,
                                      PMD_PREFER_CONSTRAINT &constraint ) ;

   const CHAR* pmdGetConfigAliasName( const CHAR* config ) ;
}

#endif

