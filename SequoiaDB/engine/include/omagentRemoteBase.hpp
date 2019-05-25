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

   Source File Name = omagentRemoteBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_REMOTE_BASE_HPP__
#define OMAGENT_REMOTE_BASE_HPP__

#include "omagentCmdBase.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _remoteExec define
   */
   class _remoteExec : public _omaCommand
   {
      public:
         _remoteExec() ;

         virtual ~_remoteExec() ;

         virtual INT32 init( const CHAR * pInfomation ) ;

         virtual const CHAR * name() = 0 ;

         virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      protected:
         BSONObj _optionObj ;
         BSONObj _matchObj ;
         BSONObj _valueObj ;

   } ;

   /*
      _remoteOmaConfigs define
   */
   class _remoteOmaConfigs : public _remoteExec
   {
      public:
         _remoteOmaConfigs() ;

         virtual ~_remoteOmaConfigs() ;

         virtual const CHAR * name() = 0 ;

      protected:
         INT32  _confObj2Str( const bson::BSONObj &conf, string &str,
                              string &errMsg, const CHAR* pExcept = NULL ) ;

         INT32 _getNodeConfigFile( string svcname, string &filePath ) ;

         INT32 _getNodeConfInfo( const string & confFile, bson::BSONObj &conf,
                             string &errMsg, BOOLEAN allowNotExist = FALSE  ) ;
   } ;
}
#endif
