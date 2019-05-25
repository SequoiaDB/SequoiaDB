/******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = pd.hpp

   Descriptive Name = Problem Determination Header

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef NETDEF_HPP_
#define NETDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossSocket.hpp"
#include "msg.hpp"
#include <string>
#include <vector>

namespace engine
{
   typedef UINT32 NET_HANDLE ;

   const NET_HANDLE NET_INVALID_HANDLE = 0 ;

   typedef UINT32 NET_GROUP_ID ;
   typedef UINT32 NET_NODE_ID ;
   typedef UINT16 NET_SERVICE_ID ;

   #define NET_INVALID_TIMER_ID        ( 0 )

   /*
      _NET_NODE_STATUS define
   */
   typedef enum _NET_NODE_STATUS
   {
      NET_NODE_STAT_NORMAL = 0,
      NET_NODE_STAT_FULLSYNC,
      NET_NODE_STAT_OFFLINE,
      NET_NODE_STAT_REBUILD,
      NET_NODE_STAT_BACKUP,

      NET_NODE_STAT_UNKNOWN
   }NET_NODE_STATUS;

   NET_NODE_STATUS netResult2Status( INT32 result ) ;

   #define NET_NODE_FAULT_TIMEOUT            ( 600 )  /// second
   #define NET_NODE_FAULTUP_MIN_TIME         ( 3 )    /// second

   /*
      _netRouteNode define
   */
   class _netRouteNode : public SDBObject
   {
   public :
      CHAR        _host[OSS_MAX_HOSTNAME+1] ;
      std::string _service[MSG_ROUTE_SERVICE_TYPE_MAX] ;
      MsgRouteID  _id ;
      BOOLEAN     _isActive ;
      UINT8       _instanceID ;

   private:
      SINT32 _status;     // make sure the addr of _status is aligned 4 bytes,
      UINT64 _faultTime ; // fault time

   public:
      _netRouteNode()
      : _instanceID( NODE_INSTANCE_ID_UNKNOWN ),
        _status( NET_NODE_STAT_NORMAL ),
        _faultTime( 0 )
      {
         _id.value = MSG_INVALID_ROUTEID ;
         _isActive = TRUE ;
         _host[0] = 0 ;
      }
      _netRouteNode( const _netRouteNode &node )
      : _instanceID( node._instanceID ),
        _status( NET_NODE_STAT_NORMAL )
      {
         SDB_ASSERT( (UINT64)&_status % 4 == 0,
                     "the addr of _status must be aligned 4 bytes!" );
         _id = node._id ;
         _isActive = node._isActive ;
         ossStrcpy( _host, node._host ) ;
         for ( UINT32 i = 0; i < MSG_ROUTE_SERVICE_TYPE_MAX; i++ )
         {
            _service[i] = node._service[i];
         }
      }

      const _netRouteNode &operator=(const _netRouteNode &node )
      {
         _id = node._id ;
         _isActive = node._isActive ;
         _status = node._status ;
         ossStrcpy( _host, node._host ) ;
         for ( UINT32 i = 0; i < MSG_ROUTE_SERVICE_TYPE_MAX; i++ )
         {
            _service[i] = node._service[i];
         }
         _instanceID = node._instanceID ;
         return *this ;
      }

      void updateStatus( NET_NODE_STATUS status, UINT64 curTime )
      {
         _status = status ;
         if ( NET_NODE_STAT_NORMAL != _status )
         {
            _faultTime = curTime ;
         }
      }

      void getStatus( INT32 &status, UINT64 &faultTime ) const
      {
         status = _status ;
         faultTime = _faultTime ;
      }

      /*
         This function will change status when timeout > faultTimeout
      */
      NET_NODE_STATUS getStatus( UINT64 curTime,
                                 INT32 faultTimeout = NET_NODE_FAULT_TIMEOUT )
      {
         if ( NET_NODE_STAT_NORMAL != _status && faultTimeout > 0 )
         {
            if ( curTime - _faultTime > (UINT64)faultTimeout  )
            {
               _status = NET_NODE_STAT_NORMAL ;
            }
         }
         return (NET_NODE_STATUS)_status ;
      }

      /*
         This function not change status when timeout > faultTimeout
      */
      BOOLEAN isInStatus( UINT64 curTime,
                          INT32 status,
                          INT32 faultTimeout = NET_NODE_FAULT_TIMEOUT )
      {
         INT32 tmpStatus = _status ;
         if ( NET_NODE_STAT_NORMAL != tmpStatus && faultTimeout > 0 )
         {
            if ( curTime - _faultTime > ( UINT64 )faultTimeout )
            {
               tmpStatus = NET_NODE_STAT_NORMAL ;
            }
         }
         return status == tmpStatus ? TRUE : FALSE ;
      }

   } ;

   /*
      _netIOV define
   */
   class _netIOV : public SDBObject
   {
   public:
      _netIOV()
      :iovBase( NULL ),
       iovLen( 0 )
      {

      }

      _netIOV( const void *base, UINT32 len )
      :iovBase(base),
       iovLen(len)
      {

      }

      virtual ~_netIOV()
      {
         iovBase = NULL ;
         iovLen = 0 ;
      }
   public:
      const void *iovBase ;
      UINT32 iovLen ;
   } ;
   typedef class _netIOV netIOV ;

   typedef std::vector<netIOV> netIOVec ;

   UINT32 netCalcIOVecSize( const netIOVec &ioVec ) ;

}

#endif // NETDEF_HPP_

