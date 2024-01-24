/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = IDataSource.hpp

   Descriptive Name = Data source interfaces.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IDATASOURCE_HPP__
#define IDATASOURCE_HPP__

#include "oss.hpp"
#include "utilUniqueID.hpp"
#include "utilAddress.hpp"
#include "coordDef.hpp"
#include "../bson/bson.hpp"

#include <boost/shared_ptr.hpp>

using namespace bson ;

#define DATASOURCE_MAX_NAME_SZ            127
#define DATASOURCE_GROUPID_MASK           0x80000000

#define SDB_GROUPID_2_DSID(gid)           ( (gid) ^ DATASOURCE_GROUPID_MASK )
#define SDB_DSID_2_GROUPID(dsid)          ( (dsid) | DATASOURCE_GROUPID_MASK )

#define SDB_IS_DSID(id)                   ( (id) & DATASOURCE_GROUPID_MASK )

// Data source access modes.
#define DS_ACCESS_DATA_READONLY      0x00000001
#define DS_ACCESS_DATA_WRITEONLY     0x00000002
#define DS_ACCESS_DATA_READWRITE     (DS_ACCESS_DATA_READONLY | DS_ACCESS_DATA_WRITEONLY)
#define DS_ACCESS_DATA_NONE          0
#define DS_ACCESS_DEFAULT            DS_ACCESS_DATA_READWRITE

#define DS_ERR_FILTER_READ       0x00000001
#define DS_ERR_FILTER_WRITE      0x00000002
#define DS_ERR_FILTER_ALL        (DS_ERR_FILTER_READ | DS_ERR_FILTER_WRITE)
#define DS_ERR_FILTER_NONE       0

#define DS_ACCESS_MODE_2_DESC( mode, descPtr )  \
do {                                            \
   switch ( mode ) {                            \
   case DS_ACCESS_DATA_READONLY:                \
      descPtr = "READ" ;                        \
      break ;                                   \
   case DS_ACCESS_DATA_WRITEONLY:               \
      desc = "WRITE" ;                          \
      break ;                                   \
   case DS_ACCESS_DATA_READWRITE:               \
      desc = "READ|WRITE" ;                     \
      break ;                                   \
   case DS_ACCESS_DATA_NONE:                    \
      desc = "NONE" ;                           \
      break ;                                   \
   default:                                     \
      desc = "" ;                               \
      break ;                                   \
   }                                            \
} while ( 0 )

#define DS_ERR_FILTER_2_DESC( filter, descPtr ) \
do {                                            \
   switch ( filter ) {                          \
   case DS_ERR_FILTER_READ:                     \
      descPtr = "READ" ;                        \
      break ;                                   \
   case DS_ERR_FILTER_WRITE:                    \
      descPtr = "WRITE" ;                       \
      break ;                                   \
   case DS_ERR_FILTER_ALL:                      \
      descPtr = "READ|WRITE" ;                  \
      break ;                                   \
   case DS_ERR_FILTER_NONE:                     \
      descPtr = "NONE" ;                        \
      break ;                                   \
   default:                                     \
      descPtr = "" ;                            \
      break ;                                   \
   }                                            \
}while ( 0 )

namespace engine
{

   /*
      SDB_DS_TYPE define
   */
   enum SDB_DS_TYPE
   {
      SDB_DS_SDB     = 1,

      SDB_DS_MAX
   } ;

   /**
    * Transaction propagation mode for data source. As transaction operations
    * are not allowed on data source, the action is relied on this mode.
    */
   enum SDB_DS_TRANS_PROPAGATE_MODE
   {
      DS_TRANS_PROPAGATE_INVALID,
      DS_TRANS_PROPAGATE_NEVER,        // Report error for any transaction
                                       // operation on data source.
      DS_TRANS_PROPAGATE_NOT_SUPPORT   // Do not report error directly, but
                                       // degrade to non transactioanl
                                       // operations and send to data source.
   } ;

   /*
      Data source type name define
   */
   #define SDB_DATASOURCE_TYPE_NAME          "sequoiadb"
   #define COORD_DS_ROUTE_SERVCIE            MSG_ROUTE_SHARD_SERVCIE

   /*
      _IDataSource define
   */
   class _IDataSource : public SDBObject
   {
   public:
      _IDataSource() {}
      virtual ~_IDataSource() {}

      virtual INT32 init( const BSONObj &obj ) = 0 ;

      /**
       * Type name of the data source. Currently only 'SequoiaDB' is supported.
       * @return
       */
      virtual const CHAR* getTypeName() const = 0 ;
      virtual SDB_DS_TYPE getType() const = 0 ;

      /**
       * Get id of the data source.
       * @return
       */
      virtual UTIL_DS_UID getID() const = 0 ;

      /**
       * Get name of the data source.
       * @return
       */
      virtual const CHAR* getName() const = 0 ;

      virtual const CHAR* getAddress() const = 0 ;

      /**
       * Get address list of the data source.
       * @return
       */
      virtual const utilAddrContainer* getAddressList() const = 0 ;
      virtual CoordGroupInfoPtr getGroupInfo() const = 0 ;

      /**
       * Get user of the data source.
       * @return
       */
      virtual const CHAR* getUser() const = 0 ;

      virtual const CHAR* getPassword() const = 0 ;


      virtual const CHAR* getErrCtlLevel() const = 0 ;

      /**
       * Get version of the data source.
       * @return
       */
      virtual const CHAR *getDSVersion() const = 0 ;

      virtual INT32 getDSMajorVersion() const = 0 ;
      virtual INT32 getDSMinorVersion() const = 0 ;
      virtual INT32 getDSFixVersion() const = 0 ;

      virtual BOOLEAN isReadable() const = 0 ;
      virtual BOOLEAN isWritable() const = 0 ;

      virtual SDB_DS_TRANS_PROPAGATE_MODE getTransPropagateMode() const = 0 ;

      virtual BOOLEAN inheritSessionAttr() const = 0 ;

      /**
       * Get error filter mask of the data source.
       * @return
       */
      virtual INT32 getErrFilterMask() const = 0 ;
   } ;
   typedef _IDataSource IDataSource ;

   typedef boost::shared_ptr<IDataSource>          CoordDataSourcePtr ;
}

#endif /* IDATASOURCE_HPP__ */
