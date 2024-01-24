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

   Source File Name = coordCommandRecycleBin.hpp

   Descriptive Name = Coord Common

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_CMD_RECYCLEBIN_HPP__
#define COORD_CMD_RECYCLEBIN_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "coordCommandCommon.hpp"
#include "coordCommandDC.hpp"
#include "coordCommandData.hpp"
#include "rtnCommand.hpp"

namespace engine
{

   /*
      _coordGetRecycleBinDetail define
    */
   class _coordGetRecycleBinDetail : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordGetRecycleBinDetail() ;
      virtual ~_coordGetRecycleBinDetail() ;

   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 std::string &clName,
                                 BSONObj &outSelector ) ;
   } ;

   typedef class _coordGetRecycleBinDetail coordGetRecycleBinDetail ;

   /*
      _coordAlterRecycleBin define
    */
   class _coordAlterRecycleBin : public _coordCommandBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordAlterRecycleBin() ;
      virtual ~_coordAlterRecycleBin() ;

      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;
   } ;

   typedef class _coordAlterRecycleBin coordAlterRecycleBin ;

   /*
      _coordGetRecycleBinCount define
    */
   class _coordGetRecycleBinCount : public _coordCMDQueryBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordGetRecycleBinCount() ;
      virtual ~_coordGetRecycleBinCount() ;

      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 std::string &clName,
                                 BSONObj &outSelector )
      {
         return SDB_OK ;
      }
   } ;

   typedef class _coordGetRecycleBinCount coordGetRecycleBinCount ;

   /*
      _coordDropRecycleBinBase define
    */
   class _coordDropRecycleBinBase : public _coordCommandBase
   {
   public:
      _coordDropRecycleBinBase( BOOLEAN isDropAll ) ;
      virtual ~_coordDropRecycleBinBase() ;

      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      INT32 _parseMessage( MsgHeader *pMsg ) ;
      INT32 _doAudit( INT32 rc ) ;

   protected:
      bson::BSONObj  _options ;
      BOOLEAN        _isDropAll ;
      const CHAR *   _recycleItemName ;
   } ;

   typedef class _coordDropRecycleBinBase coordDropRecycleBinBase ;

   /*
      _coordDropRecycleBinItem define
    */
   class _coordDropRecycleBinItem : public _coordDropRecycleBinBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordDropRecycleBinItem() ;
      virtual ~_coordDropRecycleBinItem() ;
   } ;

   typedef class _coordDropRecycleBinItem coordDropRecycleBinItem ;

   /*
      _coordDropRecycleBinAll define
    */
   class _coordDropRecycleBinAll : public _coordDropRecycleBinBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordDropRecycleBinAll() ;
      virtual ~_coordDropRecycleBinAll() ;
   } ;

   typedef class _coordDropRecycleBinAll coordDropRecycleBinAll ;

   /*
      _coordReturnRecycleBinBase define
    */
   class _coordReturnRecycleBinBase : public _coordDataCMD3Phase
   {
   public:
      _coordReturnRecycleBinBase() {}
      virtual ~_coordReturnRecycleBinBase() {}

   protected:
      typedef _coordDataCMD3Phase _BASE ;

      /*
         command on collection
       */
      virtual BOOLEAN _flagDoOnCollection () { return FALSE ; }

      virtual INT32 _regEventHandlers() ;
      virtual INT32 _generateCataMsg( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      coordCMDArguments *pArgs,
                                      CHAR **ppMsgBuf,
                                      INT32 *pBufSize )
      {
         *ppMsgBuf = (CHAR*)pMsg ;
         *pBufSize = pMsg->messageLength ;

         return SDB_OK ;
      }

      // rewrite do audit
      virtual INT32 _doAudit( coordCMDArguments *pArgs, INT32 rc ) ;

      /*
         Get output to client
       */
      virtual INT32 _doOutput( rtnContextBuf *buf ) ;

   protected:
      coordCMDRtrnTaskHandler _taskHandler ;
      coordCMDRecycleHandler  _recycleHandler ;
      coordCMDReturnHandler   _returnHandler ;
   } ;

   typedef class _coordReturnRecycleBinBase coordReturnRecycleBinBase ;

   /*
      _coordReturnRecycleBinItem define
    */
   class _coordReturnRecycleBinItem : public _coordReturnRecycleBinBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordReturnRecycleBinItem() {}
      virtual ~_coordReturnRecycleBinItem() {}

   protected:
      virtual INT32 _parseMsg( MsgHeader *pMsg,
                               coordCMDArguments *pArgs ) ;
   } ;

   typedef class _coordReturnRecycleBinItem coordReturnRecycleBinItem ;

   /*
      _coordReturnRecycleBinItemToName define
    */
   class _coordReturnRecycleBinItemToName : public _coordReturnRecycleBinBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

   public:
      _coordReturnRecycleBinItemToName() {}
      virtual ~_coordReturnRecycleBinItemToName() {}

   protected:
      virtual INT32 _parseMsg( MsgHeader *pMsg,
                               coordCMDArguments *pArgs ) ;
   } ;

   typedef class _coordReturnRecycleBinItemToName coordReturnRecycleBinItemToName ;

}

#endif // COORD_CMD_RECYCLEBIN_HPP__
