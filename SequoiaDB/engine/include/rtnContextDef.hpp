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

   Source File Name = rtnContextDef.hpp

   Descriptive Name = RunTime Context Define Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/11/2021  HGM Initial draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_CONTEXT_DEF_HPP_
#define RTN_CONTEXT_DEF_HPP_

#include "ossTypes.hpp"

namespace engine
{

   #define RTN_CONTEXT_GETNUM_ONCE     (1000)

   // max context number in a node
   #define RTN_MAX_CTX_NUM_MIN         ( 1000 )
   #define RTN_MAX_CTX_NUM_MAX         ( OSS_SINT32_MAX )
   #define RTN_MAX_CTX_NUM_DFT         ( 100000 )

   // max context number in a session per node
   #define RTN_MAX_SESS_CTX_NUM_MIN    ( 10 )
   #define RTN_MAX_SESS_CTX_NUM_MAX    ( OSS_SINT32_MAX )
   #define RTN_MAX_SESS_CTX_NUM_DFT    ( 100 )

   // context idle timeout in minutes
   #define RTN_CTX_TIMEOUT_MIN         ( 0 )
   #define RTN_CTX_TIMEOUT_MAX         ( OSS_SINT32_MAX )
   // default is 1440 minutes ( about one day )
   #define RTN_CTX_TIMEOUT_DFT         ( 1440 )

   // 60 seconds ( in microseconds )
   #define RTN_CTX_CHECK_INTERVAL      ( 60000000 )

}

#endif // RTN_CONTEXT_DEF_HPP_
