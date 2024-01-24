/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-15165:alter修改CS属性
                   seqDB-15166:cs下新增/修改/删除domain
                   
 * @Modify:        wenjing wang Init
 *                 2018-04-27
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

const char *PageSize = "PageSize" ;
const char *Domain = "Domain" ;
const char *LobPageSize = "LobPageSize" ;
const char *Type = "Type" ;
const char *Name = "Name" ;
const char *Groups = "Groups" ;
const char *AutoSplit = "AutoSplit" ;

class alterCSTest : public testBase
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;
      csName = "alterCS_15166" ;
      domainName = "alterCS_15166" ;
      rc = db.getCollectionSpace( csName.c_str(), cs );
      if ( rc == -34 )
      {
         rc = db.createCollectionSpace( csName.c_str(), 65536, cs );
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to createCollectionSpace " << csName ;
      
      if ( isStandalone( db ) )
      {
         return ;
      }
      rc = getGroups( db, groups ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getGroups " ;
      
      if ( groups.empty() ) return ;
      sdbDomain domain ;
      rc = db.getDomain( domainName.c_str(), domain ) ;
      if ( rc == -214 )
      {
         rc = db.createDomain( domainName.c_str(), 
               BSON(Groups << BSON_ARRAY( groups[0] ) << AutoSplit << true), domain );
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/get domain " << domainName ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName.c_str() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
         if ( !isStandalone( db ) )
         {
            rc = db.dropDomain( domainName.c_str() ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to dropDomain " << domainName ;
         }
         
      } 
      testBase::TearDown() ;
   }
protected:
   vector<string> groups ;
   std::string csName ;
   std::string domainName ;
   sdbCollectionSpace cs;
   BSONObj getCSAttr() ;
};

BSONObj alterCSTest::getCSAttr() 
{
   sdbReplicaGroup group ;
   sdbReplicaNode node ;
   sdbCollection cl ;
   sdbCursor cursor ;
   BSONObj ret ;
   sdb catadb ;
   INT32 rc = db.getReplicaGroup( 1, group );
   if ( rc != SDB_OK )
   {
      std::cout << "fail to getReplicaGroup(1),rc="<< rc << std::endl;
      goto done ;
   }
   
   rc = group.getMaster( node ) ;
   if ( rc != SDB_OK )
   {
      std::cout <<"fail to getMaster, rc = " << rc << std::endl;
      goto done ;
   }

   rc = node.connect( catadb );
   if ( rc != SDB_OK )
   {
      std::cout << "fail to connect, rc = " << rc << std::endl;
      goto done ;
   }
   
   rc = catadb.getCollection("SYSCAT.SYSCOLLECTIONSPACES", cl );
   if ( rc != SDB_OK )
   {
      std::cout << "fail to getCollection, rc = " << rc<< std::endl;
      goto done ;
   }
   
   rc = cl.query(cursor, BSON(Name<< csName.c_str()));
   if ( rc != SDB_OK )
   {
      std::cout << "fail to query, rc = " << rc<< std::endl;
      goto done ;
   }
   
   cursor.next( ret );
   cursor.close();
done :
   return ret ;
}

TEST_F( alterCSTest, setDomain )
{
   INT32 rc = SDB_OK ;
   
   if ( isStandalone( db ) || groups.empty() )
   {
      return ;
   }
  
   rc = cs.setDomain(BSON(Domain << domainName) ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setDomain " << domainName ;
   
   BSONObj res = getCSAttr();
   BSONElement elem = res.getFieldDotted(Domain);
   ASSERT_EQ( elem.type(), String ) << "fail to setDomain " << domainName ;
   ASSERT_EQ( elem.String(), domainName ) << "fail to setDomain " << domainName ;
   
   rc = cs.removeDomain() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to removeDomain " << domainName ;
   res = getCSAttr();
   elem = res.getFieldDotted(Domain);
   ASSERT_EQ( elem.type(), EOO ) << "fail to setDomain " << csName ;
}

TEST_F( alterCSTest, enableCapped )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   
   rc = cs.enableCapped() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to enableCapped " << csName ;
   
   BSONObj res = getCSAttr();
   BSONElement elem = res.getFieldDotted( Type ) ;
   ASSERT_EQ( elem.type(), NumberInt ) << "fail to enableCapped " ;
   ASSERT_EQ( elem.Int(), 1 ) << "fail to enableCapped " ;
   
   rc = cs.disableCapped() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to disableCapped " << csName ;
   res = getCSAttr();
   elem = res.getFieldDotted( "Type" ) ;
   ASSERT_EQ( elem.type(), NumberInt ) << "fail to enableCapped " ;
   ASSERT_EQ( elem.Int(), 0 ) << "fail to enableCapped " ;
}

TEST_F( alterCSTest, setAttributes )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   BSONObj opt ;
   if ( groups.empty()  )
   {
      opt = BSON( PageSize << 4096 << LobPageSize << 16384) ;
   }
   else
   {
      opt = BSON(Domain << domainName<< PageSize << 4096 << LobPageSize << 16384) ;
      
   }
   rc = cs.setAttributes( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setAttributes " << opt.toString() ;
   
   BSONObj res = getCSAttr();
   
   
   BSONElement elem = res.getFieldDotted(PageSize);
   ASSERT_EQ( elem.type(), NumberInt ) << "fail to setAttributes " ;
   ASSERT_EQ( elem.Int(), 4096 ) << "fail to setAttributes "  ;
   
   elem = res.getFieldDotted(LobPageSize) ;
   ASSERT_EQ( elem.type(), NumberInt ) << "fail to setAttributes " ;
   ASSERT_EQ( elem.Int(), 16384 ) << "fail to setAttributes "  ;
   
   if ( !groups.empty() )
   {
      elem = res.getFieldDotted(Domain);
      ASSERT_EQ( elem.type(), String ) << "fail to setAttributes " << domainName ;
      ASSERT_EQ( elem.String(), domainName ) << "fail to setAttributes " << domainName ;
   }
  
}

TEST_F( alterCSTest, alter )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) )
   {
      return ;
   }
   
   BSONObj opt ;
   if ( groups.empty()  )
   {
      opt = BSON( PageSize << 4096 << LobPageSize << 16384) ;
   }
   else
   {
      opt = BSON(Domain << domainName<< PageSize << 4096 << LobPageSize << 16384) ;
      
   }
   rc = cs.alterCollectionSpace( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to alterCollectionSpace " << opt.toString();
   
   BSONObj res = getCSAttr();
   BSONElement elem = res.getFieldDotted(PageSize);
   ASSERT_EQ( elem.type(), NumberInt ) << "fail to alterCollectionSpace " ;
   ASSERT_EQ( elem.Int(), 4096 ) << "fail to alterCollectionSpace "  ;
   
   elem = res.getFieldDotted(LobPageSize);
   ASSERT_EQ( elem.type(), NumberInt ) << "fail to alterCollectionSpace " ;
   ASSERT_EQ( elem.Int(), 16384 ) << "fail to alterCollectionSpace "  ;
   
  
   if ( !groups.empty() )
   {
      elem = res.getFieldDotted(Domain);
      ASSERT_EQ( elem.type(), String ) << "fail to alterCollectionSpace " << domainName ;
      ASSERT_EQ( elem.String(), domainName ) << "fail to alterCollectionSpace " << domainName ;
   }
   
   
}

