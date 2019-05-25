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

   Source File Name = omagentNodeCmd.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/08/2014  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_NODECMD_HPP__
#define OMAGENT_NODECMD_HPP__

#include "omagentCmdBase.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _omaShutdownCmd define
   */
   class _omaShutdownCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaShutdownCmd() ;
         virtual ~_omaShutdownCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;
   } ;

   /*
      _omaSetPDLevelCmd define
   */
   class _omaSetPDLevelCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaSetPDLevelCmd() ;
         virtual ~_omaSetPDLevelCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

      private:
         INT32       _pdLevel ;
   } ;

   /*
      _omaCreateNodeCmd define
   */
   class _omaCreateNodeCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaCreateNodeCmd() ;
         virtual ~_omaCreateNodeCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

      protected:
         BSONObj        _config ;
         string         _roleStr ;
   } ;

   /*
      _omaRemoveNodeCmd define
   */
   class _omaRemoveNodeCmd : public _omaCreateNodeCmd
   {
      DECLARE_OACMD_AUTO_REGISTER()

      public:
         _omaRemoveNodeCmd() ;
         virtual ~_omaRemoveNodeCmd() ;

         virtual const CHAR * name () ;
         virtual INT32 doit ( BSONObj &retObj ) ;
   } ;

   /*
      _omaStartNodeCmd define
   */
   class _omaStartNodeCmd : public _omaCommand
   {
      DECLARE_OACMD_AUTO_REGISTER()

      virtual BOOLEAN needCheckBusiness() const { return FALSE ; }

      public:
         _omaStartNodeCmd() ;
         virtual ~_omaStartNodeCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 init ( const CHAR *pInfomation ) ;

         virtual INT32 doit ( BSONObj &retObj ) ;

      protected:
         const CHAR        *_pData ;

   } ;

   /*
      _omaStopNodeCmd define
   */
   class _omaStopNodeCmd : _omaStartNodeCmd
   {
      DECLARE_OACMD_AUTO_REGISTER()

      public:
         _omaStopNodeCmd() ;
         virtual ~_omaStopNodeCmd() ;

         virtual const CHAR * name () ;

         virtual INT32 doit ( BSONObj &retObj ) ;
   } ;

}

#endif //OMAGENT_NODECMD_HPP__

