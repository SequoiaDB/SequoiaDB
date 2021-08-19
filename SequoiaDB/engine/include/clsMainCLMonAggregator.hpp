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

   Source File Name = clsMainCLMonAggregator.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/15/2020  LSQ  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef CLS_MAIN_CL_MON_AGGR__HPP__
#define CLS_MAIN_CL_MON_AGGR__HPP__

#include "rtnFetchBase.hpp"
#include "clsCatalogAgent.hpp"
#include "clsShardMgr.hpp"

using namespace bson ;

namespace engine
{
   /*
      _clsMainCLMonInfo define
   */
   class _clsMainCLMonInfo : public utilPooledObject
   {
      public:
         _clsMainCLMonInfo( const clsCatalogSet *mainCLCata,
                            UINT32 localSubCLCount ) ;

         ~_clsMainCLMonInfo() {}

         void append( const monCollection &subCLInfo ) ;

         void get( monCollection &out ) ;

         BOOLEAN isFinished()
         {
            return (_doneSubCLCount == _totalSubCLCount) ? TRUE : FALSE ;
         }

      private:
         CHAR           _name [ DMS_COLLECTION_FULL_NAME_SZ + 1 ] ;
         utilCLUniqueID _clUniqueID ;
         detailedInfo   _detail ;
         UINT32         _totalSubCLCount ;
         UINT32         _doneSubCLCount ;
   } ;
   typedef _clsMainCLMonInfo clsMainCLMonInfo ;



   /*
      _clsMainCLMonAggregator define
   */
   enum _clsShowMainCLMode
   {
      SHOW_MODE_MAIN = 0,
      SHOW_MODE_SUB,
      SHOW_MODE_BOTH
   } ;
   typedef _clsShowMainCLMode clsShowMainCLMode ;

   INT32 clsParseShowMainCLModeHint( const BSONObj &hint, clsShowMainCLMode &mode ) ;



   /*
      _clsMainCLMonAggregator define
   */
   class _clsMainCLMonAggregator : public IRtnMonProcessor
   {
      public:
         _clsMainCLMonAggregator( clsShowMainCLMode mode ) ;

         virtual ~_clsMainCLMonAggregator() ;

         virtual INT32 process( const monCollection &clIn,
                                monCollection &clOut,
                                UINT32 &resultFlag ) ;

         virtual BOOLEAN hasDataInProcess() { return !_infoMap.empty(); }

         virtual INT32 outputDataInProcess( MON_CL_LIST &out ) ;

      private:
         INT32 _getMainCLName( const CHAR *clName, std::string &mainCLName ) ;

         INT32 _createMainCLInfo( const CHAR *mainCLName, _clsMainCLMonInfo **info ) ;

      private:
         typedef ossPoolMap< std::string, _clsMainCLMonInfo* > MainCLInfoMap ;
         typedef std::pair< std::string, _clsMainCLMonInfo* > MainCLInfoPair ;

         MainCLInfoMap           _infoMap ;
         clsShowMainCLMode       _mode ;
         _clsShardMgr           *_pShdMgr ;
         _clsCatalogAgent       *_pCatAgent ;
   } ;
   typedef _clsMainCLMonAggregator clsMainCLMonAggregator ;

}

#endif //CLS_MAIN_CL_MON_AGGR__HPP__
