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

   Source File Name = coordOmStrategyAccessor.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/13/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_OM_STRATEGY_ACCESSOR_HPP__
#define COORD_OM_STRATEGY_ACCESSOR_HPP__

#include "omStrategyDef.hpp"
#include "pmdEDU.hpp"
#include "rtnContextBuff.hpp"
#include <vector>

namespace engine
{

   class _IOmProxy ;

   /*
      _coordOmStrategyAccessor define
   */
   class _coordOmStrategyAccessor : public SDBObject
   {
      public:
         _coordOmStrategyAccessor( INT64 timeout = -1 ) ;
         ~_coordOmStrategyAccessor() ;

         void  setTimeout( INT64 timeout ) ;

         INT32 getMetaInfoFromOm( omStrategyMetaInfo &metaInfo,
                                  pmdEDUCB *cb,
                                  rtnContextBuf *buf ) ;

         INT32 getStrategyInfoFromOm( vector<omTaskStrategyInfoPtr> &vecInfo,
                                      pmdEDUCB *cb,
                                      rtnContextBuf *buf ) ;

         INT32 getTaskInfoFromOm( vector<omTaskInfoPtr> &vecInfo,
                                  pmdEDUCB *cb,
                                  rtnContextBuf *buf ) ;

      private:
         _IOmProxy               *_pOmProxy ;
         INT64                   _oprTimeout ;

         string                  _clsName ;
         string                  _bizName ;

   } ;
   typedef _coordOmStrategyAccessor coordOmStrategyAccessor ;

}

#endif // COORD_OM_STRATEGY_ACCESSOR_HPP__
