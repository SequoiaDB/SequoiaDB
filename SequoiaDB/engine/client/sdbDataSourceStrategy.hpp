/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY ; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = sdbDataSourceStrategy.hpp

   Descriptive Name = SDB Data Source Strategy Include Header

   When/how to use: this program may be used on sequoiadb data source function.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
         06/30/2016   LXJ Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SDB_DATA_SOURCE_STRATEGY_HPP_
#define SDB_DATA_SOURCE_STRATEGY_HPP_

#include "ossTypes.h"
#include <vector>
#include <map>
#include <set>
#include "ossLatch.hpp"
#include <string>
#include "client.hpp"

using std::string;
using std::vector;
using std::map;
using std::set;

namespace sdbclient
{
   enum SYNC_CHOICE
   {
      DELIDLECONN,
      DELBUSYCONN,
      ADDBUSYCONN,
      ADDIDLECONN
   } ;

   class sdbDataSourceStrategy  : public SDBObject
   {
   private:
      sdbDataSourceStrategy( const sdbDataSourceStrategy &strategy ) ;
      sdbDataSourceStrategy& operator=( const sdbDataSourceStrategy &strategy ) ;
      
   protected:
      vector<string> _abnormalCoordList ;
      vector<string> _normalCoordList ;
      ossSpinXLatch _coordMutex ;
      INT32 _abPos ;

   public:
      sdbDataSourceStrategy()
         :_abPos(0) {}
      virtual ~sdbDataSourceStrategy() {}

   public:
      
      virtual void addCoord( const string &coord ) ;

      virtual void removeCoord( const string &coord ) ;
      
      virtual INT32 getNormalCoordNum() ;

      virtual INT32 getAbnormalCoordNum() ;

      virtual INT32 getLocalCoordNum() ;

      virtual INT32 getNextCoord( string& nCoord ) = 0 ;

      virtual INT32 getNextAbnormalCoord( string& nCoord ) ;

      virtual void mvCoordToAbnormal( const string &coord ) ;

      virtual void mvCoordToNormal( const string &coord ) ;

      virtual void sync( sdb *conn, SYNC_CHOICE choice ) {}

      virtual void syncAddNewConn( sdb *conn, const string &coord ) {}

   protected:
      BOOLEAN _converToIP( const string &oldcoord, string& newcoord ) ;
   private:
      BOOLEAN _isLocalIP(const string &ipstr) ;
   } ;


   class sdbDSSerialStrategy : public sdbDataSourceStrategy
   {
   private:
      sdbDSSerialStrategy( const sdbDSSerialStrategy &strategy ) ;
      sdbDSSerialStrategy& operator=( const sdbDSSerialStrategy &strategy ) ;

   public:
      sdbDSSerialStrategy()
         : _curPos(0) {}
      virtual ~sdbDSSerialStrategy(){}

   public:
      virtual INT32 getNextCoord( string& nCoord ) ;

   private:
      INT32 _curPos ;
   } ;


   class sdbDSRandomStrategy : public sdbDataSourceStrategy
   {
   private:
      sdbDSRandomStrategy( const sdbDSRandomStrategy &strategy ) ;
      sdbDSRandomStrategy& operator=( const sdbDSRandomStrategy &strategy ) ;

   public:
      sdbDSRandomStrategy() 
      {
         srand((unsigned)time( NULL )) ;
      }
      virtual ~sdbDSRandomStrategy(){}

   public:
      virtual INT32 getNextCoord( string& nCoord ) ;
   } ;

   class sdbDSLocalStrategy : public sdbDataSourceStrategy
   {
   private:
      sdbDSLocalStrategy( const sdbDSLocalStrategy &strategy ) ;
      sdbDSLocalStrategy& operator=( const sdbDSLocalStrategy &strategy ) ;

   public:
      sdbDSLocalStrategy()
         : _localPos(0),
         _normalPos(0) {}
      virtual ~sdbDSLocalStrategy(){}

   public:

      virtual INT32 getLocalCoordNum() ;

      virtual void addCoord( const string &coord ) ;

      virtual void removeCoord( const string &coord ) ;

      virtual INT32 getNextCoord( string& nCoord ) ;

      virtual void mvCoordToAbnormal( const string &coord ) ;

      virtual void mvCoordToNormal( const string &coord ) ;

   private:
      BOOLEAN _isLocalCoord( const string &coord ) ;

   private:
      vector<string> _localCoordList ;

   private:
      INT32 _localPos ;
      INT32 _normalPos ;
   } ;

   struct coordInfo : public SDBObject
   {
      INT32 usedNum ;
      INT32 totalNum ;
      BOOLEAN bAvailable ;
      string coord ;
      coordInfo( const string &c )
         :usedNum(0),
         totalNum(0),
         bAvailable(TRUE),
         coord(c) {}
   } ;

   typedef struct coordInfo coordInfo ;

   struct coordInfoCmp
   {
      bool operator()( const coordInfo *left, const coordInfo *right )
      {
         if ( left->bAvailable != right->bAvailable )
            return left->bAvailable > right->bAvailable ;
         if ( left->totalNum != right->totalNum )
            return left->totalNum < right->totalNum ;
         if ( left->usedNum != right->usedNum )
            return left->usedNum < right->usedNum ;
         INT32 res = ( left->coord ).compare( right->coord ) ;
         if ( 0 != res )
         {
            if ( res < 0 )
               return true ;
            else
               return false ;
         }
         return false ;
      }
   } ;

   class sdbDSBalanceStrategy : public sdbDataSourceStrategy
   {
   private:
      sdbDSBalanceStrategy( const sdbDSBalanceStrategy &strategy ) ;
      sdbDSBalanceStrategy& operator=( const sdbDSBalanceStrategy &strategy ) ;

   public:
      sdbDSBalanceStrategy() {}
      virtual ~sdbDSBalanceStrategy() ;

   public:
      virtual void addCoord( const string &coord ) ;

      virtual void removeCoord( const string &coord ) ;
      
      virtual INT32 getNormalCoordNum() ;

      virtual INT32 getAbnormalCoordNum() ;

      virtual INT32 getNextCoord( string& nCoord ) ;

      virtual INT32 getNextAbnormalCoord( string& nCoord ) ;

      virtual void mvCoordToAbnormal( const string &coord ) ;

      virtual void mvCoordToNormal( const string &coord ) ;

      virtual void sync( sdb *conn, SYNC_CHOICE choice ) ;

      virtual void syncAddNewConn( sdb *conn, const string &coord ) ;

   private:
      set<coordInfo*, coordInfoCmp>::const_iterator 
         _findCoord( const string &coord ) const  ;

      INT32 _getCoordNum( BOOLEAN flag ) ;

      void _syncDelIdleConn( sdb *conn ) ;

      void _syncDelBusyConn( sdb *conn ) ;

      void _syncAddBusyConn( sdb *conn ) ;

      void _syncAddIdleConn( sdb *conn ) ;

   private:
      set< coordInfo*, coordInfoCmp > _coordInfoSet ;
      map< sdb*, coordInfo* > _connToCoord ;
   } ;
}

#endif
