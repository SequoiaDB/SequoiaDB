/***************************************************
 * @Description : test domain para 
 *                seqDB-22020: create/get/alter/dropDomain参数校验
 * @Modify      : liuxiaoxuan
 *                2020-04-08
***************************************************/

#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

#define  SDB_INVALIDARG         -6
#define  SDB_DOMAIN_NOTEXIST    -214

class domainParaVerify22020 : public testBase
{
protected:
    const CHAR *largerThan128DomName ;
    const CHAR *nulHandlDomName ;
    const CHAR *notExistDomName ;
    const CHAR *altDomName ;

    void SetUp()  
    {
       testBase::SetUp() ;
    }
    void TearDown()
    {
       testBase::TearDown() ;
    }
} ;

TEST_F( domainParaVerify22020, domainPara )
{
   // Run mode is standalone
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc                       = SDB_OK ;
   sdbDomainHandle dom            = 0 ;
   largerThan128DomName       = "Largerthan128aaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" ;
   nulHandlDomName          = "NULLHandle22020" ;
   notExistDomName        = "NoCreateDomainName22020" ;
   altDomName        = "AlterCorrectDomainName22020" ;

   rc = sdbCreateDomain( db, NULL, NULL, &dom ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "Failed to create domain NULL, rc = "
      << rc ;
   // Domain name size is more than 128
   rc = sdbCreateDomain( db, largerThan128DomName, NULL, &dom ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "Failed to create domain large than"
      " 128, rc = " << rc ;
   // Option handle is null
   rc = sdbCreateDomain( db, nulHandlDomName, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "Failed to create NULL domain handle,"
      "rc = " << rc ;
   // Drop domain have no name
   rc = sdbDropDomain( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "Failed to execute dropDomain specify"
      "nothing, rc = " << rc ;
   // Get domain abnormal
   rc = sdbGetDomain( db, NULL, &dom ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "Failed to execute getDomain specify"
      "nothing, rc = " << rc ;
   rc = sdbGetDomain( db, notExistDomName, &dom ) ;
   ASSERT_EQ( SDB_DOMAIN_NOTEXIST, rc ) << "Failed to execute getDomain"
      "NotExist Name, rc = " << rc ;
   // Alter domain
   rc = sdbDropDomain( db, altDomName ) ;
   ASSERT_EQ( SDB_DOMAIN_NOTEXIST, rc ) << "Failed to drop domain for alter, rc = " << rc;
   rc = sdbCreateDomain( db, altDomName, NULL, &dom ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create domain for alter" << rc ;
   rc = sdbAlterDomain( dom, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "Failed to execute alterDomain"
      "specify no name, rc = " << rc ;
   rc = sdbDropDomain( db, altDomName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to drop domain for alter, rc = " << rc ;
}
