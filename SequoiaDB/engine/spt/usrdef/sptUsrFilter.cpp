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

   Source File Name = sptUsrFilter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/08/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrFilter.hpp"
#include "../bson/bsonobj.h"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include <boost/lexical_cast.hpp>

using namespace bson ;

namespace engine
{
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrFilter, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrFilter, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrFilter, match )
   JS_STATIC_FUNC_DEFINE( _sptUsrFilter, help )

   JS_BEGIN_MAPPING_WITHHIDE( _sptUsrFilter, "_Filter" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_match", match, 0 )
      JS_ADD_STATIC_FUNC( "help", help )
   JS_MAPPING_END()

   _sptUsrFilter::_sptUsrFilter()
   {
   }

   _sptUsrFilter::~_sptUsrFilter()
   {
   }

   INT32 _sptUsrFilter::construct( const _sptArguments & arg,
                                   _sptReturnVal & rval,
                                   BSONObj & detail )
   {
      INT32 rc = SDB_OK ;

      rc = arg.getBsonobj( 0, _filter ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "filterObj must be config" ) ;
      }
      else if ( rc )
      {
         detail = BSON( SPT_ERR << "filterObj must be BSONObj" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to get filterObj, rc: %d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrFilter::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptUsrFilter::match( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj argObj ;
      vector< BSONObj > inputVec ;
      vector< BSONObj > outputVec ;
      BSONObjBuilder builder ;
      rc = arg.getBsonobj( 0, argObj ) ;

      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "argObj must be config" ) ;
         goto error;
      }

      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "argObj must be BSONObj" ) ;
         goto error;
      }

      for ( BSONObj::iterator iter = argObj.begin();
            iter.more(); )
      {
         inputVec.push_back( iter.next().Obj() );
      }

      for( vector< BSONObj >::iterator iter = inputVec.begin();
           iter != inputVec.end();
           iter++ )
      {
         if( _match( *iter, _filter, SPT_FILTER_MATCH_AND ) )
         {
            outputVec.push_back( *iter ) ;
         }
      }

      rval.getReturnVal().setValue( outputVec ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _sptUsrFilter::_match( const BSONObj &obj, const BSONObj &filter, SPT_FILTER_MATCH pred )
   {
      BOOLEAN matched = ( SPT_FILTER_MATCH_AND == pred ) ? TRUE : FALSE ;
      BSONObjIterator itFilter( filter ) ;
      while( itFilter.more() )
      {
         BOOLEAN subMatch = FALSE ;
         BSONElement e = itFilter.next() ;
         if ( 0 == ossStrcmp( e.fieldName(), "$and" ) &&
              Array == e.type() )
         {
            subMatch = _match( obj, e.embeddedObject(), SPT_FILTER_MATCH_AND ) ;
         }
         else if ( 0 == ossStrcmp( e.fieldName(), "$or" ) &&
                   Array == e.type() )
         {
            subMatch = _match( obj, e.embeddedObject(), SPT_FILTER_MATCH_OR ) ;
         }
         else if ( 0 == ossStrcmp( e.fieldName(), "$not" ) &&
                   Array == e.type() )
         {
            subMatch = !_match( obj, e.embeddedObject(), SPT_FILTER_MATCH_AND ) ;
         }
         else if ( Object == e.type() )
         {
            subMatch = _match( obj, e.embeddedObject(), pred ) ;
         }
         else
         {
            BSONElement e1 = obj.getField( e.fieldName() ) ;
            subMatch = ( 0 == e1.woCompare( e, false ) ) ? TRUE : FALSE ;
         }

         if ( SPT_FILTER_MATCH_AND == pred && FALSE == subMatch )
         {
            matched = FALSE ;
            break ;
         }
         else if ( SPT_FILTER_MATCH_OR == pred && TRUE == subMatch )
         {
            matched = TRUE ;
            break ;
         }
      }
      return matched ;
   }

   INT32 _sptUsrFilter::help( const _sptArguments & arg,
                              _sptReturnVal & rval,
                              BSONObj & detail )
   {

      stringstream ss ;
      ss << "_Filter functions:" << endl
         << "var filter = new _Filter( filterObj )" << endl
         << "   match( bsonArray )" << endl
         << " Filter.help()" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }
}