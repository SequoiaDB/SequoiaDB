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

   Source File Name = sptUsrOmaCommon.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/18/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USROMA_COMMON_HPP_
#define SPT_USROMA_COMMON_HPP_
#include "ossTypes.hpp"
#include "../bson/bson.hpp"
namespace engine
{
   class _sptUsrOmaCommon: public SDBObject
   {
   public:
      /*
         static functions
      */
      static INT32 getOmaInstallInfo( bson::BSONObj& retObj, string &err ) ;

      static INT32 getOmaInstallFile( string &retStr, string &err ) ;

      static INT32 getOmaConfigFile( string &retStr, string &err ) ;

      static INT32 getOmaConfigs( const bson::BSONObj &arg,
                                  bson::BSONObj &retObj,
                                  string &err ) ;

      static INT32 setOmaConfigs( const bson::BSONObj &arg,
                                  const bson::BSONObj &confObj,
                                  string &err ) ;

      static INT32 getAOmaSvcName( const bson::BSONObj &arg,
                                   string &retStr,
                                   string &err ) ;

      static INT32 addAOmaSvcName( const bson::BSONObj &valueObj,
                                   const bson::BSONObj &optionObj,
                                   const bson::BSONObj &matchObj,
                                   string &err ) ;

      static INT32 delAOmaSvcName( const bson::BSONObj &arg,
                                      string &err ) ;
   private:
      static INT32 _getConfFile( string &confFile ) ;

      static INT32  _getConfInfo( const string &confFile,
                                  bson::BSONObj &conf,
                                  string &err,
                                  BOOLEAN allowNotExist = FALSE ) ;

      static INT32  _confObj2Str( const bson::BSONObj &conf, string &str,
                                  string &err,
                                  const CHAR* pExcept = NULL ) ;
   } ;
}

#endif
