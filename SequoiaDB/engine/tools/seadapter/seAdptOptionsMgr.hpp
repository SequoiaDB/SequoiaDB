/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = seAdptOptionsMgr.hpp

   Descriptive Name = Search engine adapter options manager

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/08/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_OPTIONSMGR_HPP__
#define SEADPT_OPTIONSMGR_HPP__

#include "pmdOptionsMgr.hpp"

namespace seadapter
{
   class _seAdptOptionsMgr : public engine::_pmdCfgRecord
   {
   public:
      _seAdptOptionsMgr() ;
      virtual ~_seAdptOptionsMgr() {}

      INT32 init( INT32 argc, CHAR **argv, const CHAR *exePath ) ;

      void setSvcName( const CHAR *svcName ) ;
      const CHAR* getCfgFileName() const { return _cfgFileName ; }
      const CHAR* getSvcName() const { return _serviceName ; }
      const CHAR* getDbHost() const { return _dbHost ; }
      const CHAR* getDbService() const { return _dbService ; }
      const CHAR* getSeHost() const { return _seHost ; }
      const CHAR* getSeService() const { return _seService ; }
      PDLEVEL     getDiagLevel() const ;

   protected:
      virtual INT32 doDataExchange( engine::pmdCfgExchange *pEX ) ;

   private:
      CHAR     _cfgFileName[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR     _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;
      CHAR     _dbHost[ OSS_MAX_HOSTNAME + 1 ] ;
      CHAR     _dbService[ OSS_MAX_SERVICENAME + 1 ] ;
      CHAR     _seHost[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR     _seService[ OSS_MAX_SERVICENAME + 1 ] ;
      INT32    _diagLevel ;
   } ;
   typedef _seAdptOptionsMgr seAdptOptionsMgr ;
}

#endif /* SEADPT_OPTIONSMGR_HPP__ */
