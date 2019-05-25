/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = dpsArchiveInfo.hpp

   Descriptive Name = Data Protection Services Log Archive Info

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/28/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSARCHIVEINFO_HPP_
#define DPSARCHIVEINFO_HPP_

#include "dpsLogDef.hpp"
#include "ossFile.hpp"
#include "ossLatch.hpp"
#include "../bson/bsonobj.h"
#include <string>

using namespace std ;
using namespace bson ;

namespace engine
{
   struct dpsArchiveInfo: public SDBObject
   {
      DPS_LSN startLSN ;

      dpsArchiveInfo& operator=( const dpsArchiveInfo& info )
      {
         startLSN = info.startLSN ;
         return *this ;
      }
   } ;

   class dpsArchiveInfoMgr: public SDBObject
   {
   public:
      dpsArchiveInfoMgr() ;
      ~dpsArchiveInfoMgr() ;
      INT32 init( const CHAR* archivePath ) ;
      dpsArchiveInfo getInfo() ;
      INT32 updateInfo( dpsArchiveInfo& info ) ;

   private:
      INT32 _initInfo() ;
      INT32 _open( const string& fileName, ossFile& file ) ;
      INT32 _toBson( BSONObj& data ) ;
      INT32 _fromBson( const BSONObj& data, 
                       dpsArchiveInfo& info,
                       INT64& count ) ;

   private:
      string         _path ;
      ossFile        _file1 ;
      ossFile        _file2 ;
      dpsArchiveInfo _info ;
      _ossSpinXLatch _mutex ;
      INT64          _count ;
   } ;
}

#endif /* DPSARCHIVEINFO_HPP_ */

