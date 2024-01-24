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

   Source File Name = coordSequenceAgent.hpp

   Descriptive Name = Coordinator Sequence Agent

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_SEQUENCE_AGENT_HPP_
#define COORD_SEQUENCE_AGENT_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "utilGlobalID.hpp"
#include "msg.h"
#include "utilConcurrentMap.hpp"
#include "../bson/bsonobj.h"
#include <string>

namespace engine
{
   class _coordResource ;
   class _coordSequence ;
   class _pmdEDUCB ;

   class _coordSequenceAgent: public SDBObject
   {
   private:
      typedef utilConcurrentMap<std::string, _coordSequence*, 64> COORD_SEQ_MAP ;

   public:
      _coordSequenceAgent() ;
      ~_coordSequenceAgent() ;

      INT32 init( _coordResource* resource ) ;
      void fini() ;

   public:
      INT32 getNextValue( const std::string& name, const utilSequenceID ID,
                          INT64& nextValue, _pmdEDUCB* eduCB ) ;
      INT32 adjustNextValue( const std::string& name, const utilSequenceID ID,
                             INT64 expectValue, _pmdEDUCB* eduCB ) ;
      INT32 setCurrentValue( const std::string& name, const utilSequenceID ID,
                             INT64 expectValue, _pmdEDUCB* eduCB,
                             utilSequenceID *pCachedID ) ;
      INT32 getCurrentValue( const std::string& name, const utilSequenceID ID,
                             INT64& currentValue, _pmdEDUCB* eduCB ) ;
      INT32 fetch( const std::string& name, const utilSequenceID ID,
                   const INT32 fetchNum, INT64& nextValue, INT32& returnNum,
                   INT32& increment, _pmdEDUCB* eduCB ) ;
      BOOLEAN removeCache( const std::string& sequenceName, utilSequenceID ID,
                           const INT64* pCurrentValue = NULL ) ;
      void clear() ;

   private:
      class _operateSequence ;
      class _getNextValue ;
      class _adjustNextValue ;
      class _getCurrentValue ;
      class _fetchValue ;
      friend class _getNextValue ;
      friend class _adjustNextValue ;
      friend class _getCurrentValue ;
      friend class _fetchValue ;

      INT32 _doOnSequence( const std::string& name,
                           const utilSequenceID ID,
                           _pmdEDUCB *eduCB,
                           _operateSequence& func ) ;
      INT32 _doOnSequenceByXLock( const std::string& name,
                                  const utilSequenceID ID,
                                  _pmdEDUCB *eduCB,
                                  _operateSequence& func ) ;
      INT32 _doOnSequenceBySLock( const std::string& name,
                                  const utilSequenceID ID,
                                  _pmdEDUCB *eduCB,
                                  _operateSequence& func,
                                  BOOLEAN& noCache,
                                  utilSequenceID* cachedSeqID ) ;
      INT32 _acquireSequence( _coordSequence& seq,
                              _pmdEDUCB* eduCB,
                              BOOLEAN hasExpectValue = FALSE,
                              INT64 expectValue = 0 ) ;
      INT32 _processAcquireReply( MsgHeader* msg, _coordSequence& seq ) ;
      BOOLEAN _removeCacheByID( const std::string& sequenceName,
                                const utilSequenceID ID ) ;
      BOOLEAN _isInnerRC( INT32 rc ) ;

   private:
      _coordResource*   _resource ;
      COORD_SEQ_MAP     _sequenceCache ;
   } ;
   typedef _coordSequenceAgent coordSequenceAgent ;

   INT32 coordSequenceInvalidateCache ( const CHAR * sequenceName,
                                        utilSequenceID ID,
                                        _pmdEDUCB * eduCB,
                                        const INT64 * pCurrentValue = NULL ) ;

   INT32 coordSequenceInvalidateCache ( const CHAR * collection,
                                        const CHAR * field,
                                        _pmdEDUCB * eduCB ) ;
}

#endif /* COORD_SEQUENCE_AGENT_HPP_ */

