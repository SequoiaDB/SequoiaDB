/*******************************************************************************


   Copyright (C) 2011-2022 SequoiaDB Ltd.

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

   Source File Name = rtnSnapshotProcessor.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/19/2022  YQC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "utilMath.hpp"
#include "rtnSnapshotProcessor.hpp"

using namespace bson ;

namespace engine
{

   static BSONObj _transObjWithAgv( const BSONObj &obj )
   {
      INT64 lobCapacity       = 0 ;
      INT64 totalLobs         = 0 ;
      INT64 totalUsedLobSpace = 0 ;
      INT64 totalValidLobSize = 0 ;
      INT64 avgLobSize        = 0 ;
      BSONObjIterator itr1( obj ) ;
      BSONObjIterator itr2( obj ) ;
      BSONObjBuilder builder( obj.objsize() ) ;

      /// get lob info
      while ( itr1.more() )
      {
         BSONElement e = itr1.next() ;
         const CHAR *field = e.fieldName() ;

         if ( 0 == ossStrcmp( field, FIELD_NAME_LOB_CAPACITY ) )
         {
            lobCapacity = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_TOTAL_LOBS ) )
         {
            totalLobs = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_TOTAL_USED_LOB_SPACE ) )
         {
            totalUsedLobSpace = e.numberLong() ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_TOTAL_VALID_LOB_SIZE ) )
         {
            totalValidLobSize = e.numberLong() ;
         }
      }

      if ( 0 < totalLobs )
      {
         avgLobSize = totalValidLobSize / totalLobs ;
      }

      while ( itr2.more() )
      {
         BSONElement e = itr2.next() ;
         const CHAR *field = e.fieldName() ;
         if ( 0 == ossStrcmp( field, FIELD_NAME_USED_LOB_SPACE_RATIO ) )
         {
            builder.append( FIELD_NAME_USED_LOB_SPACE_RATIO,
                           utilPercentage( totalUsedLobSpace, lobCapacity ) ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_LOB_USAGE_RATE ) )
         {
            builder.append( FIELD_NAME_LOB_USAGE_RATE,
                           utilPercentage( totalValidLobSize, totalUsedLobSpace ) ) ;
         }
         else if ( 0 == ossStrcmp( field, FIELD_NAME_AVG_LOB_SIZE ) )
         {
            builder.append( FIELD_NAME_AVG_LOB_SIZE, avgLobSize ) ;
         }
         else
         {
            builder.append( e ) ;
         }
      }

      return builder.obj() ;
   }

   _rtnCSSnapshotProcessor::_rtnCSSnapshotProcessor()
   {
      _eof = FALSE ;
      _hasDone = FALSE ;
   }

   _rtnCSSnapshotProcessor::~_rtnCSSnapshotProcessor()
   {
      _eof = FALSE ;
      _hasDone = FALSE ;
   }

   INT32 _rtnCSSnapshotProcessor::pushIn( const BSONObj &obj )
   {
      _obj = obj ;
      return SDB_OK ;
   }

   INT32  _rtnCSSnapshotProcessor::output( BSONObj &obj, BOOLEAN & hasOut )
   {
      INT32 rc = SDB_OK ;
      BSONObj emptyObj ;
      try
      {
         if ( !_obj.isEmpty() )
         {
            obj = _transObjWithAgv( _obj ) ;
            _obj = emptyObj ;
            hasOut = TRUE ;
         }
         else
         {
            hasOut = FALSE ;
            if ( _hasDone )
            {
               _eof = TRUE ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Exception captured:%s when output cs snapshot result",
                 e.what() ) ;
      }
      return rc ;
   }

   INT32 _rtnCSSnapshotProcessor::done( BOOLEAN &hasOut )
   {
      _hasDone = TRUE ;
      if ( !_obj.isEmpty() )
      {
         hasOut = TRUE ;
      }

      return SDB_OK ;
   }

   BOOLEAN _rtnCSSnapshotProcessor::eof() const
   {
      return _eof ;
   }

}  // namespace engine