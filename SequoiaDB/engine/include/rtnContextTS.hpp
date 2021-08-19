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

   Source File Name = rtnContextTS.hpp

   Descriptive Name = RunTime Text Search Context

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/30/2017  YSD Split from rtnContextData.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXTTS_HPP__
#define RTN_CONTEXTTS_HPP__

#include "rtnContextMainCL.hpp"
#include "rtnResultSetFilter.hpp"
#include "pmdRemoteSession.hpp"

namespace engine
{
   // Context for text search data.
   class _rtnContextTS : public rtnContextMain
   {
      DECLARE_RTN_CTX_AUTO_REGISTER()

      public:
         _rtnContextTS( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextTS() ;

      public:
         virtual const CHAR* name() const ;
         virtual RTN_CONTEXT_TYPE getType() const ;
         virtual _dmsStorageUnit* getSU() ;

         void enableRIDFilter() ;
         INT32 open( const rtnQueryOptions &options, pmdEDUCB *eduCB ) ;

      protected:
         virtual BOOLEAN _requireExplicitSorting () const ;
         virtual INT32   _prepareAllSubCtxDataByOrder( _pmdEDUCB *cb ) ;
         virtual INT32   _getNonEmptyNormalSubCtx( _pmdEDUCB *cb,
                                                   rtnSubContext*& subCtx ) ;
         virtual INT32   _saveEmptyOrderedSubCtx( rtnSubContext* subCtx ) ;
         virtual INT32   _saveEmptyNormalSubCtx( rtnSubContext* subCtx ) ;
         virtual INT32   _saveNonEmptyNormalSubCtx( rtnSubContext* subCtx ) ;
         virtual BOOLEAN requireOrder () const
         {
            return !_options.isOrderByEmpty() ;
         }

      private:
         INT32 _prepareNextSubContext( pmdEDUCB *eduCB,
                                       BOOLEAN getMore = TRUE ) ;
         INT32 _prepareSubCtxData( pmdEDUCB *cb, INT32 maxNumToReturn ) ;
         INT32 _queryRemote( const rtnQueryOptions &options, pmdEDUCB *cb ) ;

      private:
         pmdEDUCB*            _eduCB ;
         rtnQueryOptions      _options ;
         UINT64               _remoteSessionID ;
         rtnSubCLContext      *_subContext ;
         INT64                _remoteCtxID ;
         BOOLEAN              _enableRIDFilter ;
         rtnResultSetFilter   _ridFilter ;
   } ;
   typedef _rtnContextTS rtnContextTS ;
}

#endif /* RTN_CONTEXTTS_HPP__ */
