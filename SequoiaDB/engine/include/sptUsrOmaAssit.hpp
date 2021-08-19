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

   Source File Name = sptUsrOmaAssit.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USROMA_ASSIT_HPP__
#define SPT_USROMA_ASSIT_HPP__

#include "oss.hpp"
#include "sptUsrRemoteAssit.hpp"
#include <string>
using std::string ;

namespace engine
{

   /*
      sptUsrOmaAssit define
   */
   class _sptUsrOmaAssit : public _sptUsrRemoteAssit
   {
      public:
         _sptUsrOmaAssit() ;
         ~_sptUsrOmaAssit() ;

      public:
         INT32       connect( const CHAR *pHostName,
                              const CHAR *pServiceName ) ;
         INT32       disconnect() ;

         INT32       createNode( const CHAR *pSvcName,
                                 const CHAR *pDBPath,
                                 const CHAR *pConfig ) ;

         INT32       removeNode( const CHAR *pSvcName,
                                 const CHAR * pConfig ) ;

         INT32       startNode( const CHAR *pSvcName ) ;

         INT32       stopNode( const CHAR *pSvcName ) ;

      protected:
         INT32       _getNodeHandle( const CHAR *pSvcName,
                                     ossValuePtr &handle ) ;
         void        _releaseNodeHandle( ossValuePtr handle ) ;

         INT32       _getCoordGroupHandle( ossValuePtr &handle ) ;

         INT32       _regSocket( ossValuePtr pSock ) ;

      private:
         ossValuePtr          _groupHandle ;
   } ;
   typedef _sptUsrOmaAssit sptUsrOmaAssit ;

}

#endif // SPT_USROMA_ASSIT_HPP__
