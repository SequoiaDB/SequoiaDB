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

   Source File Name = coordCommandSequence.hpp

   Descriptive Name = Coordinator Sequence Command

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_COMMAND_SEQUENCE_HPP_
#define COORD_COMMAND_SEQUENCE_HPP_

#include "coordCommandCommon.hpp"
#include "coordFactory.hpp"
#include "rtnCommand.hpp"
#include <string>

namespace engine
{
   // This command is executed in _pmdCoordProcessor::_onQueryReqMsg for request from client
   class _coordCMDInvalidateSequenceCache : public _coordCmdWithLocation
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDInvalidateSequenceCache() ;
         virtual ~_coordCMDInvalidateSequenceCache() ;
      private:
         virtual BOOLEAN _useContext() { return FALSE ; }
         virtual INT32   _onLocalMode( INT32 flag ) { return SDB_OK ; }
         virtual void    _preSet( pmdEDUCB *cb, coordCtrlParam &ctrlParam ) ;
         virtual UINT32  _getControlMask() const ;
   } ;

   typedef class _coordCMDInvalidateSequenceCache coordCMDInvalidateSequenceCache ;

   // This command is executed in _CoordCB::_processQueryMsg for each coord node
   class _coordInvalidateSequenceCache : public _rtnCommand
   {
      DECLARE_CMD_AUTO_REGISTER()
   public:
      _coordInvalidateSequenceCache() ;
      virtual ~_coordInvalidateSequenceCache() ;

   public:
      virtual const CHAR *name() { return NAME_INVALIDATE_SEQUENCE_CACHE ; }
      virtual RTN_COMMAND_TYPE type() { return CMD_INVALIDATE_SEQUENCE_CACHE ; }
      virtual INT32 spaceNode () ;
      virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                           const CHAR *pMatcherBuff,
                           const CHAR *pSelectBuff,
                           const CHAR *pOrderByBuff,
                           const CHAR *pHintBuff ) ;
      virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                           _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                           INT16 w = 1, INT64 *pContextID = NULL ) ;

   private:
      bson::BSONObj  _object ;
      const CHAR *   _collection ;
      const CHAR *   _fieldName ;
      const CHAR *   _sequenceName ;
      utilSequenceID _sequenceID ;
   } ;
}

#endif /* COORD_COMMAND_SEQUENCE_HPP_ */

