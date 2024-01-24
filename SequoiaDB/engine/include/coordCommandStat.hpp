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

   Source File Name = coordCommandStat.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2017  XJH Init
   Last Changed =

*******************************************************************************/

#ifndef COORD_COMMAND_STAT_HPP__
#define COORD_COMMAND_STAT_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDStatisticsBase define
   */
   class _coordCMDStatisticsBase : public _coordCommandBase
   {
      public:
         _coordCMDStatisticsBase() ;
         virtual ~_coordCMDStatisticsBase() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) = 0 ;

         virtual INT32 generateVCLResult( const CHAR *pCLName,
                                          rtnContext *pContext,
                                          pmdEDUCB *cb ) ;

         virtual BOOLEAN openEmptyContext() const { return FALSE ; }

      private:
         INT32 _executeOnVCL( const CHAR *pCLName,
                                pmdEDUCB *cb,
                                INT64 &contextID ) ;

   } ;
   typedef _coordCMDStatisticsBase coordCMDStatisticsBase ;

   /*
      _coordCMDGetIndexes define, used by versions before v3.4.5
   */
   class _coordCMDGetIndexesOldVersion : public _coordCMDStatisticsBase
   {
      typedef ossPoolMap< string, BSONObj>      CoordIndexMap ;

      public:
         _coordCMDGetIndexesOldVersion() ;
         virtual ~_coordCMDGetIndexesOldVersion() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetIndexesOldVersion coordCMDGetIndexesOldVersion ;

   /*
      _coordCMDGetCount define
   */
   class _coordCMDGetCount : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCount() ;
         virtual ~_coordCMDGetCount() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
         virtual INT32 generateVCLResult( const CHAR *pCLName,
                                          rtnContext *pContext,
                                          pmdEDUCB *cb ) ;
         virtual BOOLEAN openEmptyContext() const { return TRUE ; }
   } ;
   typedef _coordCMDGetCount coordCMDGetCount ;

   /*
      _coordCMDGetDatablocks define
   */
   class _coordCMDGetDatablocks : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetDatablocks() ;
         virtual ~_coordCMDGetDatablocks() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetDatablocks coordCMDGetDatablocks ;

   /*
      _coordCMDGetQueryMeta define
   */
   class _coordCMDGetQueryMeta : public _coordCMDGetDatablocks
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetQueryMeta() ;
         virtual ~_coordCMDGetQueryMeta() ;
   } ;
   typedef _coordCMDGetQueryMeta coordCMDGetQueryMeta ;

   /*
      _coordCMDGetCollectionDetail define
   */
   class _coordCMDGetCollectionDetail : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCollectionDetail() ;
         virtual ~_coordCMDGetCollectionDetail() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetCollectionDetail coordCMDGetCollectionDetail ;

   /*
      _coordCMDGetCLStat define
   */
   class _coordCMDGetCLStat : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCLStat() ;
         virtual ~_coordCMDGetCLStat() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext, pmdEDUCB *cb ) ;
      
      private:
         void _merge( const collectionStatInfo &from, collectionStatInfo &to ) ;
   } ;
   typedef _coordCMDGetCLStat coordCMDGetCLStat ;

   /*
      _coordCMDGetIndexStat define
   */
   class _coordCMDGetIndexStat : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetIndexStat() ;
         virtual ~_coordCMDGetIndexStat() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
         INT32 _getResultCount( UINT32 &resCount, pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetIndexStat coordCMDGetIndexStat ;

}

#endif // COORD_COMMAND_STAT_HPP__
