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

   Source File Name = rtnContextAlter.hpp

   Descriptive Name = RunTime Alter Operation Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          3/12/2018   HGM Init draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_CONTEXT_ALTER_HPP_
#define RTN_CONTEXT_ALTER_HPP_

#include "rtnContext.hpp"
#include "rtnAlterJob.hpp"

namespace engine
{

   typedef enum _RTN_ALTER_PHASE
   {
      RTN_ALTER_PHASE_0 = 0,
      RTN_ALTER_PHASE_1
   } RTN_ALTER_PHASE ;

   /*
      _rtnContextAlterBase define
    */
   class _rtnContextAlterBase : public _rtnContextBase,
                                public _rtnAlterJobHolder
   {
      public :
         _rtnContextAlterBase ( SINT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextAlterBase() ;

         INT32 open ( rtnAlterJobHolder & holder,
                      _pmdEDUCB * cb, INT16 w ) ;

         OSS_INLINE virtual _dmsStorageUnit * getSU ()
         {
            return NULL ;
         }

         OSS_INLINE virtual BOOLEAN isWrite () const
         {
            return TRUE ;
         }

         virtual const CHAR *getProcessName() const
         {
            return ( NULL != _alterJob &&
                     NULL != _alterJob->getObjectName() ) ?
                   ( _alterJob->getObjectName() ) :
                   ( "" ) ;
         }

      protected :
         void _close ( _pmdEDUCB * cb ) ;
         virtual INT32 _openInternal ( _pmdEDUCB * cb ) = 0 ;
         virtual INT32 _closeInternal ( _pmdEDUCB * cb ) = 0 ;

         virtual INT32 _checkWritable ( _pmdEDUCB * cb ) ;
         virtual void _releaseWritable ( _pmdEDUCB * cb ) ;

         virtual INT32 _reserveLogSpace (  _pmdEDUCB * cb ) ;
         virtual void _releaseLogSpace ( _pmdEDUCB * cb ) ;

         virtual INT32 _lockTransaction ( _pmdEDUCB * cb ) = 0 ;
         virtual void _releaseTransaction ( _pmdEDUCB * cb ) = 0 ;

      protected :
         _SDB_DMSCB *         _dmsCB ;
         dpsTransCB *         _transCB;
         UINT32               _reservedLogSpace ;
         BOOLEAN              _checkedWritable ;
         RTN_ALTER_PHASE      _phase ;
   } ;

   typedef class _rtnContextAlterBase rtnContextAlterBase ;

   /*
      _rtnContextAlterCS define
    */
   class _rtnContextAlterCS : public _rtnContextAlterBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextAlterCS )

      public :
         _rtnContextAlterCS ( SINT64 contextID, UINT64 eduID ) ;
         ~_rtnContextAlterCS () ;

         OSS_INLINE virtual const CHAR* name () const
         {
            return "ALTERCS" ;
         }

         OSS_INLINE virtual RTN_CONTEXT_TYPE getType () const
         {
            return RTN_CONTEXT_ALTERCS ;
         }

      protected :
         virtual INT32 _openInternal ( _pmdEDUCB * cb ) ;
         virtual INT32 _closeInternal ( _pmdEDUCB * cb ) ;
         virtual INT32 _prepareData ( _pmdEDUCB * cb ) ;
         virtual void _toString ( stringstream & ss ) ;

         virtual INT32 _lockTransaction ( _pmdEDUCB * cb ) ;
         virtual void _releaseTransaction ( _pmdEDUCB * cb ) ;

      protected :
         UINT32            _logicalCSID ;
         _dmsStorageUnit * _su ;
   } ;

   typedef class _rtnContextAlterCS rtnContextAlterCS ;

   /*
      _rtnContextAlterCL define
    */
   class _rtnContextAlterCL : public _rtnContextAlterBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextAlterCL )

      public :
         _rtnContextAlterCL ( SINT64 contextID, UINT64 eduID ) ;
         ~_rtnContextAlterCL () ;

         OSS_INLINE virtual const CHAR* name () const
         {
            return "ALTERCL" ;
         }

         OSS_INLINE virtual RTN_CONTEXT_TYPE getType () const
         {
            return RTN_CONTEXT_ALTERCL ;
         }

      protected :
         virtual INT32 _openInternal ( _pmdEDUCB * cb ) ;
         virtual INT32 _closeInternal ( _pmdEDUCB * cb ) ;
         virtual INT32 _prepareData ( _pmdEDUCB * cb ) ;
         virtual void _toString ( stringstream & ss ) ;

         virtual INT32 _lockTransaction ( _pmdEDUCB * cb ) ;
         virtual void _releaseTransaction ( _pmdEDUCB * cb ) ;

         INT32 _checkCompress () ;
         INT32 _checkExtOptions () ;

      protected :
         UINT32            _logicalCSID ;
         UINT16            _mbID ;
         _dmsStorageUnit * _su ;
         _dmsMBContext *   _mbContext ;
   } ;

   typedef class _rtnContextAlterCL rtnContextAlterCL ;
}

#endif /* RTN_CONTEXT_ALTER_HPP_ */
