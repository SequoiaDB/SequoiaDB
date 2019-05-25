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

   Source File Name = omagentNodeMgr.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/04/2015  LZ Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OMAGENT_NODEPATHGUARD_HPP_
#define OMAGENT_NODEPATHGUARD_HPP_

#include "oss.hpp"
#include "ossSocket.hpp"
#include "pmdOptionsMgr.hpp"
#include <vector>

namespace engine {

   class _omaNodePathGuard : public SDBObject
   {
   public:
      _omaNodePathGuard() ;
      ~_omaNodePathGuard() ;

      void  init( const CHAR *nodeName, pmdOptionsCB *options ) ;

      const CHAR* name() const { return _nodeName ; }
      std::vector< std::string > *getPaths() { return &_nodePaths ; }

      BOOLEAN muteXOn( _omaNodePathGuard *pOther ) ;
      INT32   checkValid( pmdOptionsCB *options ) ;

   protected:
      INT32    _checkExistedFiles( const CHAR* path,
                                   const CHAR *filter = NULL,
                                   UINT32 deep = 1 ) ;

   private:
      CHAR _nodeName[ OSS_MAX_SERVICENAME + 1 ] ;
      std::vector<std::string> _nodePaths ;

   };

   typedef _omaNodePathGuard omaNodePathGuard ;
}

#endif // OMAGENT_NODEPATHGUARD_HPP_

