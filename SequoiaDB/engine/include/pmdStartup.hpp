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

   Source File Name = pmdStartup.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef PMD_STARTUP_HPP_
#define PMD_STARTUP_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossIO.hpp"
#include <string>

#define PMD_STARTUP_FILE_NAME          ".SEQUOIADB_STARTUP"

namespace engine
{

   /*
      PMD Start type define
   */
   enum SDB_START_TYPE
   {
      SDB_START_NORMAL  = 0,
      SDB_START_CRASH,
      SDB_START_ERROR
   } ;

   const CHAR* pmdGetStartTypeStr( SDB_START_TYPE type ) ;

   SDB_START_TYPE pmdStr2StartType( const CHAR* str ) ;

   /*
      _pmdStartup define
   */
   class _pmdStartup : public SDBObject
   {
      public:
         _pmdStartup () ;
         ~_pmdStartup () ;

         INT32 init ( const CHAR *pPath, BOOLEAN onlyCheck = FALSE ) ;
         INT32 final () ;
         void  ok ( BOOLEAN bOK = TRUE ) ;
         BOOLEAN isOK () const ;
         void  restart( BOOLEAN bRestart, INT32 rc ) ;
         SDB_START_TYPE getStartType() const { return _startType ; }

         BOOLEAN  needRestart() const ;

      protected:
         INT32 _writeStartStr( SDB_START_TYPE startType,
                               BOOLEAN ok ) ;

      private:
         OSSFILE           _file ;
         std::string       _fileName ;
         BOOLEAN           _ok ;
         SDB_START_TYPE    _startType ;
         BOOLEAN           _fileOpened ;
         BOOLEAN           _fileLocked ;
         BOOLEAN           _restart ;
   };

   typedef _pmdStartup pmdStartup ;

   pmdStartup& pmdGetStartup () ;

}

#endif //PMD_STARTUP_HPP_

