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

   Source File Name = seAdptOprMon.hpp

   Descriptive Name = Search engine adapter operation monitor.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/21/2019  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_OPRMON_HPP__
#define SEADPT_OPRMON_HPP__

#include "oss.hpp"
#include "rtnExtOprDef.hpp"

using namespace engine ;

namespace seadapter
{
   class _seAdptOprMon : public SDBObject
   {
   public:
      _seAdptOprMon()
      {
         reset( TRUE ) ;
      }

      ~_seAdptOprMon() {}

      void reset( BOOLEAN resetTotal = FALSE )
      {
         _insertCount = 0 ;
         _updateCount = 0 ;
         _deleteCount = 0 ;
         _ignoreCount = 0 ;
         if ( resetTotal )
         {
            _totalInsertCount = 0 ;
            _totalUpdateCount = 0 ;
            _totalDeleteCount = 0 ;
            _totalIgnoreCount = 0 ;
         }
      }

      OSS_INLINE void monOprCountInc( _rtnExtOprType type, UINT64 delta = 1 )
      {
         switch ( type )
         {
         case RTN_EXT_INSERT:
            monInsertCountInc( delta ) ;
            break ;
         case RTN_EXT_UPDATE:
         case RTN_EXT_UPDATE_WITH_ID:
            monUpdateCountInc( delta ) ;
            break ;
         case RTN_EXT_DELETE:
            monDeleteCountInc( delta ) ;
            break ;
         default:
            break ;
         }
      }

      OSS_INLINE void monInsertCountInc( UINT64 delta = 1 )
      {
         _insertCount += delta ;
         _totalInsertCount += delta ;
      }

      OSS_INLINE void monUpdateCountInc( UINT64 delta = 1 )
      {
         _updateCount += delta ;
         _totalUpdateCount += delta ;
      }

      OSS_INLINE void monDeleteCountInc( UINT64 delta = 1 )
      {
         _deleteCount += delta ;
         _totalDeleteCount += delta ;
      }

      OSS_INLINE void monIgnoreCountInc( UINT64 delta = 1 )
      {
         _ignoreCount += delta ;
         _totalIgnoreCount += delta ;
      }

   public:
      UINT32 _insertCount ;
      UINT32 _updateCount ;
      UINT32 _deleteCount ;
      UINT32 _ignoreCount ;
      UINT64 _totalInsertCount ;
      UINT64 _totalUpdateCount ;
      UINT64 _totalDeleteCount ;
      UINT64 _totalIgnoreCount ;
   } ;
   typedef _seAdptOprMon seAdptOprMon ;
}

#endif /* SEADPT_OPRMON_HPP__ */

