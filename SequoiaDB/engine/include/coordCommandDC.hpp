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

   Source File Name = coordCommandDC.hpp

   Descriptive Name = Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/11/15    XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_DC_HPP__
#define COORD_COMMAND_DC_HPP__

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordAlterDC define
   */
   class _coordAlterDC : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordAlterDC() ;
         virtual ~_coordAlterDC() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      protected:
         INT32       _executeByNodes( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      CoordGroupList &groupLst,
                                      const CHAR *pAction,
                                      rtnContextBuf *buf ) ;

         INT32       _executeByGroups( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       CoordGroupList &groupLst,
                                       const CHAR *pAction,
                                       rtnContextBuf *buf ) ;

   } ;
   typedef _coordAlterDC coordAlterDC ;

   /*
      _coordGetDCInfo define
   */
   class _coordGetDCInfo : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordGetDCInfo() ;
         virtual ~_coordGetDCInfo() ;

      protected:
         virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                    string &clName,
                                    BSONObj &outSelector ) ;
   } ;
   typedef _coordGetDCInfo coordGetDCInfo ;

}

#endif // COORD_COMMAND_DC_HPP__

