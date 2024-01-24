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
   typedef UINT64 utilIdxUniqueID ;
   typedef UINT32 utilIdxInnerID ;

   /// cs unique id, valid values range from 1 to 4294967040
   #define UTIL_CSUNIQUEID_MAX       0x7FFFFFFF
   /// cl unique id (64bit) = cs unqiue id (32bit) + cl inner id (32bit)
   /// cl inner id: valid values range from 1 to 4294967040
   #define UTIL_CLINNERID_MAX        0x7FFFFFFF

   #define UTIL_CSUNIQUEID_CAT_MIN   0xFFFFFF00
   #define UTIL_CSUNIQUEID_SYS_MIN   0xFFFFFFF0

   #define UTIL_UNIQUEID_LOCAL_BIT   0x80000000

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

   typedef UINT32                         UTIL_DS_UID ;
   #define UTIL_INVALID_DS_UID            0xFFFFFFFF

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

   BSONObj utilSetUniqueID( const BSONObj& clInfoObj,
                            utilCLUniqueID setValue = UTIL_UNIQUEID_NULL ) ;

   // idxUniqueID(64bit) = csUniqueID(32bit) + idxInnerID(32bit)
   // The first bit of idxInnerID indicates whether it is standalone index or
   // consistency index. The second bit of idxInnerID is reserved.
   #define UTIL_FLAG_STANDALONE_IDX 0x80000000

   #define UTIL_IDXINNERID_MAX      0x3FFFFFFF

   OSS_INLINE utilCSUniqueID utilGetCSUniqIDFromIdx( utilIdxUniqueID idxUniqueID )
   {
      return idxUniqueID >> 32 ;
   }

   OSS_INLINE utilIdxInnerID utilGetIdxInnerID( utilIdxUniqueID idxUniqueID )
   {
      utilIdxInnerID inId = (utilIdxInnerID)idxUniqueID ;
      OSS_BIT_CLEAR( inId, UTIL_FLAG_STANDALONE_IDX ) ;
      return inId ;
   }

   OSS_INLINE utilIdxInnerID utilGetIdxInnerIDWithFlag( utilIdxUniqueID idxUniqueID )
   {
      utilIdxInnerID inId = (utilIdxInnerID)idxUniqueID ;
      return inId ;
   }

   OSS_INLINE BOOLEAN utilIsStandaloneIdx( utilIdxUniqueID idxUniqueID )
   {
      utilIdxInnerID inId = (utilIdxInnerID)idxUniqueID ;
      return OSS_BIT_TEST( inId, UTIL_FLAG_STANDALONE_IDX ) ;
   }

   OSS_INLINE utilIdxUniqueID utilBuildIdxUniqueID( utilCSUniqueID csUniqueID,
                                                    utilIdxInnerID idxInnerID,
                                                    BOOLEAN isStandaloneIdx = FALSE )
   {
      if ( isStandaloneIdx )
      {
         OSS_BIT_SET( idxInnerID, UTIL_FLAG_STANDALONE_IDX ) ;
      }
      return ossPack32To64( csUniqueID, idxInnerID ) ;
   }

   OSS_INLINE BOOLEAN utilCheckIdxUniqueID( utilIdxUniqueID idxUniqueID,
                                            utilCSUniqueID csUniqueID )
   {

      return utilGetCSUniqIDFromIdx( idxUniqueID ) == csUniqueID ;
   }

   struct util_cmp_str
   {
      bool operator() (const char *a, const char *b) const
      {
         return std::strcmp(a,b)<0 ;
      }
   } ;
   typedef ossPoolMap<const CHAR*, BSONObj, util_cmp_str> MAP_IDXNAME_DEF ;
   typedef ossPoolMap<const CHAR*, MAP_IDXNAME_DEF, util_cmp_str> MAP_CLNAME_IDX ;

   INT32 utilBson2IdxNameId( const ossPoolVector<BSONObj>& idxInfoVec,
                             MAP_CLNAME_IDX& clMap ) ;

   // get bounds to match collections within a collection space with
   // specified unique ID
   INT32 utilGetCSBounds( const CHAR *fieldName,
                          utilCSUniqueID csUniqueID,
                          bson::BSONObjBuilder &builder ) ;

   // get bounds to match collections within a collection space with
   // specified unique ID
   INT32 utilGetCSBounds( const CHAR *fieldName,
                          utilCSUniqueID csUniqueID,
                          bson::BSONObj &matcher ) ;

}

#endif //UTIL_UNIQUEID_HPP_
