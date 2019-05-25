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

   Source File Name = coordCommandStat.hpp

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

#ifndef COORD_COMMAND_STAT_HPP__
#define COORD_COMMAND_STAT_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDStatisticsBase define
   */
   class _coordCMDStatisticsBase : public _coordCommandBase
   {
      public:
         _coordCMDStatisticsBase() ;
         virtual ~_coordCMDStatisticsBase() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) = 0 ;

         virtual BOOLEAN openEmptyContext() const { return FALSE ; }
   } ;
   typedef _coordCMDStatisticsBase coordCMDStatisticsBase ;

   /*
      _coordCMDGetIndexes define
   */
   class _coordCMDGetIndexes : public _coordCMDStatisticsBase
   {
      typedef _utilMap< string, BSONObj, 10 >      CoordIndexMap ;

      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetIndexes() ;
         virtual ~_coordCMDGetIndexes() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetIndexes coordCMDGetIndexes ;

   /*
      _coordCMDGetCount define
   */
   class _coordCMDGetCount : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCount() ;
         virtual ~_coordCMDGetCount() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
         virtual BOOLEAN openEmptyContext() const { return TRUE ; }
   } ;
   typedef _coordCMDGetCount coordCMDGetCount ;

   /*
      _coordCMDGetDatablocks define
   */
   class _coordCMDGetDatablocks : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetDatablocks() ;
         virtual ~_coordCMDGetDatablocks() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetDatablocks coordCMDGetDatablocks ;

   /*
      _coordCMDGetQueryMeta define
   */
   class _coordCMDGetQueryMeta : public _coordCMDGetDatablocks
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetQueryMeta() ;
         virtual ~_coordCMDGetQueryMeta() ;
   } ;
   typedef _coordCMDGetQueryMeta coordCMDGetQueryMeta ;

}

#endif // COORD_COMMAND_STAT_HPP__
