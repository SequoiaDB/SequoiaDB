/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = rtnSessionProperty.hpp

   Descriptive Name = session properties

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/01/2018  HGM Initial Draft, split from coordRemoteSession.hpp

   Last Changed =

*******************************************************************************/

#ifndef RTN_SESSION_PROPERTY_HPP__
#define RTN_SESSION_PROPERTY_HPP__

#include "msg.hpp"
#include "utilArray.hpp"
#include "ossMemPool.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   #define RTN_SESSION_OPERATION_TIMEOUT_MIN ( 1000 )    // 1000ms
   #define RTN_SESSION_OPERATION_TIMEOUT_MAX ( -1 )

   typedef enum _RTN_PREFER_INSTANCE_TYPE
   {
      // Note : secondary choice means lower priority than numeric instances
      PREFER_INSTANCE_TYPE_MASTER_SND  = -6, // master node in secondary choice
      PREFER_INSTANCE_TYPE_SLAVE_SND   = -5, // any slave node in secondary choice
      PREFER_INSTANCE_TYPE_ANYONE_SND  = -4, // any node( include master ) in secondary choice
      PREFER_INSTANCE_TYPE_MASTER      = -3, // master node
      PREFER_INSTANCE_TYPE_SLAVE       = -2, // any slave node
      PREFER_INSTANCE_TYPE_ANYONE      = -1, // any node( include master )

      PREFER_INSTANCE_TYPE_MIN       = 0,
      PREFER_INSTANCE_TYPE_UNKNOWN   = PREFER_INSTANCE_TYPE_MIN,
      PREFER_INSTANCE_TYPE_MAX       = 256
   } RTN_PREFER_INSTANCE_TYPE ;

   typedef enum _RTN_PREFER_INSTANCE_MODE
   {
      PREFER_INSTANCE_MODE_UNKNOWN = 0,
      PREFER_INSTANCE_MODE_RANDOM = 1,
      PREFER_INSTANCE_MODE_ORDERED,
   } RTN_PREFER_INSTANCE_MODE ;

   typedef ossPoolList< UINT8 > RTN_INSTANCE_LIST ;

   /*
      _rtnInstanceOption define
    */
   class _rtnInstanceOption ;
   typedef class _rtnInstanceOption rtnInstanceOption ;
   class _dpsTransConfItem ;

   class _rtnInstanceOption : public SDBObject
   {
      public :
         _rtnInstanceOption () ;

         _rtnInstanceOption ( const rtnInstanceOption & option ) ;

         ~_rtnInstanceOption () ;

         rtnInstanceOption & operator = ( const rtnInstanceOption & option ) ;

         OSS_INLINE BOOLEAN isValidated () const
         {
            return ( PREFER_INSTANCE_TYPE_UNKNOWN == _specInstance &&
                     _instanceList.empty() ) ? FALSE : TRUE ;
         }

         OSS_INLINE BOOLEAN isMasterRequired() const
         {
            // must use master node
            return ( _instanceList.empty() &&
                     ( PREFER_INSTANCE_TYPE_MASTER == _specInstance ||
                       PREFER_INSTANCE_TYPE_MASTER_SND == _specInstance ) ) ;
         }

         OSS_INLINE BOOLEAN isMasterPreferred () const
         {
            // master node is preferred, but not required
            return ( PREFER_INSTANCE_TYPE_MASTER == _specInstance ||
                     PREFER_INSTANCE_TYPE_MASTER_SND == _specInstance ) ;
         }

         OSS_INLINE BOOLEAN isSlavePreferred () const
         {
            // slave node is preferred
            return ( PREFER_INSTANCE_TYPE_SLAVE == _specInstance ||
                     PREFER_INSTANCE_TYPE_SLAVE_SND == _specInstance ) ;
         }

         OSS_INLINE BOOLEAN hasCommonInstance () const
         {
            return _instanceList.empty() ? FALSE : TRUE ;
         }

         OSS_INLINE RTN_PREFER_INSTANCE_TYPE getSpecialInstance () const
         {
            return (RTN_PREFER_INSTANCE_TYPE)_specInstance ;
         }

         OSS_INLINE const RTN_INSTANCE_LIST & getInstanceList () const
         {
            return _instanceList ;
         }

         OSS_INLINE RTN_PREFER_INSTANCE_MODE getPreferredMode () const
         {
            return (RTN_PREFER_INSTANCE_MODE)_mode ;
         }

         OSS_INLINE BOOLEAN isPreferredStrict() const
         {
            return _strict ? TRUE : FALSE ;
         }

         OSS_INLINE void setPreferedPeriod( INT64 period )
         {
            // check range
            if ( period < 0 )
            {
               _period = -1 ;
            }
            else if ( period > OSS_SINT32_MAX_LL )
            {
               _period = OSS_SINT32_MAX ;
            }
            else
            {
               _period = (UINT32)period ;
            }
         }

         OSS_INLINE INT32 getPreferedPeriod() const
         {
            return _period ;
         }

         void  reset () ;
         INT32 setPreferredInstance ( PREFER_REPLICA_TYPE replType ) ;
         INT32 setPreferredInstance ( RTN_PREFER_INSTANCE_TYPE instance ) ;
         INT32 setPreferredInstanceMode ( RTN_PREFER_INSTANCE_MODE mode ) ;

         INT32 parsePreferredInstance ( const bson::BSONElement & option ) ;
         INT32 parsePreferredInstance ( const CHAR * instanceStr ) ;
         INT32 parsePreferredInstanceMode ( const CHAR * instanceModeStr ) ;

         void  setPreferredStrict( BOOLEAN strict ) ;

         void toBSON ( bson::BSONObjBuilder & builder ) const ;

      protected :
         INT32 _parseIntegerPreferredInstance ( const bson::BSONElement & option ) ;
         INT32 _parseStringPreferredInstance ( const bson::BSONElement & option ) ;
         INT32 _parseArrayPreferredInstance ( const bson::BSONElement & option ) ;

         const CHAR * _getInstanceStr ( INT8 instance ) const ;

         void _clearInstance () ;

      protected :
         UINT8             _mode ;
         UINT8             _strict ;
         INT8              _specInstance ;
         INT32             _period ;
         RTN_INSTANCE_LIST _instanceList ;
   } ;

   /*
      _rtnSessionProperty define
    */
   class _rtnSessionProperty ;
   typedef class _rtnSessionProperty rtnSessionProperty ;

   class _rtnSessionProperty : public SDBObject
   {
      public :
         _rtnSessionProperty () ;

         _rtnSessionProperty ( const rtnSessionProperty & property ) ;

         virtual ~_rtnSessionProperty () ;

         void setInstanceOption ( const CHAR * instanceStr,
                                  const CHAR * instanceModeStr,
                                  BOOLEAN preferedStrict,
                                  INT32 preferedPeriod,
                                  RTN_PREFER_INSTANCE_TYPE defaultInstance ) ;

         OSS_INLINE void setInstanceOption ( const rtnInstanceOption & instanceOption )
         {
            _instanceOption = instanceOption ;
         }

         OSS_INLINE const rtnInstanceOption & getInstanceOption () const
         {
            return _instanceOption ;
         }

         OSS_INLINE rtnInstanceOption & getInstanceOption ()
         {
            return _instanceOption ;
         }

         OSS_INLINE BOOLEAN isMasterRequired () const
         {
            return _instanceOption.isMasterRequired() ;
         }

         OSS_INLINE void setMasterRequired ()
         {
            _instanceOption.setPreferredInstance( PREFER_INSTANCE_TYPE_MASTER ) ;
            _instanceOption.setPreferredInstanceMode( PREFER_INSTANCE_MODE_RANDOM ) ;
         }

         OSS_INLINE BOOLEAN isMasterPreferred() const
         {
            return _instanceOption.isMasterPreferred() ;
         }

         OSS_INLINE BOOLEAN isSlavePreferred() const
         {
            return _instanceOption.isSlavePreferred() ;
         }

         OSS_INLINE void setOperationTimeout ( INT64 operationTimeout )
         {
            if ( operationTimeout < 0 )
            {
               _operationTimeout = RTN_SESSION_OPERATION_TIMEOUT_MAX ;
            }
            else if ( operationTimeout < RTN_SESSION_OPERATION_TIMEOUT_MIN )
            {
               _operationTimeout = RTN_SESSION_OPERATION_TIMEOUT_MIN ;
            }
            else
            {
               _operationTimeout = operationTimeout ;
            }
         }

         OSS_INLINE INT64 getOperationTimeout () const
         {
            return _operationTimeout ;
         }

         INT32 parseProperty( const BSONObj &property ) ;

         BSONObj toBSON () const ;

      protected :
         INT32 _parsePropertyV0 ( const bson::BSONObj & property ) ;
         INT32 _parsePropertyV1 ( const bson::BSONObj & property ) ;

         virtual void   _onSetInstance () {}
         virtual void   _toBson( BSONObjBuilder &builder ) const {}

         virtual INT32  _checkTransConf( const _dpsTransConfItem *pTransConf ) ;
         virtual void   _updateTransConf( const _dpsTransConfItem *pTransConf ) ;
         virtual INT32  _checkSource( const CHAR *pSource ) ;
         virtual void   _updateSource( const CHAR *pSource ) ;

      protected :
         rtnInstanceOption    _instanceOption ;
         INT64                _operationTimeout ;
   } ;

}

#endif // RTN_SESSION_PROPERTY_HPP__
