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

   Source File Name = sptUsrRemoteAssit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/07/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/


#ifndef SPT_USRREMOTE_ASSIT_HPP__
#define SPT_USRREMOTE_ASSIT_HPP__

#include "msg.h"
#include "oss.hpp"
#include "sptRemote.hpp"
#include <string>
using std::string ;
namespace engine
{
   /*
      sptUsrRemoteAssit define
   */
   class _sptUsrRemoteAssit : public SDBObject
   {
      public:
         _sptUsrRemoteAssit() ;

         ~_sptUsrRemoteAssit() ;

      public:
         INT32       connect( const CHAR *pHostName,
                              const CHAR *pServiceName ) ;

         INT32       disconnect() ;

         INT32       runCommand( string command,
                                 const CHAR* arg1,
                                 CHAR **ppRetBuffer,
                                 INT32 &retCode,
                                 BOOLEAN needRecv = TRUE ) ;
         inline ossValuePtr getHandle()
         {
            return _handle ;
         } ;

      private:
         ossValuePtr _handle ;
         sptRemote   _remote ;
   } ;
   typedef _sptUsrRemoteAssit sptUsrRemoteAssit ;
}
#endif