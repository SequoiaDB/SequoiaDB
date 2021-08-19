/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = utilUniqueID.hpp

   Descriptive Name =

   When/how to use: Process CS/CL Unique ID

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          05/24/2018  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_UNIQUEID_HPP_
#define UTIL_UNIQUEID_HPP_

#include "ossTypes.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"

using namespace bson ;

namespace engine
{
   typedef UINT32 utilCSUniqueID ;
   typedef UINT64 utilCLUniqueID ;
   typedef UINT32 utilCLInnerID ;

   /// cs unique id, valid values range from 1 to 4294967040
   #define UTIL_CSUNIQUEID_MAX       0xFFFFFF00
   /// cl unique id (64bit) = cs unqiue id (32bit) + cl inner id (32bit)
   /// cl inner id: valid values range from 1 to u4294967040
   #define UTIL_CLINNERID_MAX        0xFFFFFF00

   /// Before version 3.0.1, cs/cl has not its unique id. After the upgrade
   /// to version 3.0.1+, unique id will be set. But if the cs only exists
   /// on data, doesn't exists on catalog, the unique id will be 0xFFFFFFFF.
   /// While if cl only exists on data, doesn't exists on catalog, the cl inner
   /// id will be 0. In addition, system cs/cl unique id is 0.
   #define UTIL_UNIQUEID_NULL        0

   /// Directly connect data node, then create cs/cl
   #define UTIL_CSUNIQUEID_LOCAL     0xFFFFFFFF
   #define UTIL_CLINNERID_LOCAL      0xFFFFFFFF
   #define UTIL_CLUNIQUEID_LOCAL     0xFFFFFFFFFFFFFFFF

   /// coord.loadCS(), if the cl of loaded cs does not exist in the catalog,
   /// the cl inner id will be 0xFFFFFFFE.
   #define UTIL_CSUNIQUEID_LOADCS    0xFFFFFFFE
   #define UTIL_CLINNERID_LOADCS     0xFFFFFFFE


   #define UTIL_IS_VALID_CSUNIQUEID( id )      \
      ( ( id != UTIL_UNIQUEID_NULL ) &&        \
        ( id != UTIL_CSUNIQUEID_LOCAL ) &&     \
        ( id != UTIL_CSUNIQUEID_LOADCS ) )

   #define UTIL_IS_VALID_CLUNIQUEID( id )                       \
      ( ( utilGetCLInnerID( id ) != UTIL_UNIQUEID_NULL ) &&     \
        ( utilGetCLInnerID( id ) != UTIL_CLINNERID_LOCAL ) &&   \
        ( utilGetCLInnerID( id ) != UTIL_CLINNERID_LOADCS ) )

   OSS_INLINE utilCSUniqueID utilGetCSUniqueID( utilCLUniqueID clUniqueID )
   {
      return clUniqueID >> 32 ;
   }

   OSS_INLINE utilCLInnerID utilGetCLInnerID( utilCLUniqueID clUniqueID )
   {
      return (utilCLInnerID)clUniqueID ;
   }

   OSS_INLINE utilCLUniqueID utilBuildCLUniqueID( utilCSUniqueID csUniqueID,
                                                  utilCLInnerID clInnerID )
   {
      return ossPack32To64( csUniqueID, clInnerID ) ;
   }

   typedef std::pair<std::string, utilCLUniqueID> PAIR_CLNAME_ID ;

   std::string utilClNameId2Str( std::vector< PAIR_CLNAME_ID > clInfoList ) ;

   typedef std::map<std::string, utilCLUniqueID> MAP_CLNAME_ID ;

   typedef std::map<utilCLUniqueID, std::string> MAP_CLID_NAME ;

   MAP_CLNAME_ID utilBson2ClNameId( const BSONObj& clInfoObj ) ;

   MAP_CLID_NAME utilBson2ClIdName( const BSONObj& clInfoObj ) ;

   BSONObj utilSetUniqueID( const BSONObj& clInfoObj,
                            utilCLUniqueID setValue = UTIL_UNIQUEID_NULL ) ;
}

#endif //UTIL_UNIQUEID_HPP_

