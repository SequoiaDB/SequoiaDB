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

   Source File Name = qgmPlSplitBy.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "qgmPlSplitBy.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"
#include <sstream>

using namespace std ;
using namespace bson ;

namespace engine
{
   _qgmPlSplitBy::_qgmPlSplitBy( const _qgmDbAttr &splitby,
                                 const _qgmField &alias )
   :_qgmPlan( QGM_PLAN_TYPE_SPLIT, alias ),
   _splitby(splitby),
   _itr(_fetch.obj),
   _fieldName( _splitby.attr().toFieldName() )
   {
      _initialized = TRUE ;
      _replaced = FALSE ;
   }

   _qgmPlSplitBy::~_qgmPlSplitBy()
   {

   }

   INT32 _qgmPlSplitBy::_execute( _pmdEDUCB *eduCB )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( 1 == inputSize(), "impossible" ) ;
      rc = input( 0 )->execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLSPLITBY__FETCHNEXT, "_qgmPlSplitBy::_fetchNext" )
   INT32 _qgmPlSplitBy::_fetchNext( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLSPLITBY__FETCHNEXT ) ;
      INT32 rc = SDB_OK ;
      _replaced = FALSE ;

      if ( _fetch.obj.isEmpty() )
      {
   fetch:
         rc = input( 0 )->fetchNext( _fetch ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         SDB_ASSERT( NULL == _fetch.next, "impossible" ) ;

         _splitEle = _fetch.obj.getFieldDotted( _fieldName ) ;
         if ( Array != _splitEle.type() ) /// not array
         {
            next = _fetch ;
            _clear() ;
            goto done ;
         }
         else if ( 0 == _splitEle.embeddedObject().nFields() )
         {
            BSONObjBuilder build ;
            BSONObjIterator itr( _fetch.obj ) ;
            rc = _buildNewObj( build, itr, BSONElement() ) ;
            PD_RC_CHECK( rc, PDERROR, "Build new object failed, rc: %d",
                         rc ) ;
            next.obj = build.obj() ;
            next.alias = _fetch.alias ;
            _clear() ;
            goto done ;
         }
         else
         {
            _itr = BSONObjIterator( _splitEle.embeddedObject() ) ;
         }
      }

      SDB_ASSERT( NULL == _fetch.next, "impossible" ) ;
      if ( _itr.more() )
      {
         BSONObjBuilder build ;
         BSONObjIterator itr( _fetch.obj ) ;
         rc = _buildNewObj( build, itr, _itr.next() ) ;
         PD_RC_CHECK( rc, PDERROR, "Build new object failed, rc: %d",
                      rc ) ;
         next.obj = build.obj() ;
         next.alias = _fetch.alias ;
      }
      else
      {
         _clear() ;
         goto fetch ;
      }

   done:
      PD_TRACE_EXITRC( SDB__QGMPLSPLITBY__FETCHNEXT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _qgmPlSplitBy::_appendReplace( BSONObjBuilder &b,
                                       const BSONElement &replace )
   {
      if ( !replace.eoo() )
      {
         b.appendAs( replace, _splitEle.fieldName() ) ;
      }
      else
      {
         b.appendNull( _splitEle.fieldName() ) ;
      }
   }

   void _qgmPlSplitBy::_appendReplace( BSONArrayBuilder &b,
                                       const BSONElement &replace )
   {
      if ( !replace.eoo() )
      {
         b.append( replace ) ;
      }
      else
      {
         b.appendNull() ;
      }
   }

   template<class Builder>
   INT32 _qgmPlSplitBy::_buildNewObj( Builder &b, BSONObjIterator &es,
                                      const BSONElement &replace )
   {
      INT32 rc = SDB_OK ;

      while( es.more() )
      {
         BSONElement e = es.next() ;

         /*
            Split element EOO or
            Split element END <= e BEING or
            Split element BEGIN >= e END
         */
         if ( _replaced ||
              _splitEle.eoo() ||
              _splitEle.fieldName() + _splitEle.size() - 1 <=
              e.fieldName() - 1 ||
              _splitEle.fieldName() - 1 >=
              e.fieldName() + e.size() - 1 )
         {
            b.append( e ) ;
         }
         /*
            Split element == e
         */
         else if ( _splitEle.fieldName() == e.fieldName() &&
                   _splitEle.size() == e.size() )
         {
            _appendReplace( b, replace ) ;
            _replaced = TRUE ;
         }
         /*
            Split element in e
         */
         else
         {
            if ( Object == e.type() )
            {
               BSONObjBuilder bb( b.subobjStart(e.fieldName())) ;
               BSONObjIterator bis( e.Obj() ) ;
               rc = _buildNewObj( bb, bis, replace ) ;
               PD_RC_CHECK( rc, PDERROR, "Build object[%s] failed, rc: %d",
                            e.toString().c_str(), rc ) ;
               bb.done() ;
            }
            else if ( Array == e.type() )
            {
               BSONArrayBuilder ba( b.subarrayStart( e.fieldName() ) ) ;
               BSONObjIterator bis(e.embeddedObject()) ;
               rc = _buildNewObj( ba, bis, replace ) ;
               PD_RC_CHECK( rc, PDERROR, "Build array[%s] failed, rc: %d",
                            e.toString().c_str(), rc ) ;
               ba.done() ;
            }
            else
            {
               b.append( e ) ;
            }
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   void _qgmPlSplitBy::_clear()
   {
      _fetch.obj = BSONObj() ;
      _splitEle = BSONElement() ;
      _fetch.alias.clear() ;
   }

   string _qgmPlSplitBy::toString() const
   {
      stringstream ss ;
      ss << "split by [" << _splitby.toString() << "]" << "\n" ;
      return ss.str();
   }
}
