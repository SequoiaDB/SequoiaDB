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

   Source File Name = authActionSet.cpp

   Descriptive Name = 

   When/how to use: this program may be used on backup or restore db data.
   You can specfiy some options from parameters.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/14/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#include "authActionSet.hpp"
#include <boost/make_shared.hpp>
#include <sstream>

namespace engine
{
   boost::shared_ptr< _authActionSet > _authActionSet::fromBson( const bson::BSONObj &obj )
   {
      boost::shared_ptr< _authActionSet > result = boost::make_shared< _authActionSet >();

      bson::BSONObjIterator it( obj );
      while ( it.more() )
      {
         bson::BSONElement e = it.next();
         if ( e.type() == bson::String )
         {
            result->addAction( authActionTypeParse( e.valuestr() ) );
         }
         else
         {
            result.reset();
            break;
         }
      }

      return result;
   }

   _authActionSet::_authActionSet()
   {
      _data.reset();
   }

   _authActionSet::_authActionSet( const std::vector< ACTION_TYPE > &actions )
   {
      for ( std::vector< ACTION_TYPE >::const_iterator it = actions.begin(); it != actions.end();
            ++it )
      {
         _data.set( UINT32( *it ) );
      }
   }

   authActionSet::_authActionSet( const ACTION_SET_NUMBER_ARRAY& numbers )
   {
      for ( INT32 action = 0; action < ACTION_SET_SIZE; ++action )
      {
         INT32 n = action / sizeof( UINT64 ) / 8;
         _data.set( action, numbers.numbers[ n ] >> action & 1 );
      }
   }

   _authActionSet::~_authActionSet() {}

   std::ostream &operator<<( std::ostream &os, const _authActionSet &set )
   {
      BOOLEAN first = TRUE;
      os << "[";
      for ( INT32 action = 0; action < ACTION_SET_SIZE; ++action )
      {
         if ( set._data[ action ] )
         {
            if ( !first )
            {
               os << ", ";
            }
            else
            {
               first = FALSE;
            }
            os << authActionTypeSerializer( static_cast< ACTION_TYPE >( action ) );
         }
      }
      os << "]";
      return os;
   }

   boost::shared_ptr< _authActionSet > _authActionSet::operator&(
      const _authActionSet &other ) const
   {
      boost::shared_ptr< _authActionSet > result = boost::make_shared< _authActionSet >();

      result->_data = this->_data & other._data;

      return result;
   }

   void _authActionSet::addAction( ACTION_TYPE action )
   {
      if ( action != ACTION_TYPE__invalid )
      {
         _data.set( action );
      }
   }

   void _authActionSet::addAllActionsFromSet( const _authActionSet &actionSet )
   {
      _data |= actionSet._data;
   }

   void _authActionSet::removeAction( ACTION_TYPE action )
   {
      _data.reset( action );
   }

   void _authActionSet::removeActionsFromSet( const _authActionSet &actionSet )
   {
      _data &= ~actionSet._data;
   }

   BOOLEAN _authActionSet::contains( ACTION_TYPE action ) const
   {
      return _data[ action ];
   }

   BOOLEAN _authActionSet::contains( const _authActionSet &other ) const
   {
      return ( _data | other._data ) == _data;
   }

   BOOLEAN _authActionSet::isSupersetOf( const _authActionSet &other ) const
   {
      return ( _data & other._data ) == other._data;
   }

   void _authActionSet::toBSONArray( bson::BSONArrayBuilder &builder ) const
   {
      for ( INT32 action = 0; action < ACTION_SET_SIZE; ++action )
      {
         if ( _data[ action ] )
         {
            builder.append( authActionTypeSerializer( static_cast< ACTION_TYPE >( action ) ) );
         }
      }
      builder.doneFast();
   }
} // namespace engine