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

   Source File Name = coordCommandBackup.hpp

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

#ifndef COORD_COMMAND_BACKUP_HPP__
#define COORD_COMMAND_BACKUP_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "aggrBuilder.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordBackupBase define
   */
   class _coordBackupBase : public _coordCommandBase
   {
      public:
         _coordBackupBase() ;
         virtual ~_coordBackupBase() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

      protected:
         virtual FILTER_BSON_ID  _getGroupMatherIndex () = 0 ;
         virtual NODE_SEL_STY    _nodeSelWhenNoFilter () = 0 ;
         virtual BOOLEAN         _useContext () = 0 ;
         virtual UINT32          _getMask() const = 0 ;

   } ;
   typedef _coordBackupBase coordBackupBase ;

   /*
      _coordRemoveBackup define
   */
   class _coordRemoveBackup : public _coordBackupBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordRemoveBackup() ;
         virtual ~_coordRemoveBackup() ;

      protected:
         virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
         virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
         virtual BOOLEAN         _useContext () ;
         virtual UINT32          _getMask() const ;
   } ;
   typedef _coordRemoveBackup coordRemoveBackup ;

   /*
      _coordBackupOffline define
   */
   class _coordBackupOffline : public _coordBackupBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordBackupOffline() ;
         virtual ~_coordBackupOffline() ;

      protected:
         virtual FILTER_BSON_ID  _getGroupMatherIndex () ;
         virtual NODE_SEL_STY    _nodeSelWhenNoFilter () ;
         virtual BOOLEAN         _useContext () ;
         virtual UINT32          _getMask() const ;

         virtual BOOLEAN         _interruptWhenFailed() const ;
   } ;
   typedef _coordBackupOffline coordBackupOffline ;

}

#endif // COORD_COMMAND_BACKUP_HPP__
