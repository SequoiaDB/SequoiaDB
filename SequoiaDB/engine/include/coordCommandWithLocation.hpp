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

   Source File Name = coordCommandWithLocation.hpp

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

#ifndef COORD_COMMAND_WITH_LOCATION_HPP__
#define COORD_COMMAND_WITH_LOCATION_HPP__

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDExpConfig define
   */
   class _coordCMDExpConfig : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDExpConfig() ;
         virtual ~_coordCMDExpConfig() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;

         virtual INT32   _posExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     ROUTE_RC_MAP &faileds ) ;
   } ;
   typedef _coordCMDExpConfig coordCMDExpConfig ;

   /*
      _coordCMDInvalidateCache define
   */
   class _coordCMDInvalidateCache : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDInvalidateCache() ;
         virtual ~_coordCMDInvalidateCache() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) { return SDB_OK ; }
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;

         virtual INT32   _preExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     coordCtrlParam &ctrlParam,
                                     SET_RC &ignoreRCList ) ;
   } ;
   typedef _coordCMDInvalidateCache coordCMDInvalidateCache ;

   /*
      _coordCMDSyncDB define
   */
   class _coordCMDSyncDB : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDSyncDB() ;
         virtual ~_coordCMDSyncDB() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) { return flag ; }
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;

         virtual INT32   _preExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     coordCtrlParam &ctrlParam,
                                     SET_RC &ignoreRCList ) ;
   } ;
   typedef _coordCMDSyncDB coordCMDSyncDB ;

   /*
      _coordCmdLoadCS define
   */
   class _coordCmdLoadCS : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCmdLoadCS() ;
         virtual ~_coordCmdLoadCS() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) { return flag ; }
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;

         virtual INT32   _preExcute( MsgHeader *pMsg,
                                     pmdEDUCB *cb,
                                     coordCtrlParam &ctrlParam,
                                     SET_RC &ignoreRCList ) ;
   } ;
   typedef _coordCmdLoadCS coordCmdLoadCS ;

   /*
      _coordCmdUnloadCS define
   */
   typedef _coordCmdLoadCS _coordCmdUnloadCS ;

   /*
      _coordForceSession define
   */
   class _coordForceSession: public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordForceSession() ;
         virtual ~_coordForceSession() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;
   typedef _coordForceSession coordForceSession ;

   /*
      _coordSetPDLevel define
   */
   class _coordSetPDLevel : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordSetPDLevel() ;
         virtual ~_coordSetPDLevel() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;
   typedef _coordSetPDLevel coordSetPDLevel ;

   /*
      _coordReloadConf define
   */
   class _coordReloadConf : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordReloadConf() ;
         virtual ~_coordReloadConf() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;
   typedef _coordReloadConf coordReloadConf ;

   /*
      _coordUpdateConf define
   */
   class _coordUpdateConf : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordUpdateConf() ;
         virtual ~_coordUpdateConf() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;
   typedef _coordUpdateConf coordUpdateConf ;

   /*
      _coordDeleteConf define
   */
   class _coordDeleteConf : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordDeleteConf() ;
         virtual ~_coordDeleteConf() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) ;
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;
   typedef _coordDeleteConf coordDeleteConf ;

   /*
      _coordCMDAnalyze define
   */
   class _coordCMDAnalyze : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;

      public:
         _coordCMDAnalyze () ;

         virtual ~_coordCMDAnalyze () ;

      private:
         virtual BOOLEAN _useContext () { return FALSE ; }

         virtual INT32   _onLocalMode ( INT32 flag ) { return flag ; }

         virtual void    _preSet ( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;

         virtual UINT32  _getControlMask () const ;

         virtual INT32   _preExcute ( MsgHeader *pMsg,
                                      pmdEDUCB *cb,
                                      coordCtrlParam &ctrlParam,
                                      SET_RC &ignoreRCList ) ;

         INT32   _getCSGrps ( const CHAR *csname, pmdEDUCB *cb,
                              coordCtrlParam &ctrlParam ) ;

         INT32   _getCLGrps ( MsgHeader *pMsg, const CHAR *clname,
                              pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
   } ;

   typedef _coordCMDAnalyze coordCMDAnalyze ;

}

#endif // COORD_COMMAND_WITH_LOCATION_HPP__

