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

   Source File Name = rtnUserCache.cpp

   Descriptive Name = 

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for update
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/25/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnUserCache.hpp"
#include "clsMgr.hpp"
#include "clsResourceContainer.hpp"
#include "coordCommandRole.hpp"
#include "coordResource.hpp"
#include "msgDef.h"
#include "authDef.hpp"
#include "rtn.hpp"
#include "rtnContextBuff.hpp"
using namespace bson;

namespace engine
{
   const INT32 RTN_GET_USER_RETYY_TIMES = 4;

   INT32 _rtnUserCache::getACL( pmdEDUCB *cb, const KEY_TYPE &userName, VALUE_TYPE &acl )
   {
      INT32 rc = SDB_OK;
      acl.reset();
      try
      {
         acl = _get( userName );
         if ( acl )
         {
            goto done;
         }

         {
            ossScopedLock lock( &_fetchLatch, EXCLUSIVE );
            acl = _get( userName );
            if ( acl )
            {
               goto done;
            }

            VALUE_TYPE fetched;
            rc = _fetch( cb, userName, fetched );
            PD_RC_CHECK( rc, PDERROR, "Failed to fetch ACL for user: %s", userName.c_str() );
            SDB_ASSERT( fetched, "Fetched ACL can't be NULL" );

            std::pair< DATA_TYPE::iterator, bool > r = _insert( userName, fetched );
            if ( !r.second )
            {
               PD_LOG( PDERROR, "Failed to insert ACL to map for user: %s", userName.c_str() );
               rc = SDB_SYS;
               goto error;
            }
            acl = fetched;
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e );
         PD_LOG( PDERROR, "Exception occurred: %s", e.what() );
         goto error;
      }
      catch ( boost::exception &e )
      {
         rc = SDB_SYS;
         PD_LOG( PDERROR, "Exception occurred: %s", boost::diagnostic_information( e ).c_str() );
         goto error;
      }

   done:
      return rc;
   error:
      acl.reset();
      goto done;
   }

   _rtnUserCache::VALUE_TYPE _rtnUserCache::_get( const KEY_TYPE &userName )
   {
      ossScopedLock lock( &_latch, SHARED );
      DATA_TYPE::iterator it = _data.find( userName );
      if ( it != _data.end() )
      {
         return it->second;
      }
      else
      {
         return VALUE_TYPE();
      }
   }

   std::pair< _rtnUserCache::DATA_TYPE::iterator, bool > _rtnUserCache::_insert(
      const KEY_TYPE &userName,
      const VALUE_TYPE &acl )
   {
      ossScopedLock lock( &_latch, EXCLUSIVE );
      return _data.insert( std::make_pair( userName, acl ) );
   }

   INT32 _rtnUserCache::_fetchForCoord( pmdEDUCB *cb,
                                        const KEY_TYPE &userName,
                                        const CHAR *pMsgBuffer,
                                        BSONObj &privsObj )
   {
      INT32 rc = SDB_OK;
      INT64 contextID = -1;
      rtnContextBuf buf;
      coordResource *pResource = sdbGetResourceContainer()->getResource();
      _coordCMDGetUser opr;
      BSONObj userInfo;

      rc = opr.init( pResource, cb );
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init operator[%s], rc: %d", opr.getName(), rc );
         goto error;
      }

      rc = opr.execute( (MsgHeader *)pMsgBuffer, cb, contextID, &buf );
      PD_RC_CHECK( rc, PDERROR, "Failed to execute get user, rc: %d", rc );

