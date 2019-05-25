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

   Source File Name = coordOmProxy.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/03/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_OM_PROXY_HPP__
#define COORD_OM_PROXY_HPP__

#include "sdbIOmProxy.hpp"
#include "oss.hpp"

using namespace bson ;

namespace engine
{

   class _coordResource ;

   /*
      _coordOmProxy define
   */
   class _coordOmProxy : public _IOmProxy, public SDBObject
   {
      public:
         _coordOmProxy() ;
         virtual ~_coordOmProxy() ;

         INT32          init( _coordResource *pResource ) ;

      public:
         virtual INT32  queryOnOm( MsgHeader *pMsg,
                                  INT32 requestType,
                                  pmdEDUCB *cb,
                                  INT64 &contextID,
                                  rtnContextBuf *buf ) ;

         virtual INT32 queryOnOm( const rtnQueryOptions &options,
                                  pmdEDUCB *cb,
                                  SINT64 &contextID,
                                  rtnContextBuf *buf ) ;

         virtual INT32 queryOnOmAndPushToVec( const rtnQueryOptions &options,
                                              pmdEDUCB *cb,
                                              vector<BSONObj> &objs,
                                              rtnContextBuf *buf ) ;

         virtual void  setOprTimeout( INT64 timeout ) ;

      protected:
         _coordResource                *_pResource ;
         INT64                         _oprTimeout ;

   } ;
   typedef _coordOmProxy coordOmProxy ;

}

#endif //COORD_OM_PROXY_HPP__

