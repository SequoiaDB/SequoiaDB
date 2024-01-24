/*******************************************************************************

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

   Source File Name = coordSdbDataSource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/02/2021  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef COORD_SDBCOORDDATASOURCE_HPP__
#define COORD_SDBCOORDDATASOURCE_HPP__

#include "IDataSource.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordSdbDataSource define
   */
   class _coordSdbDataSource : public _IDataSource
   {
   public:
      _coordSdbDataSource() ;
      virtual ~_coordSdbDataSource() ;

      virtual const CHAR* getTypeName() const { return SDB_DATASOURCE_TYPE_NAME ; }
      virtual SDB_DS_TYPE getType() const { return SDB_DS_SDB ; }

   public:
      virtual INT32           init( const BSONObj &obj ) ;

      virtual UTIL_DS_UID     getID() const { return _id ; }
      virtual const CHAR*     getName() const { return _name ; }
      virtual const CHAR*     getAddress() const ;
      virtual const CHAR*     getUser() const { return _user ; }
      virtual const CHAR*     getPassword() const { return _password ; }
      virtual const CHAR*     getErrCtlLevel() const { return _errCtlLevel ; }
      virtual const CHAR*     getDSVersion() const ;
      virtual INT32           getErrFilterMask() const ;
      virtual BOOLEAN         isReadable() const ;
      virtual BOOLEAN         isWritable() const ;
      virtual INT32           getDSMajorVersion() const ;
      virtual INT32           getDSMinorVersion() const ;
      virtual INT32           getDSFixVersion() const ;
      virtual SDB_DS_TRANS_PROPAGATE_MODE getTransPropagateMode() const ;
      virtual BOOLEAN         inheritSessionAttr() const ;

      virtual const utilAddrContainer* getAddressList() const ;
      virtual CoordGroupInfoPtr        getGroupInfo() const ;

   protected:
      INT32  _buildGroupInfo( BSONObj &objGroup ) ;

   private:
      UTIL_DS_UID _id ;
      CHAR _name[ DATASOURCE_MAX_NAME_SZ + 1 ] ;
      CHAR _user[ SDB_MAX_USERNAME_LENGTH + 1 ] ;
      CHAR _password[ SDB_MAX_PASSWORD_LENGTH + 1 ] ;
      CHAR _errCtlLevel[ 8 ] ;
      UINT32 _accessMode ;
      BSONObj _options ;
      ossPoolString _address ;
      utilAddrContainer _addrArray ;
      CoordGroupInfoPtr _groupInfoPtr ;
      ossPoolString _dsVersion ;
      INT32         _dsMajorVersion ;
      INT32         _dsMinorVersion ;
      INT32         _dsFixVersion ;
      INT32         _errFilterMask ;
      SDB_DS_TRANS_PROPAGATE_MODE _transPropagateMode ;
      BOOLEAN       _inheritSessionAttr ;
   } ;
   typedef _coordSdbDataSource coordSdbDataSource ;
}

#endif /* COORD_SDBCOORDDATASOURCE_HPP__ */

