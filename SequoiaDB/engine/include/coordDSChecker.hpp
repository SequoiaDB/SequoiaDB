/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordDSChecker.hpp

   Descriptive Name = Data source checker header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_DSCHECKER_HPP__
#define COORD_DSCHECKER_HPP__

#include "oss.hpp"
#include "pmdEDU.hpp"
#include "IDataSource.hpp"
#include "coordRemoteConnection.hpp"

namespace engine
{
   class _coordDSInfoChecker : public SDBObject
   {
   public:
      _coordDSInfoChecker() {}
      virtual ~_coordDSInfoChecker() {}

      INT32 check( const BSONObj& infoObj, pmdEDUCB *cb, BSONObj *dsMeta ) ;

      static INT32 checkDSName( const CHAR *name ) ;

   private:
      INT32 _getDSMeta( coordRemoteConnection &connection,
                        BSONObj &dsMeta, pmdEDUCB *cb ) ;

      static BOOLEAN _isSysName( const CHAR *name ) ;
   } ;
   typedef _coordDSInfoChecker coordDSInfoChecker ;

   class _coordDSAddrChecker : public SDBObject
   {
   public:
      _coordDSAddrChecker() ;
      ~_coordDSAddrChecker() ;

      INT32 check( const CHAR *addrList, const CHAR *user,
                   const CHAR *password, pmdEDUCB *cb ) ;

   private:
      INT32 _checkAddrConflict( coordRemoteConnection &connection,
                                utilAddrContainer &addrContainer,
                                _utilArray<UINT16> &pendingAddr,
                                pmdEDUCB *cb );
      INT32 _getMyAddresses( _utilArray<utilAddrPair> &addresses ) ;
      INT32 _getDSGroupAddresses( coordRemoteConnection &connection,
                                  const CHAR *groupName,
                                  utilAddrContainer &addrContainer,
                                  pmdEDUCB *cb ) ;
      INT32 _parseDSSvcAddresses( const BSONObj &groupInfo,
                                  utilAddrContainer &addresses ) ;

      INT32 _ensureSameClusterAddresses( coordRemoteConnection &connection,
                                         utilAddrContainer &addrContainer,
                                         _utilArray<UINT16> &unkownAddr,
                                         pmdEDUCB *cb ) ;

   private:
      const CHAR *_user ;
      const CHAR *_password ;
   } ;
   typedef _coordDSAddrChecker coordDSAddrChecker ;

   class _coordDSCSChecker : public SDBObject
   {
   public:
      _coordDSCSChecker() ;
      ~_coordDSCSChecker() ;

      INT32 check( CoordDataSourcePtr dsPtr, const CHAR *name, pmdEDUCB *cb,
                   BOOLEAN &exist ) ;

   } ;
   typedef _coordDSCSChecker coordDSCSChecker ;

   class _coordDSCLChecker : public SDBObject
   {
   public:
      _coordDSCLChecker() ;
      ~_coordDSCLChecker() ;

      INT32 check( CoordDataSourcePtr dsPtr, const CHAR *name, pmdEDUCB *cb,
                   BOOLEAN &exist ) ;

   } ;
   typedef _coordDSCLChecker coordDSCLChecker ;
}

#endif /* COORD_DSCHECKER_HPP__ */
