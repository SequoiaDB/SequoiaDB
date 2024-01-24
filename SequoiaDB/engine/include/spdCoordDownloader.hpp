/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

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

   Source File Name = spdCoordDownloader.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPDCOORDDOWNLOADER_HPP_
#define SPDCOORDDOWNLOADER_HPP_

#include "spdFuncDownloader.hpp"
#include "rtnContext.hpp"

namespace engine
{
   class _coordCommandBase ;
   class _SDB_RTNCB ;

   /*
      _spdCoordDownloader define
   */
   class _spdCoordDownloader : public _spdFuncDownloader
   {
   public:
      _spdCoordDownloader( _coordCommandBase *command,
                           _pmdEDUCB *cb ) ;
      virtual ~_spdCoordDownloader() ;

   public:
      virtual INT32 next( BSONObj &func ) ;
      virtual INT32 download( const BSONObj &matcher ) ;

   private:
      _rtnContextBuf _context ;
      SINT64 _contextID ;
      _coordCommandBase *_command ;
      _pmdEDUCB *_cb ;
      _SDB_RTNCB *_rtnCB ;
   } ;

   typedef class _spdCoordDownloader spdCoordDownloader ;
}

#endif // SPDCOORDDOWNLOADER_HPP_

