/*******************************************************************************


   Copyright (C) 2011-2019 SequoiaDB Ltd.

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

   Source File Name = rtnResultSetFilter.hpp

   Descriptive Name = Resultset filter

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/20/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_RSFILTER_HPP__
#define RTN_RSFILTER_HPP__

#include "oss.hpp"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "pd.hpp"

namespace engine
{
   class _rtnResultSetFilter : public SDBObject
   {
      typedef ossPoolSet<dmsRecordID>  RID_SET ;
      typedef RID_SET::const_iterator  RID_SET_ITR ;

   public:
      _rtnResultSetFilter() {}
      virtual ~_rtnResultSetFilter() {}

      OSS_INLINE INT32 push( const dmsRecordID &rid, BOOLEAN &hasPushed ) ;
      OSS_INLINE BOOLEAN isFiltered( const dmsRecordID &item ) ;

   private:
      RID_SET _filterSet ;
   } ;
   typedef _rtnResultSetFilter rtnResultSetFilter ;

   OSS_INLINE INT32 _rtnResultSetFilter::push( const dmsRecordID &rid,
                                               BOOLEAN &hasPushed )
   {
      INT32 rc = SDB_OK ;
      std::pair<ossPoolSet<dmsRecordID>::iterator, bool> result ;
      try
      {
         result = _filterSet.insert( rid ) ;
      }
      catch( const std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      hasPushed = result.second ;

   done:
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE BOOLEAN _rtnResultSetFilter::isFiltered( const dmsRecordID &item )
   {
      return ( _filterSet.find( item ) != _filterSet.end() ) ;
   }
}

#endif /* RTN_RSFILTER_HPP__ */