      rc = rtnGetMore( contextID, 1, buf, cb, sdbGetRTNCB() );
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_AUTH_USER_NOT_EXIST;
         contextID = -1;
         PD_LOG( PDERROR, "User[%s] doesn't exist, rc: %d", userName.c_str(), rc );
         goto error;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Get more record from context[%lld] failed[%d]", contextID, rc );
         contextID = -1;
         goto error;
      }

      rc = buf.nextObj( userInfo );
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get user, rc: %d", rc );
         goto error;
      }

      privsObj = userInfo.getObjectField( AUTH_FIELD_NAME_INHERITED_PRIVILEGES ).getOwned();
   done:
      if ( -1 != contextID )
      {
         sdbGetRTNCB()->contextDelete( contextID, cb );
      }
      return rc;
   error:
      goto done;
   }

   INT32 _rtnUserCache::_fetch( pmdEDUCB *cb, const KEY_TYPE &userName, VALUE_TYPE &acl )
   {
      INT32 rc = SDB_OK;
      CHAR *pBuffer = NULL;
      INT32 buffSize = 0;
      BSONObj privsObj;
      boost::shared_ptr< authAccessControlList > fetched;

      BSONObj queryObj =
         BSON( FIELD_NAME_USER << userName.c_str() << AUTH_FIELD_NAME_SHOW_PRIVILEGES << true );

      acl.reset();

      rc = msgBuildQueryMsg( &pBuffer, &buffSize, CMD_ADMIN_PREFIX CMD_NAME_GET_USER, 0, 0, 0, 1,
                             &queryObj, NULL, NULL, NULL, cb );
      PD_RC_CHECK( rc, PDERROR, "Failed to build query msg, rc: %d", rc );
      if ( SDB_ROLE_COORD == pmdGetDBRole() )
      {
         rc = _fetchForCoord( cb, userName, pBuffer, privsObj );
         PD_RC_CHECK( rc, PDERROR, "Failed to fetch acl, rc: %d", rc );
      }
      else
      {
         MsgHeader *pRecv = NULL;
         shardCB *pShard = sdbGetShardCB();
         UINT32 retryTimes = 0;
         MsgOpReply replyHeader;

         replyHeader.contextID = -1;
         replyHeader.numReturned = 0;

         while ( TRUE )
         {
            ++retryTimes;
            rc = pShard->syncSend( (MsgHeader *)pBuffer, CATALOG_GROUPID, TRUE, &pRecv );
            if ( SDB_OK != rc )
            {
               rc = pShard->syncSend( (MsgHeader *)pBuffer, CATALOG_GROUPID, FALSE, &pRecv );
               PD_RC_CHECK( rc, PDERROR, "Failed to send get user req to catalog, rc: %d", rc );
            }
            if ( !pRecv )
            {
               rc = SDB_SYS;
               PD_LOG( PDERROR, "Syncsend return ok but res is NULL" );
               goto error;
            }
            rc = MSG_GET_INNER_REPLY_RC( pRecv );
            replyHeader.flags = rc;
            replyHeader.startFrom = MSG_GET_INNER_REPLY_STARTFROM( pRecv );
            ossMemcpy( &( replyHeader.header ), pRecv, sizeof( MsgHeader ) );
            if ( SDB_OK == rc && pRecv->messageLength > (INT32)sizeof( MsgOpReply ) )
            {
               try
               {
                  const CHAR *data = (const CHAR *)pRecv + sizeof( MsgOpReply );
                  privsObj = BSONObj( data )
                                .getObjectField( AUTH_FIELD_NAME_INHERITED_PRIVILEGES )
                                .getOwned();
               }
               catch ( std::exception &e )
               {
                  rc = SDB_OOM;
                  PD_LOG( PDERROR, "Exception occurred: %s", e.what() );
                  /// release recv msg before goto error
                  SDB_OSS_FREE( pRecv );
                  pRecv = NULL;
                  goto error;
               }
            }

            SDB_OSS_FREE( pRecv );
            pRecv = NULL;
            if ( SDB_CLS_NOT_PRIMARY == rc && retryTimes < RTN_GET_USER_RETYY_TIMES )
            {
               INT32 rcTmp = SDB_OK;
               rcTmp = pShard->updatePrimaryByReply( &( replyHeader.header ) );
               if ( SDB_NET_CANNOT_CONNECT == rcTmp )
               {
                  /// the node is crashed, sleep some seconds
                  PD_LOG( PDWARNING,
                          "Catalog group primary node is crashed "
                          "but other nodes not aware, sleep %d seconds",
                          NET_NODE_FAULTUP_MIN_TIME );
                  ossSleep( NET_NODE_FAULTUP_MIN_TIME * OSS_ONE_SEC );
               }

               if ( rcTmp )
               {
                  pShard->updateCatGroup( CLS_SHARD_TIMEOUT );
               }
               continue;
            }
            else if ( rc )
            {
               PD_LOG( PDERROR, "Failed to get ACL for user %s, rc: %d", userName.c_str(), rc );
               goto error;
            }
            else
            {
               break;
            }
         }
      }

      fetched = boost::make_shared< authAccessControlList >();
      if ( !fetched )
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "Failed to allocate memory for ACL" );
         goto error;
      }
      for ( BSONObjIterator it( privsObj ); it.more(); )
      {
         BSONElement ele = it.next();
         rc = fetched->addPrivilege( ele.Obj() );
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to add privilege, rc: %d", rc );
            goto error;
         }
      }

      acl = fetched;

   done:
      if ( pBuffer )
      {
         msgReleaseBuffer( pBuffer, cb );
      }
      return rc;
   error:
      acl.reset();
      goto done;
   }

   void _rtnUserCache::remove( const KEY_TYPE &userName )
   {
      ossScopedLock lock( &_latch, EXCLUSIVE );
      _data.erase( userName );
   }

   void _rtnUserCache::clear()
   {
      ossScopedLock lock( &_latch, EXCLUSIVE );
      _data.clear();
   }
} // namespace engine