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

   Source File Name = omagentRemoteUsrCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_REMOTE_USRCMD_HPP_
#define OMAGENT_REMOTE_USRCMD_HPP_

#include "omagentCmdBase.hpp"
#include "omagentRemoteBase.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{
   /*
      _remoteCmdRun define
   */
   class _remoteCmdRun : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteCmdRun() ;

         ~_remoteCmdRun() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteCmdStart define
   */
   class _remoteCmdStart : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteCmdStart() ;

         ~_remoteCmdStart() ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;
   } ;

   /*
      _remoteCmdRunJS define
   */
   class _remoteCmdRunJS : public _remoteExec
   {
      DECLARE_OACMD_AUTO_REGISTER()
      public:
         _remoteCmdRunJS() ;

         ~_remoteCmdRunJS() ;

         INT32 init ( const CHAR *pInfomation ) ;

         const CHAR *name() ;

         INT32 doit( BSONObj &retObj ) ;

         INT32 final( BSONObj &rval, BSONObj &retObj ) ;

      protected:
         string _code ;
         _sptScope* _jsScope ;
         BOOLEAN _isRelease ;
   } ;
}
#endif