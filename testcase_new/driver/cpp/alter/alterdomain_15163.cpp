/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-15163:domain下新增/修改/删除组
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

const char *AutoSplit = "AutoSplit" ;
const char *Groups = "Groups" ;
const char *Name = "Name" ;
const char *GroupName = "GroupName" ;

class alterDomainTest : public testBase
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;
      if ( isStandalone( db ) )
      {
         return ;
      }
      
      domainName = "alterdomain_15163" ;
      rc = getGroups( db, groupNames ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getGroups " ;
      rc = db.getDomain( domainName.c_str(), domain );
      if ( rc == -214 )
      {
         BSONObj opt = BSON( Groups<< BSON_ARRAY(groupNames[0].c_str()) << "AutoSplit" << false ) ;
         rc = db.createDomain( domainName.c_str(), opt, domain ) ;
         std::cout << opt.toString() << std::endl;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to get/createDomain " << domainName ;
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( !isStandalone( db ) && shouldClear() )
      {
         rc = db.dropDomain( domainName.c_str() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to dropDomain " << domainName ; 
      } 
      testBase::TearDown() ;
   }
protected:
   std::string domainName ;
   std::vector<std::string> groupNames ;
   sdbDomain domain ;
   
   BSONObj getDomainAttr();
} ;

BSONObj alterDomainTest::getDomainAttr() 
{
   sdbCursor cursor ;
   INT32 rc = SDB_OK ;
   BSONObj ret ;
   BSONObj cond = BSON( Name <<  domainName ) ;
   
   rc = db.listDomains( cursor, cond ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "listDomains(" << cond.toString() <<  ")" << std::endl ;
   } 
   while ( SDB_DMS_EOC != cursor.next( ret ) ) ;
   
   return ret ;
}

BOOLEAN isContainObj(BSONObj l, BSONObj r )
{
   BOOLEAN ret = FALSE ;
   BSONObjIterator iter( r ) ;
   while ( iter.more() )
   {
      BSONElement beField = iter.next() ;
      const CHAR *pFieldName = beField.fieldName() ;
      BSONElement elem = l.getFieldDotted( pFieldName );
      if ( elem.eoo() )
      {
          ret = FALSE ;
          break ;
      }
      else if ( beField.type() == elem.type() )
      {
         if ( beField.type() != Object && beField.type() != Array )
         {
            ret = beField == elem ;
         }
         else 
         {
            if ( !strcmp( pFieldName, Groups ))
            {
               BSONObjIterator lit( elem.embeddedObject() ) ;
               BSONObjIterator rit( beField.embeddedObject() ) ;
               while ( lit.more() && rit.more() )
               {
                  BSONElement ltmpE = lit.next() ;
                  BSONElement rtmpE = rit.next() ;
                  
                  if ( rtmpE.type() == String && ltmpE.type() == Object )
                  {
                     BSONElement lsubE = ltmpE.Obj().getFieldDotted("GroupName") ;
                     if ( lsubE.type() == String )
                     {
                        std::cout << lsubE.String() << "== " << rtmpE.String() << std::endl;
                        ret = lsubE.String() == rtmpE.String() ;
                     }
                     else
                     {
                        ret = FALSE ;
                     }
                  }
                  else
                  {
                     ret = FALSE ;
                     break ;
                  }
               }
            }
         }
         
         if ( !ret ) break ;
      }
      else
      {
         ret = FALSE ;
         break ;
      }
   }
   return ret ;
}

TEST_F( alterDomainTest, addGroups )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;
   }
   BSONObj opt = BSON( Groups << BSON_ARRAY(groupNames[0]) ) ;
   rc = domain.addGroups( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to addGroups " << opt.toString() ;
   BSONObj attr = getDomainAttr() ;
   BOOLEAN ret = isContainObj( attr, opt ) ;
   ASSERT_EQ( TRUE, ret ) << "fail to addGroups " << opt.toString() ;
   
   rc = domain.removeGroups( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to removeGroups " << opt.toString() ;
   attr = getDomainAttr() ;
   ret = isContainObj( attr, opt ) ;
   ASSERT_EQ( FALSE, rc ) << "fail to removeGroups " << opt.toString() ;
   
}

TEST_F( alterDomainTest, setGroups )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;
   }
   
   BSONObj opt = BSON( Groups << BSON_ARRAY(groupNames[0]) ) ;
   rc = domain.setGroups( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setGroups " << opt.toString() ;
   
   BSONObj attr = getDomainAttr() ;
   BOOLEAN ret = isContainObj( attr, opt ) ;
   ASSERT_EQ( TRUE, ret ) << "fail to setGroups " << opt.toString() ;
}

TEST_F( alterDomainTest, setAttributes )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;
   }
   
   BSONObj opt = BSON(  AutoSplit << false << Groups << BSON_ARRAY(groupNames[0]) ) ;
   rc = domain.setAttributes( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setAttributes " << opt.toString() ;
   
   BSONObj attr = getDomainAttr() ;
   BOOLEAN ret = isContainObj( attr, opt ) ;
   ASSERT_EQ( TRUE, ret ) << "fail to setAttributes " << opt.toString() ;
}

TEST_F( alterDomainTest, alterDomain )
{
   INT32 rc = SDB_OK ;
   if ( isStandalone( db ) || groupNames.empty() )
   {
      return ;
   }
   
   BSONObj opt = BSON(  AutoSplit << false << Groups << BSON_ARRAY(groupNames[0]) ) ;
   rc = domain.alterDomain( opt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to alterDomain " << opt.toString() ;
   BSONObj attr = getDomainAttr() ;
   BOOLEAN ret = isContainObj( attr, opt ) ;
   ASSERT_EQ( TRUE, ret ) << "fail to alterDomain " << opt.toString() ;
}
