/*******************************************************************************


   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnContextDel.hpp

   Descriptive Name = RunTime Delete Operation Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          5/26/2017   David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_DEL_HPP_
#define RTN_CONTEXT_DEL_HPP_

#include "rtnContext.hpp"

namespace engine
{
   /*
      _rtnContextDelCS define
   */
   class _clsCatalogAgent ;
   class dpsTransCB ;
   class _SDB_DMSCB ;

   class _rtnContextDelCS : public _rtnContextBase
   {
      enum delCSPhase
      {
         DELCSPHASE_0 = 0,
         DELCSPHASE_1
      };
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _rtnContextDelCS( SINT64 contextID, UINT64 eduID ) ;
      ~_rtnContextDelCS();
      virtual std::string      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }

      INT32 open( const CHAR *pCollectionName,
                  _pmdEDUCB *cb );

      virtual INT32 getMore( INT32 maxNumToReturn, rtnContextBuf &buffObj,
                             _pmdEDUCB *cb );

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ){ return SDB_DMS_EOC; };
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _tryLock( const CHAR *pCollectionName,
                     _pmdEDUCB *cb );

      INT32 _releaseLock( _pmdEDUCB *cb );

      void _clean( _pmdEDUCB *cb );

   private:
      delCSPhase           _status;
      _SDB_DMSCB            *_pDmsCB;
      dpsTransCB           *_pTransCB;
      _clsCatalogAgent     *_pCatAgent;
      CHAR                 _name[ DMS_COLLECTION_SPACE_NAME_SZ + 1 ];
      UINT32               _gotLogSize;
      BOOLEAN              _gotDmsCBWrite;
      UINT32               _logicCSID;
   };
   typedef class _rtnContextDelCS rtnContextDelCS;

   /*
      _rtnContextDelCL define
   */
   class _rtnContextDelCL : public _rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _rtnContextDelCL( SINT64 contextID, UINT64 eduID );
      ~_rtnContextDelCL();
      virtual std::string      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }
      virtual BOOLEAN          isWrite() const { return TRUE ; }

      INT32 open( const CHAR *pCollectionName, _pmdEDUCB *cb,
                  INT16 w );

      virtual INT32 getMore( INT32 maxNumToReturn, rtnContextBuf &buffObj,
                             _pmdEDUCB *cb );

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ){ return SDB_DMS_EOC; };
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _tryLock( const CHAR *pCollectionName,
                      _pmdEDUCB *cb );

      INT32 _releaseLock( _pmdEDUCB *cb );

      void _clean( _pmdEDUCB *cb );

   private:
      _SDB_DMSCB           *_pDmsCB;
      _clsCatalogAgent     *_pCatAgent;
      dpsTransCB           *_pTransCB;
      CHAR                 _collectionName[ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
      const CHAR           *_clShortName ;
      BOOLEAN              _gotDmsCBWrite ;
      BOOLEAN              _hasLock ;
      BOOLEAN              _hasDropped ;

      _dmsStorageUnit      *_su ;
      _dmsMBContext        *_mbContext ;
   };
   typedef class _rtnContextDelCL rtnContextDelCL;

   /*
      _rtnContextDelMainCL define
   */
   class _SDB_RTNCB;
   class _rtnContextDelMainCL : public _rtnContextBase
   {
   typedef _utilMap< std::string, SINT64, 20 >  SUBCL_CONTEXT_LIST ;
      DECLARE_RTN_CTX_AUTO_REGISTER()
   public:
      _rtnContextDelMainCL( SINT64 contextID, UINT64 eduID );
      ~_rtnContextDelMainCL();
      virtual std::string      name() const ;
      virtual RTN_CONTEXT_TYPE getType () const;
      virtual _dmsStorageUnit* getSU () { return NULL ; }

      INT32 open( const CHAR *pCollectionName,
                  vector< string > &subCLList,
                  INT32 version,
                  _pmdEDUCB *cb,
                  INT16 w ) ;

      virtual INT32 getMore( INT32 maxNumToReturn, rtnContextBuf &buffObj,
                             _pmdEDUCB *cb ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ){ return SDB_DMS_EOC; };
      virtual void  _toString( stringstream &ss ) ;

   private:
      void _clean( _pmdEDUCB *cb );

   private:
      _clsCatalogAgent           *_pCatAgent;
      _SDB_RTNCB                 *_pRtncb;
      CHAR                       _name[ DMS_COLLECTION_FULL_NAME_SZ + 1 ];
      SUBCL_CONTEXT_LIST         _subContextList ;
      INT32                      _version ;

   };
   typedef class _rtnContextDelMainCL rtnContextDelMainCL;
}

#endif /* RTN_CONTEXT_DEL_HPP_ */

