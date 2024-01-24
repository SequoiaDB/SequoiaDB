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

   Source File Name = rtnContextMain.hpp

   Descriptive Name = RunTime Main-Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2017   David Li  draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_MAIN_CONTEXT_HPP_
#define RTN_MAIN_CONTEXT_HPP_

#include "oss.hpp"
#include "rtnContext.hpp"
#include "rtnSubContext.hpp"
#include "rtnContextDataDispatcher.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   class _pmdEDUCB ;
   typedef ossPoolList< rtnSubContext* >           LST_SUB_CTX_PTR ;

   class _rtnContextMain : public _rtnContextBase,
                           public _rtnCtxDataDispatcher
   {
   protected:
      typedef ossPoolMultiSet< rtnSubContext*,
                               rtnSubContextComperator > SUB_ORDERED_CTX_SET ;
      typedef SUB_ORDERED_CTX_SET::iterator SUB_ORDERED_CTX_SET_IT ;
      typedef std::pair< SUB_ORDERED_CTX_SET_IT, SUB_ORDERED_CTX_SET_IT >
                                                SUB_ORDERED_CTX_SET_IT_PAIR ;

   public:
      _rtnContextMain( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextMain() ;

   private:
      // disallow copy and assign
      _rtnContextMain( const _rtnContextMain& ) ;
      void operator=( const _rtnContextMain& ) ;

   public:
      _dmsStorageUnit* getSU () { return NULL ; }

      virtual BOOLEAN requireOrder() const = 0 ;

      OSS_INLINE rtnQueryOptions & getQueryOptions ()
      {
         return _options ;
      }

      OSS_INLINE const rtnQueryOptions & getQueryOptions () const
      {
         return _options ;
      }

      OSS_INLINE INT64 getNumToSkip () const
      {
         return _numToSkip ;
      }

      OSS_INLINE INT64 getNumToReturn () const
      {
         return _numToReturn ;
      }

      INT32 reopenForExplain ( INT64 numToSkip, INT64 numToReturn ) ;

   protected:
      virtual INT32     _doAdvance( INT32 type,
                                    INT32 prefixNum,
                                    const BSONObj &keyVal,
                                    const BSONObj &orderby,
                                    const BSONObj &arg,
                                    BOOLEAN isLocate,
                                    _pmdEDUCB *cb ) ;

      virtual INT32     _getAdvanceOrderby( BSONObj &orderby, 
                                            BOOLEAN isRange = FALSE ) const ;

   protected:
      virtual BOOLEAN _requireExplicitSorting () const = 0 ;
      virtual INT32   _prepareAllSubCtxDataByOrder( _pmdEDUCB *cb ) = 0 ;
      virtual INT32   _getNonEmptyNormalSubCtx( _pmdEDUCB *cb, rtnSubContext*& subCtx ) = 0 ;
      virtual INT32   _saveEmptyOrderedSubCtx( rtnSubContext* subCtx ) = 0 ;
      virtual INT32   _saveEmptyNormalSubCtx( rtnSubContext* subCtx ) = 0 ;
      virtual INT32   _saveNonEmptyNormalSubCtx( rtnSubContext* subCtx ) = 0 ;
      virtual INT32   _doAfterPrepareData( _pmdEDUCB *cb ) { return SDB_OK ; }

      virtual INT32   _processCacheData( _pmdEDUCB *cb )
      {
         return SDB_OK ;
      }

      virtual INT32   _prepareSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx ) = 0 ;

      virtual INT32   _doSubCtxsAdvance( LST_SUB_CTX_PTR &lstCtx,
                                         const BSONObj &arg,
                                         _pmdEDUCB *cb ) = 0 ;
      virtual void    _preReleaseSubContext( rtnSubContext *subCtx ) = 0 ;

   protected:
      INT32 _prepareData( _pmdEDUCB *cb ) ;
      INT32 _prepareDataByOrder( _pmdEDUCB *cb );
      INT32 _prepareDataNormal( _pmdEDUCB *cb ) ;
      INT32 _saveNonEmptyOrderedSubCtx( rtnSubContext* subCtx ) ;

      INT32 _processSubContext ( rtnSubContext * subContext,
                                 BOOLEAN & skipData ) ;

      INT32 _checkSubContext ( rtnSubContext * subContext ) ;

      INT32 _checkSubContextAdvance( rtnSubContext *pSubCtx,
                                     ixmIndexKeyGen &keyGen,
                                     INT32 type,
                                     INT32 prefixNum,
                                     const BSONObj &keyVal,
                                     const BSONObj &orderby,
                                     BOOLEAN &processed ) ;

      INT32 _checkOrderCtxsAdvance( INT32 type,
                                    INT32 prefixNum,
                                    const BSONObj &keyVal,
                                    const BSONObj &orderby ) ;

      void _releaseSubContext( rtnSubContext *subCtx )
      {
         if ( NULL != subCtx )
         {
            _preReleaseSubContext( subCtx ) ;
            SDB_OSS_DEL subCtx ;
         }
      }

   protected:
      rtnQueryOptions            _options ;
      SUB_ORDERED_CTX_SET        _orderedContexts ;
      _ixmIndexKeyGen*           _keyGen ;
      INT64                      _numToReturn ;
      INT64                      _numToSkip ;
   } ;

   typedef _rtnContextMain rtnContextMain ;
}

#endif /* RTN_MAIN_CONTEXT_HPP_ */
