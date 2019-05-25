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

   Source File Name = pmdPrefetcher.cpp

   Descriptive Name = Process MoDel Prefetcher

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main entry point for prefetcher

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/01/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef BPSPREFETCH_HPP__
#define BPSPREFETCH_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"

namespace engine
{
   /*
      BPS_DMS_TYPE define
   */
   enum BPS_DMS_TYPE
   {
      BPS_DMS_DATA      = 0,
      BPS_DMS_INDEX,
   } ;

   /*
      _bpsPreLoadReq define
   */
   class _bpsPreLoadReq : public SDBObject
   {
      public:
         BPS_DMS_TYPE         _type ;
         dmsStorageUnitID     _csid ;
         UINT32               _csLID ;
         dmsExtentID          _extid ;

         _bpsPreLoadReq( dmsStorageUnitID csid, UINT32 csLID, dmsExtentID extID,
                         BPS_DMS_TYPE type = BPS_DMS_DATA )
         {
            _type = type ;
            _csid = csid ;
            _csLID = csLID ;
            _extid = extID ;
         }
         _bpsPreLoadReq( const _bpsPreLoadReq &right )
         {
            _type = right._type ;
            _csid = right._csid ;
            _csLID = right._csLID ;
            _extid = right._extid ;
         }
         _bpsPreLoadReq &operator=( const _bpsPreLoadReq &right )
         {
            _type = right._type ;
            _csid = right._csid ;
            _csLID = right._csLID ;
            _extid = right._extid ;

            return *this ;
         }
   } ;
   typedef class _bpsPreLoadReq bpsPreLoadReq ;

   class _rtnContextBase ;
   /*
      _bpsDataPref define
   */
   class _bpsDataPref : public SDBObject
   {
      public:
         UINT32                  _prefID ;
         _rtnContextBase         *_context ;

         _bpsDataPref ()
         {
            _prefID  = 0 ;
            _context = NULL ;
         }
         _bpsDataPref( UINT32 prefID, _rtnContextBase *context )
         {
            _prefID     = prefID ;
            _context    = context ;
         }
         _bpsDataPref( const _bpsDataPref &right )
         {
            _prefID     = right._prefID ;
            _context    = right._context ;
         }
         _bpsDataPref& operator=( const _bpsDataPref &right )
         {
            _prefID     = right._prefID ;
            _context    = right._context ;
            return *this ;
         }
   } ;
   typedef class _bpsDataPref bpsDataPref ;

}

#endif //BPSPREFETCH_HPP__

