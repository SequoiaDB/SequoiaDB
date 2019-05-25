/*******************************************************************************

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
#include "utilList.hpp"
#include "../bson/bson.h"

namespace engine
{

   typedef enum _RTN_PREFER_INSTANCE_TYPE
   {
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

   typedef _utilList< UINT8 > RTN_INSTANCE_LIST ;

   /*
      _rtnInstanceOption define
    */
   class _rtnInstanceOption ;
   typedef class _rtnInstanceOption rtnInstanceOption ;

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

         OSS_INLINE BOOLEAN isMasterPreferred () const
         {
            return ( _instanceList.empty() &&
                     ( PREFER_INSTANCE_TYPE_MASTER == _specInstance ||
                       PREFER_INSTANCE_TYPE_MASTER_SND == _specInstance ) ) ;
         }

         OSS_INLINE BOOLEAN isSlavePerferred () const
         {
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

         void reset () ;
         INT32 setPreferredInstance ( PREFER_REPLICA_TYPE replType ) ;
         INT32 setPreferredInstance ( RTN_PREFER_INSTANCE_TYPE instance ) ;
         INT32 setPreferredInstanceMode ( RTN_PREFER_INSTANCE_MODE mode ) ;

         INT32 parsePreferredInstance ( const bson::BSONElement & option ) ;
         INT32 parsePreferredInstance ( const CHAR * instanceStr ) ;
         INT32 parsePreferredInstanceMode ( const CHAR * instanceModeStr ) ;

         void toBSON ( bson::BSONObjBuilder & builder ) const ;

      protected :
         INT32 _parseIntegerPreferredInstance ( const bson::BSONElement & option ) ;
         INT32 _parseStringPreferredInstance ( const bson::BSONElement & option ) ;
         INT32 _parseArrayPreferredInstance ( const bson::BSONElement & option ) ;

         const CHAR * _getInstanceStr ( INT8 instance ) const ;

         void _clearInstance () ;

      protected :
         UINT8             _mode ;
         INT8              _specInstance ;
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

         OSS_INLINE BOOLEAN isMasterPreferred () const
         {
            return _instanceOption.isMasterPreferred() ;
         }

         OSS_INLINE void setMasterPreferred ()
         {
            _instanceOption.setPreferredInstance( PREFER_INSTANCE_TYPE_MASTER ) ;
            _instanceOption.setPreferredInstanceMode( PREFER_INSTANCE_MODE_RANDOM ) ;
         }

         OSS_INLINE void setOperationTimeout ( INT64 operationTimeout )
         {
            _operationTimeout = operationTimeout < 0 ? -1 : operationTimeout ;
         }

         OSS_INLINE INT64 getOperationTimeout () const
         {
            return _operationTimeout ;
         }

         INT32 parseProperty ( const bson::BSONObj & property ) ;

         bson::BSONObj toBSON () const ;

      protected :
         INT32 _parsePropertyV0 ( const bson::BSONObj & property ) ;
         INT32 _parsePropertyV1 ( const bson::BSONObj & property ) ;

         virtual void _onSetInstance () {}

      protected :
         rtnInstanceOption _instanceOption ;
         INT64             _operationTimeout ;
   } ;

}

#endif // RTN_SESSION_PROPERTY_HPP__
