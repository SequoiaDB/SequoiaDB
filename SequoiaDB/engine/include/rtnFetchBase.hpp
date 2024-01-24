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

   Source File Name = rtnFetchBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_FETCH_BASE_HPP__
#define RTN_FETCH_BASE_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "pmdEDU.hpp"
#include "ossMemPool.hpp"
#include "monDMS.hpp"
#include "../bson/bson.h"
#include "utilPooledAutoPtr.hpp"

using namespace bson ;

namespace engine
{

   #define DECLARE_FETCH_AUTO_REGISTER() \
   public: \
      static _rtnFetchBase *newThis () ; \


   #define IMPLEMENT_FETCH_AUTO_REGISTER(theClass) \
   _rtnFetchBase *theClass::newThis () \
   { \
      return SDB_OSS_NEW theClass() ;\
   } \
   _rtnFetchAssit theClass##Assit ( theClass::newThis ) ; \


   /*
      RTN_FETCH_TYPE define
   */
   enum RTN_FETCH_TYPE
   {
      RTN_FETCH_TRANS      = 1,        /// transactions
      RTN_FETCH_CONTEXT,               /// context
      RTN_FETCH_SESSION,               /// session
      RTN_FETCH_COLLECTION,            /// collection
      RTN_FETCH_COLLECTIONSPACE,       /// collectionspace
      RTN_FETCH_DATABASE,              /// database
      RTN_FETCH_SYSTEM,                /// system
      RTN_FETCH_STORAGEUNIT,           /// storageunit
      RTN_FETCH_INDEX,                 /// index
      RTN_FETCH_DATABLOCK,             /// datablock
      RTN_FETCH_BACKUP,                /// backup
      RTN_FETCH_ACCESSPLANS,           /// access plans
      RTN_FETCH_HEALTH,                /// node health check
      RTN_FETCH_CONFIGS,               /// config
      RTN_FETCH_SVCTASKS,              /// svc tasks
      RTN_FETCH_VCL_SESSIONINFO,       /// VCL session info
      RTN_FETCH_QUERIES,               /// queries
      RTN_FETCH_LATCHWAITS,            /// latch waits
      RTN_FETCH_LOCKWAITS,             /// lock waits
      RTN_FETCH_INDEXSTATS,            /// index statistics
      RTN_FETCH_TASKS,                 /// tasks
      RTN_FETCH_RECYCLEBIN,            /// recycle bin

      RTN_FETCH_DATASET,               /// fetch from inner data set

      RTN_FETCH_TRANSWAITS,            /// transaction waits
      RTN_FETCH_TRANSDEADLOCK,         /// transaction deadlock 

      RTN_FETCH_MAX
   } ;

   /*
      _IRtnMonProcessor define
   */
   class _IRtnMonProcessor : public utilPooledObject
   {
      public:
         _IRtnMonProcessor() {}
         virtual ~_IRtnMonProcessor() {}

         virtual INT32     pushIn( const BSONObj &obj ) = 0 ;
         virtual INT32     output( BSONObj &obj, BOOLEAN &hasOut ) = 0 ;

         virtual INT32     done( BOOLEAN &hasOut ) = 0 ;
         virtual BOOLEAN   eof() const = 0 ;
   } ;
   typedef _IRtnMonProcessor IRtnMonProcessor ;
   typedef utilSharePtr<IRtnMonProcessor>       IRtnMonProcessorPtr ;

   /*
      _rtnFetchBase define
   */
   class _rtnFetchBase : public utilPooledObject
   {
      public :
         _rtnFetchBase( INT32 sz, RTN_FETCH_TYPE type )
         :_builder( sz ), _hitEnd( TRUE ), _type( type )
         {
         }

         virtual ~_rtnFetchBase()
         {
         }

         virtual INT32           init( pmdEDUCB *cb,
                                       BOOLEAN isCurrent,
                                       BOOLEAN isDetail,
                                       UINT32 addInfoMask,
                                       const BSONObj obj = BSONObj() ) = 0 ;

         virtual const CHAR*     getName() const = 0 ;

      public:
         virtual BOOLEAN   isHitEnd() const { return _hitEnd ; }
         virtual INT32     fetch( BSONObj &obj ) = 0 ;

         RTN_FETCH_TYPE    getType() const { return _type ; }

      public:
         BufBuilder        _builder ;

      protected:
         BOOLEAN           _hitEnd ;
         RTN_FETCH_TYPE    _type ;
   } ;
   typedef _rtnFetchBase rtnFetchBase ;

   typedef _rtnFetchBase* (*FETCH_NEW_FUNC)() ;

   /*
      _rtnFetchAssit define
   */
   class _rtnFetchAssit : public SDBObject
   {
      public:
         _rtnFetchAssit ( FETCH_NEW_FUNC pFunc ) ;
         ~_rtnFetchAssit () ;
   } ;

   /*
      _rtnFetchBuilder define
   */
   class _rtnFetchBuilder : public SDBObject
   {
      friend class _rtnFetchAssit ;

      typedef ossPoolMap< INT32, FETCH_NEW_FUNC >  MAP_FUNCS ;

      public:
         _rtnFetchBuilder () ;
         ~_rtnFetchBuilder () ;
      public:
         _rtnFetchBase  *create ( INT32 type ) ;
         void            release ( _rtnFetchBase *pFetch ) ;

      protected:
         INT32 _register( INT32 type, FETCH_NEW_FUNC pFunc ) ;

      private:
         MAP_FUNCS            _mapFuncs ;

   };

   _rtnFetchBuilder *getRtnFetchBuilder () ;

}

#endif //RTN_FETCH_BASE_HPP__

