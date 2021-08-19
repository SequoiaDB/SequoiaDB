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

      protected:

         virtual INT32   _preExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     coordCtrlParam &ctrlParam,
                                     SET_RC &ignoreRCList ) ;
         virtual INT32   _posExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     ROUTE_RC_MAP &faileds ) ;

         virtual INT32 _handleHints ( BSONObj &hint, UINT32 mask ) ;
         virtual COORD_SHOWERROR_TYPE _getShowErrorType ()
         {
            return _showError ;
         }
         virtual COORD_SHOWERRORMODE_TYPE _getShowErrorModeType ()
         {
            return _showErrorMode ;
         }

      protected :
         INT32   _getCSGrps ( const CHAR * collectionSpace,
                              pmdEDUCB * cb,
                              coordCtrlParam & ctrlParam ) ;

         INT32   _getCLGrps ( MsgHeader * message,
                              const CHAR * collection,
                              pmdEDUCB * cb,
                              coordCtrlParam & ctrlParam ) ;

      protected:
         COORD_SHOWERROR_TYPE _showError ;
         COORD_SHOWERRORMODE_TYPE _showErrorMode ;
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

      protected:
         virtual INT32 _handleHints ( BSONObj &hint, UINT32 mask ) ;
         virtual COORD_SHOWERROR_TYPE _getShowErrorType ()
         {
            return _showError ;
         }
         virtual COORD_SHOWERRORMODE_TYPE _getShowErrorModeType ()
         {
            return _showErrorMode ; 
         }

      protected:
         COORD_SHOWERROR_TYPE _showError ;
         COORD_SHOWERRORMODE_TYPE _showErrorMode ;

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

      virtual INT32 _processVCS( rtnQueryOptions &queryOpt,
                                 const CHAR *pName,
                                 rtnContext *pContext ) ;

   protected:
      INT32       _processQueryVCS( rtnQueryOptions &queryOpt,
                                    const CHAR *pName,
                                    pmdEDUCB *cb,
                                    INT64 &contextID,
                                    rtnContextBuf *buf ) ;

   } ;
   typedef _coordCMDQueryBase coordCMDQueryBase ;

}

#endif // COORD_COMMAND_COMMON_HPP__

