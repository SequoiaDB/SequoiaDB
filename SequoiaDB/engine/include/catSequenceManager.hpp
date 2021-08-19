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

   Source File Name = catSequenceManager.hpp

   Descriptive Name = Sequence manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/19/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_SEQUENCE_MANAGER_HPP_
#define CAT_SEQUENCE_MANAGER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "catSequence.hpp"
#include "pmdEDU.hpp"
#include "../bson/bsonobj.h"
#include "utilConcurrentMap.hpp"
#include <string>

namespace engine
{
   struct _catSequenceAcquirer: public SDBObject
   {
      utilSequenceID ID ;
      INT64 nextValue ;
      INT32 acquireSize ;
      INT32 increment ;

      _catSequenceAcquirer()
      {
         ID = UTIL_SEQUENCEID_NULL ;
         nextValue = 0 ;
         acquireSize = 0 ;
         increment = 0 ;
      }
   } ;
   typedef _catSequenceAcquirer catSequenceAcquirer ;

   class _catSequenceManager: public SDBObject
   {
   private:
      typedef utilConcurrentMap<std::string, _catSequence*, 64> CAT_SEQ_MAP ;
      // disallow copy and assign
      _catSequenceManager( const _catSequenceManager& ) ;
      void operator=( const _catSequenceManager& ) ;

   public:
      _catSequenceManager() ;
      ~_catSequenceManager() ;

      INT32 active() ;
      INT32 deactive() ;

      INT32 createSequence( const std::string& name, const bson::BSONObj& options,
                            _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 insertSequence ( const std::string & name,
                             bson::BSONObj & options, _pmdEDUCB * eduCB,
                             INT16 w ) ;
      INT32 dropSequence( const std::string& name, _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 alterSequence( const std::string& name,
                           const bson::BSONObj& options,
                           _pmdEDUCB* eduCB,
                           INT16 w,
                           bson::BSONObj * oldOptions,
                           UINT32 * alterMask ) ;
      INT32 acquireSequence( const std::string& name,
                             const utilSequenceID ID,
                             _catSequenceAcquirer& acquirer,
                             _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 resetSequence( const std::string& name, _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 adjustSequence( const std::string &name,
                            const utilSequenceID ID,
                            INT64 expectValue,
                            _pmdEDUCB *eduCB, INT16 w ) ;
      OSS_INLINE INT32 getSequence( const std::string& name,
                                    bson::BSONObj& sequence,
                                    _pmdEDUCB* eduCB )
      {
         return _findSequence( name, sequence, eduCB ) ;
      }

   private:
      class _operateSequence ;
      class _acquireSequence ;
      class _adjustSequence ;
      friend class _acquireSequence ;
      friend class _adjustSequence ;

      INT32 _insertSequence( bson::BSONObj& sequence, _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 _deleteSequence( const std::string& name, _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 _updateSequence( const std::string& name, const bson::BSONObj& options,
                             _pmdEDUCB* eduCB, INT16 w ) ;
      INT32 _findSequence( const std::string& name, bson::BSONObj& sequence,
                           _pmdEDUCB* eduCB ) ;
      INT32 _doOnSequence( const std::string& name,
                           const utilSequenceID ID,
                           _pmdEDUCB* eduCB,
                           INT16 w,
                           _operateSequence& func ) ;
      INT32 _doOnSequenceBySLock( const std::string& name,
                                  const utilSequenceID ID,
                                  _pmdEDUCB* eduCB,
                                  INT16 w,
                                  _operateSequence& func,
                                  BOOLEAN& noCache,
                                  utilSequenceID &cachedSeqID ) ;
      INT32 _doOnSequenceByXLock( const std::string& name,
                                  const utilSequenceID ID,
                                  _pmdEDUCB* eduCB,
                                  INT16 w,
                                  _operateSequence& func ) ;
      BOOLEAN _removeCacheByID( const std::string& name, utilSequenceID ID ) ;
      void  _cleanCache( BOOLEAN needFlush ) ;

   private:
      CAT_SEQ_MAP _sequenceCache ;
   } ;
   typedef _catSequenceManager catSequenceManager ;
}

#endif /* CAT_SEQUENCE_MANAGER_HPP_ */

