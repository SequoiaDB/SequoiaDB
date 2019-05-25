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

   Source File Name = coordCommandCommon.hpp

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

#ifndef COORD_COMMAND_COMMON_HPP__
#define COORD_COMMAND_COMMON_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "aggrBuilder.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCmdWithLocation define
   */
   class _coordCmdWithLocation : public _coordCommandBase
   {
      public:
         _coordCmdWithLocation() ;
         virtual ~_coordCmdWithLocation() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      private:
         virtual BOOLEAN _useContext() = 0 ;
         virtual INT32   _onLocalMode( INT32 flag ) = 0 ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) = 0 ;
         virtual UINT32  _getControlMask() const = 0 ;

         virtual INT32   _preExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     coordCtrlParam &ctrlParam,
                                     SET_RC &ignoreRCList ) ;
         virtual INT32   _posExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     ROUTE_RC_MAP &faileds ) ;

   } ;
   typedef _coordCmdWithLocation coordCmdWithLocation ;

   /*
      _coordCMDMonIntrBase define
   */
   class _coordCMDMonIntrBase : public _coordCmdWithLocation
   {
      public:
         _coordCMDMonIntrBase() ;
         virtual ~_coordCMDMonIntrBase() ;

      private:
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual INT32   _onLocalMode( INT32 flag ) ;

   } ;
   typedef _coordCMDMonIntrBase coordCMDMonIntrBase ;

   /*
      _coordCMDMonCurIntrBase define
   */
   class _coordCMDMonCurIntrBase : public _coordCMDMonIntrBase
   {
      public:
         _coordCMDMonCurIntrBase() ;
         virtual ~_coordCMDMonCurIntrBase() ;
      private:
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;
   typedef _coordCMDMonCurIntrBase coordCMDMonCurIntrBase ;

   /*
      _coordAggrCmdBase define
   */
   class _coordAggrCmdBase : public _aggrCmdBase
   {
      public:
         _coordAggrCmdBase() ;
         virtual ~_coordAggrCmdBase() ;

         INT32 appendObjs( const CHAR *pInputBuffer,
                           CHAR *&pOutputBuffer,
                           INT32 &bufferSize,
                           INT32 &bufUsed,
                           INT32 &buffObjNum ) ;
   } ;
   typedef _coordAggrCmdBase coordAggrCmdBase ;

   /*
      _coordCMDMonBase define
   */
   class _coordCMDMonBase : public _coordCommandBase, public _coordAggrCmdBase
   {
      public:
         _coordCMDMonBase() ;
         virtual ~_coordCMDMonBase() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         virtual const CHAR *getIntrCMDName() = 0 ;
         virtual const CHAR *getInnerAggrContent() = 0 ;
         virtual BOOLEAN    _useContext() { return TRUE ; }
   } ;
   typedef _coordCMDMonBase coordCMDMonBase ;

   /*
      _coordCMDQueryBase define
   */
   class _coordCMDQueryBase : public _coordCommandBase
   {
   public:
      _coordCMDQueryBase() ;
      virtual ~_coordCMDQueryBase() ;

      virtual INT32 execute( MsgHeader *pMsg,
                             pmdEDUCB *cb,
                             INT64 &contextID,
                             rtnContextBuf *buf ) ;

   protected:
      virtual INT32 _preProcess( rtnQueryOptions &queryOpt,
                                 string &clName,
                                 BSONObj &outSelector ) = 0 ;
   } ;
   typedef _coordCMDQueryBase coordCMDQueryBase ;

}

#endif // COORD_COMMAND_COMMON_HPP__

