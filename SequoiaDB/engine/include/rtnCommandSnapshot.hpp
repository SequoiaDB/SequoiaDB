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

   Source File Name = rtnCommandSnapshot.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          23/06/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_COMMAND_SNAPSHOT_HPP_
#define RTN_COMMAND_SNAPSHOT_HPP_

#include "rtnCommandMon.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;

   class _rtnSnapshotInner : public _rtnMonInnerBase
   {
      protected:
         _rtnSnapshotInner () {}
         virtual ~_rtnSnapshotInner () {}

      protected:
         virtual BOOLEAN _isDetail() const { return TRUE ; }
   } ;

   class _rtnSnapshot : public _rtnMonBase
   {
      protected:
         _rtnSnapshot () {}
         virtual ~_rtnSnapshot () {}

      protected:
         virtual BOOLEAN _isDetail() const { return TRUE ; }

   } ;

   class _rtnSnapshotSystem : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotSystem () ;
         virtual ~_rtnSnapshotSystem () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotSystemInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSystemInner() {}
         virtual ~_rtnSnapshotSystemInner() {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   class _rtnSnapshotHealth : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotHealth () ;
         virtual ~_rtnSnapshotHealth () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotHealthInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotHealthInner() {}
         virtual ~_rtnSnapshotHealthInner() {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   class _rtnSnapshotContexts : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContexts () ;
         virtual ~_rtnSnapshotContexts () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotContextsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsInner() {}
         virtual ~_rtnSnapshotContextsInner() {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   class _rtnSnapshotContextsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsCurrent () ;
         virtual ~_rtnSnapshotContextsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotContextsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotContextsCurrentInner() {}
         virtual ~_rtnSnapshotContextsCurrentInner() {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   class _rtnSnapshotDatabase : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotDatabase () ;
         virtual ~_rtnSnapshotDatabase () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotDatabaseInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotDatabaseInner () {}
         virtual ~_rtnSnapshotDatabaseInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   };

   class _rtnSnapshotCollections : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollections () ;
         virtual ~_rtnSnapshotCollections () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotCollectionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionsInner () {}
         virtual ~_rtnSnapshotCollectionsInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   };

   class _rtnSnapshotCollectionSpaces : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionSpaces () ;
         virtual ~_rtnSnapshotCollectionSpaces () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotCollectionSpacesInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotCollectionSpacesInner () {}
         virtual ~_rtnSnapshotCollectionSpacesInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   };

   class _rtnSnapshotReset : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotReset () ;
         virtual ~_rtnSnapshotReset () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL ) ;
      private:
         RTN_COMMAND_TYPE _type ;
         EDUID _sessionID ;
         BOOLEAN _resetAllSession ;

   };

   class _rtnSnapshotSessions : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessions () ;
         virtual ~_rtnSnapshotSessions () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotSessionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsInner () {}
         virtual ~_rtnSnapshotSessionsInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   };

   class _rtnSnapshotSessionsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsCurrent () ;
         virtual ~_rtnSnapshotSessionsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotSessionsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotSessionsCurrentInner () {}
         virtual ~_rtnSnapshotSessionsCurrentInner () {}
         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   };

   class _rtnSnapshotTransactionsCurrent : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsCurrent () ;
         virtual ~_rtnSnapshotTransactionsCurrent () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   class _rtnSnapshotTransactionsCurrentInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsCurrentInner () {}
         virtual ~_rtnSnapshotTransactionsCurrentInner () {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   class _rtnSnapshotTransactions : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactions () ;
         virtual ~_rtnSnapshotTransactions () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   } ;

   class _rtnSnapshotTransactionsInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotTransactionsInner () {}
         virtual ~_rtnSnapshotTransactionsInner () {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

   /*
      _rtnSnapshotAccessPlans define
    */
   class _rtnSnapshotAccessPlans : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER () ;

      public :
         _rtnSnapshotAccessPlans () ;
         virtual ~_rtnSnapshotAccessPlans () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected :
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private :
         virtual const CHAR *getIntrCMDName() ;
   } ;

   /*
      _rtnSnapshotAccessPlansInner define
    */
   class _rtnSnapshotAccessPlansInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotAccessPlansInner () ;
         virtual ~_rtnSnapshotAccessPlansInner () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType () const ;
         virtual BOOLEAN _isCurrent () const ;
         virtual UINT32  _addInfoMask () const ;
   } ;

   class _rtnSnapshotConfig : public _rtnSnapshot
   {
      DECLARE_CMD_AUTO_REGISTER()

      public :
         _rtnSnapshotConfig () ;
         virtual ~_rtnSnapshotConfig () ;

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;

      private:
         virtual const CHAR *getIntrCMDName() ;
   };

   class _rtnSnapshotConfigInner : public _rtnSnapshotInner
   {
      DECLARE_CMD_AUTO_REGISTER()

      public:
         _rtnSnapshotConfigInner() {}
         virtual ~_rtnSnapshotConfigInner() {}

         virtual const CHAR * name () ;
         virtual RTN_COMMAND_TYPE type () ;

      protected:
         virtual INT32   _getFetchType() const ;
         virtual BOOLEAN _isCurrent() const ;
         virtual UINT32  _addInfoMask() const ;
   } ;

}

#endif //RTN_COMMAND_SNAPSHOT_HPP_

