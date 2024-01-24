/******************************************************************************

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

   Source File Name = mthElemMatchIterator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "mthElemMatchIterator.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthMatchTree.hpp"

using namespace bson ;

namespace engine
{
   _mthElemMatchIterator::_mthElemMatchIterator( const bson::BSONObj &obj,
                                                 _mthMatchTree *matcher,
                                                 bson::BSONObjBuilder *matchTargetBob,
                                                 INT32 n,
                                                 BOOLEAN isArray,
                                                 BOOLEAN subFieldIsOp )
   :_matcher( matcher ),
    _obj( obj ),
    _i( _obj ),
    _n( n ),
    _matched( 0 ),
    _isArray( isArray ),
    _matchTargetBob( matchTargetBob ),
    _subFieldIsOp( subFieldIsOp )
   {
      SDB_ASSERT( NULL != _matcher, "can not be null" ) ;
   }

   _mthElemMatchIterator::~_mthElemMatchIterator()
   {

   }

   INT32 _mthElemMatchIterator::_buildMatchTarget( const bson::BSONElement &ele,
                                                   bson::BSONObj &matchTarget,
                                                   const CHAR* newFieldName )
   {
      INT32 rc  = SDB_OK ;
      _matchTargetBob->reset() ;

      try
      {
         if ( NULL == newFieldName )
         {
            _matchTargetBob->append( ele ) ;
         }
         else
         {
            _matchTargetBob->appendAs( ele, newFieldName ) ;
         }

         matchTarget = _matchTargetBob->done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG ( PDERROR, "Build $elemMatch or $elemMatchOne match target "
                  "exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthElemMatchIterator::next( bson::BSONElement &e )
   {
      INT32   rc = SDB_OK ;
      e = BSONElement() ;

      while ( _i.more() && ( _n < 0 || _matched < _n ) )
      {
         BSONElement ele = _i.next() ;
         BSONObj matchTarget ;
         if ( _isArray )
         {
            if ( _subFieldIsOp )
            {
               rc = _buildMatchTarget( ele, matchTarget, "" ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to build $elemMatch or $elemMatchOne"
                          " match target, rc: %d", rc ) ;
                  goto error ;
               }
            }
            else
            {
               if ( Object == ele.type() )
               {
                  matchTarget = ele.embeddedObject() ;
               }
               else
               {
                  continue ;
               }
            }
         }
         else
         {
            if ( _subFieldIsOp )
            {
               rc = _buildMatchTarget( ele, matchTarget, "" ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to build $elemMatch or $elemMatchOne"
                          "match target, rc: %d", rc ) ;
                  goto error ;
               }
            }
            else
            {
               rc = _buildMatchTarget( ele, matchTarget ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to build $elemMatch or $elemMatchOne"
                          "match target, rc: %d", rc ) ;
                  goto error ;
               }
            }
         }

         BOOLEAN res = FALSE ;
         rc = _matcher->matches( matchTarget, res ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to match obj:%d", rc ) ;
            goto error ;
         }
         else if ( res )
         {
            e = ele ;
            ++_matched ;
            break ;
         }
      }

      if ( e.eoo() )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
   done:
      return rc ;
   error:
      e = BSONElement() ;
      goto done ;
   }
}

