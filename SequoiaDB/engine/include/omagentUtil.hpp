/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = omagentUtil.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_UTIL_HPP_
#define OMAGENT_UTIL_HPP_

#include "core.hpp"
#include "ossUtil.hpp"
#include "omagentDef.hpp"

#include <string>
#include <vector>

#include "../bson/bson.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   INT32 checkBuffer ( CHAR **ppBuffer, INT32 *bufferSize,
                       INT32 packetLength ) ;

   INT32 readFile ( const CHAR * name , CHAR ** buf , UINT32 * bufSize,
                    UINT32 * readSize ) ;
   BOOLEAN portCanUsed ( UINT32 port, INT32 timeoutMilli = 1000 ) ; 

   // get bson field
   INT32 omaGetIntElement ( const BSONObj &obj, const CHAR *fieldName,
                            INT32 &value ) ;

   INT32 omaGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               const CHAR **value ) ;
   INT32 omaGetStringElement ( const BSONObj &obj, const CHAR *fieldName,
                               string& value ) ;

   INT32 omaGetObjElement ( const BSONObj &obj, const CHAR *fieldName,
                            BSONObj &value ) ;

   INT32 omaGetSubObjArrayElement ( const BSONObj &obj,
                                    const CHAR *objFieldName, 
                                    const CHAR *subObjFieldName, 
                                    const CHAR *subObjNewFieldName,
                                    BSONObjBuilder &builder ) ;

   INT32 omaGetBooleanElement ( const BSONObj &obj, const CHAR *fieldName,
                                BOOLEAN &value ) ;

   /*
      Node Manager Tool Functions Define
   */
   INT32          omStartDBNode( const CHAR *pExecName,
                                 const CHAR *pCfgPath,
                                 const CHAR *pSvcName,
                                 OSSPID &pid,
                                 BOOLEAN useCurUser = FALSE ) ;

   INT32          omStopDBNode( const CHAR *pExecName,
                                const CHAR *pServiceName,
                                BOOLEAN force = FALSE ) ;

   INT32          omGetSvcListFromConfig( const CHAR *pCfgRootDir,
                                          vector< string > &svcList ) ;

   INT32          omCheckDBProcessBySvc( const CHAR *svcname,
                                         BOOLEAN &isRuning,
                                         OSSPID &pid ) ;

   string         omPickNodeOutString( const string &out,
                                       const CHAR *pSvcname ) ;

}

#endif // OMAGENT_UTIL_HPP_


