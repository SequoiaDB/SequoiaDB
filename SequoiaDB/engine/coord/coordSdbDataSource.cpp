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

   Source File Name = coordSdbDataSource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/02/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#include "coordSdbDataSource.hpp"
#include "msgDef.hpp"
#include "msgCatalog.hpp"
#include "pd.hpp"
#include "utilDataSource.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordSdbDataSource implement
   */
   _coordSdbDataSource::_coordSdbDataSource()
   : _id( UTIL_INVALID_DS_UID ),
     _accessMode( DS_ACCESS_DEFAULT ),
     _dsMajorVersion( 0 ),
     _dsMinorVersion( 0 ),
     _dsFixVersion( 0 ),
     _errFilterMask( DS_ERR_FILTER_NONE ),
     _transPropagateMode( DS_TRANS_PROPAGATE_NEVER ),
     _inheritSessionAttr( FALSE )
   {
      ossMemset( _name, 0, DATASOURCE_MAX_NAME_SZ + 1 ) ;
      ossMemset( _user, 0, SDB_MAX_USERNAME_LENGTH + 1 ) ;
      ossMemset( _password, 0, SDB_MAX_PASSWORD_LENGTH + 1 ) ;
      ossMemset( _errCtlLevel, 0, sizeof( _errCtlLevel ) ) ;
   }

   _coordSdbDataSource::~_coordSdbDataSource()
   {
   }

   INT32 _coordSdbDataSource::init( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      CoordGroupInfo *pGroupInfo = NULL ;

      try
      {
         BSONObj objGroup ;
         BSONObjIterator itr( obj ) ;
         BSONElement ele ;

         while ( itr.more() )
         {
            ele = itr.next() ;

            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ID ) )
            {
               _id = (UTIL_DS_UID)ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_NAME ) )
            {
               ossStrncpy( _name, ele.valuestr(), DATASOURCE_MAX_NAME_SZ ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ADDRESS ) )
            {
               _address = ele.poolStr() ;
               rc = utilParseAddrList( _address.c_str(), _addrArray ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Parse address list[%s] failed, rc: %d",
                          _address.c_str(), rc ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_USER ) )
            {
               ossStrncpy( _user, ele.valuestr(), SDB_MAX_USERNAME_LENGTH ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_PASSWD ) )
            {
               ossStrncpy( _password, ele.valuestr(), SDB_MAX_PASSWORD_LENGTH ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_ERRORCTLLEVEL ) )
            {
               ossStrncpy( _errCtlLevel, ele.valuestr(),
                           sizeof(_errCtlLevel) - 1 ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_ACCESSMODE ) )
            {
               _accessMode = (UINT32)ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_DSVERSION ) )
            {
               _dsVersion = ele.poolStr() ;
               ossSscanf( ele.valuestr(), "%d.%d.%d",
                          &_dsMajorVersion,
                          &_dsMinorVersion,
                          &_dsFixVersion ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_ERRORFILTERMASK ) )
            {
               _errFilterMask = ele.numberInt() ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_TRANS_PROPAGATE_MODE ) )
            {
               _transPropagateMode = sdbDSTransModeFromDesc( ele.valuestr() ) ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(),
                                      FIELD_NAME_INHERIT_SESSION_ATTR ) )
            {
               _inheritSessionAttr = ele.boolean() ;
            }
         }

         rc = _buildGroupInfo( objGroup ) ;
         if ( rc )
         {
            goto error;
         }

         /// Create group info
         pGroupInfo = SDB_OSS_NEW CoordGroupInfo( SDB_DSID_2_GROUPID( _id ) ) ;
         if ( NULL == pGroupInfo )
         {
            rc = SDB_OOM ;
            PD_LOG ( PDERROR, "Alloc group info failed" ) ;
            goto error ;
         }

         rc = pGroupInfo->updateGroupItem( objGroup ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Update group info from bson[%s] failed, "
                    "rc: %d", objGroup.toString().c_str(), rc ) ;
            goto error ;
         }

         _groupInfoPtr = CoordGroupInfoPtr( pGroupInfo ) ;
         pGroupInfo = NULL ;
      }
      catch ( exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

   done:
      if ( pGroupInfo )
      {
         SDB_OSS_DEL pGroupInfo ;
      }
      return rc ;
   error:
      goto done ;
   }

   const utilAddrContainer* _coordSdbDataSource::getAddressList() const
   {
      return &_addrArray ;
   }

   CoordGroupInfoPtr _coordSdbDataSource::getGroupInfo() const
   {
      return _groupInfoPtr ;
   }

   const CHAR* _coordSdbDataSource::getAddress() const
   {
      return _address.c_str() ;
   }

   const CHAR* _coordSdbDataSource::getDSVersion() const
   {
      return _dsVersion.c_str() ;
   }

   INT32 _coordSdbDataSource::getDSMajorVersion() const
   {
      return _dsMajorVersion ;
   }

   INT32 _coordSdbDataSource::getDSMinorVersion() const
   {
      return _dsMinorVersion ;
   }

   INT32 _coordSdbDataSource::getDSFixVersion() const
   {
      return _dsFixVersion ;
   }

   INT32 _coordSdbDataSource::getErrFilterMask() const
   {
      return _errFilterMask ;
   }

   BOOLEAN _coordSdbDataSource::isReadable() const
   {
      return _accessMode & DS_ACCESS_DATA_READONLY ;
   }

   BOOLEAN _coordSdbDataSource::isWritable() const
   {
      return _accessMode & DS_ACCESS_DATA_WRITEONLY ;
   }

   SDB_DS_TRANS_PROPAGATE_MODE _coordSdbDataSource::getTransPropagateMode() const
   {
      return _transPropagateMode ;
   }

   BOOLEAN _coordSdbDataSource::inheritSessionAttr() const
   {
      return _inheritSessionAttr ;
   }

   INT32 _coordSdbDataSource::_buildGroupInfo( BSONObj &objGroup )
   {
      INT32 rc = SDB_OK ;
      INT32 beginID = 1 ;

      if ( !_addrArray.empty() )
      {
         try
         {
            BSONObjBuilder builder( 1024 ) ;
            BSONObj objOMGroup ;

            utilAddrPair item ;
            const _utilArray<utilAddrPair> &addrs = _addrArray.getAddresses() ;

            /// GroupID
            builder.append( CAT_GROUPID_NAME, SDB_DSID_2_GROUPID( _id ) ) ;
            /// GroupName
            builder.append( CAT_GROUPNAME_NAME, "" ) ;
            /// Role
            builder.append( CAT_ROLE_NAME, SDB_ROLE_COORD ) ;
            /// Secret ID
            builder.append( FIELD_NAME_SECRETID, (INT32)0 ) ;
            /// Version
            builder.append( CAT_VERSION_NAME, (INT32)1 ) ;
            /// Group:[{},{}...]
            BSONArrayBuilder arrGroup( builder.subarrayStart( CAT_GROUP_NAME ) ) ;

            for ( UINT32 i = 0; i < addrs.size(); ++i )
            {
               item = addrs[ i ] ;
               BSONObjBuilder node( arrGroup.subobjStart() ) ;
               /// NodeID
               node.append( CAT_NODEID_NAME, beginID++ ) ;
               /// HostName
               node.append( CAT_HOST_FIELD_NAME, item.getHost() ) ;
               /// Status
               node.append( CAT_STATUS_NAME, (INT32)SDB_CAT_GRP_ACTIVE ) ;
               /// Service:[{},{}...]
               BSONArrayBuilder arrSvc( node.subarrayStart(
                                        CAT_SERVICE_FIELD_NAME ) ) ;
               BSONObjBuilder svc( arrSvc.subobjStart() ) ;
               /// Type
               svc.append( CAT_SERVICE_TYPE_FIELD_NAME,
                           (INT32)COORD_DS_ROUTE_SERVCIE ) ;
               /// Name
               svc.append( CAT_SERVICE_NAME_FIELD_NAME,
                           item.getService() ) ;

               svc.done() ;
               arrSvc.done() ;
               /// End Service
               node.done() ;
            }
            arrGroup.done() ;
            /// End Group

            objGroup = builder.obj() ;
         }
         catch( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG ( PDERROR, "Occur exception: %s", e.what() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
