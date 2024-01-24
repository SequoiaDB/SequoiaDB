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

   Source File Name = utilLightJobBase.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/12/2019  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_LIGHT_JOB_BASE_HPP__
#define UTIL_LIGHT_JOB_BASE_HPP__

#include "sdbInterface.hpp"
#include "utilPooledObject.hpp"
#include "ossPriorityQueue.hpp"

namespace engine
{

   /*
      UTIL_LJOB_PRIORITY define
   */
   #define UTIL_LJOB_PRI_HIGHEST          ( -20 )
   #define UTIL_LJOB_PRI_HIGH             ( -10 )
   #define UTIL_LJOB_PRI_MID              ( 0 )
   #define UTIL_LJOB_PRI_LOW              ( 10 )
   #define UTIL_LJOB_PRI_LOWEST           ( 20 )

   /*
      UTIL_LIGHTJOB_DO_RESULT define
   */
   enum UTIL_LJOB_DO_RESULT
   {
      UTIL_LJOB_DO_FINISH,
      UTIL_LJOB_DO_CONT
   } ;

   #define UTIL_LJOB_MIN_AVG_COST         ( 60 )         /// micro second
   #define UTIL_LJOB_DFT_AVG_COST         ( 60000 )      /// micro second
   #define UTIL_LJOB_MAX_AVG_COST         ( 60000000 )   /// micro second

   /*
      _utilLightJob define
   */
   class _utilLightJob : public _utilPooledObject
   {
      public:
         _utilLightJob() { _jobID = 0 ; }
         virtual ~_utilLightJob() {}

         INT32       submit( BOOLEAN takeOver = TRUE,
                             INT32 priority = UTIL_LJOB_PRI_MID,
                             UINT64 expectAvgCost = UTIL_LJOB_DFT_AVG_COST,
                             UINT64 *pJobID = NULL ) ;

         UINT64      getJobID() const { return _jobID ; }

      public:
         virtual const CHAR*     name() const = 0 ;
         virtual INT32           doit( IExecutor *pExe,
                                       UTIL_LJOB_DO_RESULT &result,
                                       UINT64 &sleepTime ) = 0 ;

      private:
         UINT64      _jobID ;

   } ;
   typedef _utilLightJob utilLightJob ;

   /*
      _utilLightJobInfo define
   */
   class _utilLightJobInfo
   {
      public:
         _utilLightJobInfo() ;
         _utilLightJobInfo( utilLightJob *pJob,
                            BOOLEAN takeOver = TRUE,
                            INT32 priority = UTIL_LJOB_PRI_MID,
                            UINT64 expectAvgCost = UTIL_LJOB_DFT_AVG_COST ) ;
         ~_utilLightJobInfo() ;

         void     reset() ;

         bool     operator< ( const _utilLightJobInfo &right ) const ;

         void     upPriority() ;
         void     downPriority() ;
         void     restorePriority() ;

         INT32    doit( IExecutor *pExe, UTIL_LJOB_DO_RESULT &result ) ;
         void     release() ;

         UINT64   lastDoTime() const { return _lastDoTime ; }
         UINT64   lastCost() const { return _lastCost ; }
         UINT64   totalCost() const { return _totalCost ; }
         UINT64   totalTimes() const { return _totalTimes ; }

         UINT64   expectSleepTime() const { return _sleepTime ; }

         FLOAT64  avgCost() const ;
         UINT64   expectAvgCost() const { return _expectAvgCost ; }

         void     resetStat() ;

         utilLightJob*  getJob() { return _pJob ; }

      protected:
         INT32    adjustPriority( INT32 priority ) ;
         UINT64   adjustAvgCost( UINT64 avgCost ) ;

      private:
         utilLightJob   *_pJob ;
         INT32          _orgPriority ;
         INT32          _priority ;
         UINT64         _expectAvgCost  ;
         BOOLEAN        _takeOver ;

         UINT64         _lastDoTime ;        // micro second
         UINT64         _lastCost ;          // micro second
         UINT64         _totalCost ;         // micro second
         UINT64         _totalTimes ;

         UINT64         _sleepTime ;
   } ;
   typedef _utilLightJobInfo utilLightJobInfo ;

   /*
      _utilLightJobMgr define
   */
   class _utilLightJobMgr : public SDBObject
   {
      public:
         _utilLightJobMgr() ;
         virtual ~_utilLightJobMgr() ;

         void           fini( IExecutor *pExe ) ;

      public:
         virtual void   push( utilLightJob *pJob,
                              BOOLEAN takeOver = TRUE,
                              INT32 priority = UTIL_LJOB_PRI_MID,
                              UINT64 expectAvgCost = UTIL_LJOB_DFT_AVG_COST ) ;

         virtual void   push( const utilLightJobInfo &job ) ;

      public:

         UINT64         size() ;
         BOOLEAN        isEmpty() ;

         BOOLEAN        pop( utilLightJobInfo &job, INT64 millisec ) ;
         UINT64         allocID() ;

      protected:
         virtual void   _onFini() {}

      private:
         ossPriorityQueue<utilLightJobInfo>     _queue ;
         ossAtomic64                            _id ;

   } ;
   typedef _utilLightJobMgr utilLightJobMgr ;

   /*
      Global functions
   */
   utilLightJobMgr* utilGetGlobalJobMgr() ;
   void utilSetGlobalJobMgr( utilLightJobMgr *pJobMgr ) ;

}

#endif //UTIL_LIGHT_JOB_BASE_HPP__

