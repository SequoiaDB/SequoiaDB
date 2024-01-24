/*******************************************************************************

   Copyright (C) 2023-present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilGlobalID.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          11/24/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_GLOBALID_HPP_
#define UTIL_GLOBALID_HPP_

#include "ossTypes.hpp"

namespace engine
{
   /* GlobalID in catalog SYSINFO.SYSDCBASE */
   /* It will not be initialized until the first sequence be created
      cause only sequence use the GlobalID as sequence ID right now */
   typedef UINT64 utilGlobalID ;

   typedef utilGlobalID utilSequenceID ;
   typedef utilGlobalID utilRecycleID ;
 
   #define UTIL_GLOGALID_MAX         OSS_UINT64_MAX
 
   #define UTIL_GLOBAL_NULL          0
   #define UTIL_SEQUENCEID_NULL      UTIL_GLOBAL_NULL
   #define UTIL_RECYCLEID_NULL       UTIL_GLOBAL_NULL
}

#endif //UTIL_UNIQUEID_HPP_


