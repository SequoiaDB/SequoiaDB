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

   Source File Name = coordLobOperator.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/

#ifndef COORD_LOB_OPERATOR_HPP__
#define COORD_LOB_OPERATOR_HPP__

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordOpenLob define
   */
   class _coordOpenLob : public _coordOperator
   {
      public:
         _coordOpenLob() ;
         virtual ~_coordOpenLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordOpenLob coordOpenLob ;

   /*
      _coordWriteLob define
   */
   class _coordWriteLob : public _coordOperator
   {
      public:
         _coordWriteLob() ;
         virtual ~_coordWriteLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordWriteLob coordWriteLob ;

   /*
      _coordReadLob define
   */
   class _coordReadLob : public _coordOperator
   {
      public:
         _coordReadLob() ;
         virtual ~_coordReadLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordReadLob coordReadLob ;

   /*
      _coordLockLob define
   */
   class _coordLockLob : public _coordOperator
   {
      public:
         _coordLockLob() ;
         virtual ~_coordLockLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordLockLob coordLockLob ;

   /*
      _coordCloseLob define
   */
   class _coordCloseLob : public _coordOperator
   {
      public:
         _coordCloseLob() ;
         virtual ~_coordCloseLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordCloseLob coordCloseLob ;

   /*
      _coordRemoveLob define
   */
   class _coordRemoveLob : public _coordOperator
   {
      public:
         _coordRemoveLob() ;
         virtual ~_coordRemoveLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordRemoveLob coordRemoveLob ;

   /*
      _coordTruncateLob define
   */
   class _coordTruncateLob : public _coordOperator
   {
      public:
         _coordTruncateLob() ;
         virtual ~_coordTruncateLob() ;
      public:
         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
   } ;
   typedef _coordTruncateLob coordTruncateLob ;
}

#endif // COORD_LOB_OPERATOR_HPP__

