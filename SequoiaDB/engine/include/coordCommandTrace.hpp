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

   Source File Name = coordCommandTrace.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_TRACE_HPP__
#define COORD_COMMAND_TRACE_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDTraceStart define
   */
   class _coordCMDTraceStart : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceStart() ;
         virtual ~_coordCMDTraceStart() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceStart coordCMDTraceStart ;

   /*
      _coordCMDTraceResume define
   */
   class _coordCMDTraceResume : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceResume() ;
         virtual ~_coordCMDTraceResume() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceResume coordCMDTraceResume ;

   /*
      _coordCMDTraceStop define
   */
   class _coordCMDTraceStop : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceStop() ;
         virtual ~_coordCMDTraceStop() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceStop coordCMDTraceStop ;

   /*
      _coordCMDTraceStatus define
   */
   class _coordCMDTraceStatus : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public :
         _coordCMDTraceStatus() ;
         virtual ~_coordCMDTraceStatus() ;
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCMDTraceStatus coordCMDTraceStatus ;

}

#endif // COORD_COMMAND_TRACE_HPP__
