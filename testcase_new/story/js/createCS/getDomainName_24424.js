/******************************************************************************
 * @Description   : seqDB-24424:创建Domain并使用CS.getDomainName()
 * @Author        : liuli
 * @CreateTime    : 2021.10.14
 * @LastEditTime  : 2021.11.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_24424";
   var domainName1 = "domain_24424_1";
   var domainName2 = "domain_24424_2";

   commDropCS( db, csName );
   commDropDomain( db, domainName1 );
   commDropDomain( db, domainName2 );
   var groupNames = commGetDataGroupNames( db );

   db.createDomain( domainName1, [groupNames[1]] );
   db.createDomain( domainName2, [groupNames[1]] );

   var dbcs = db.createCS( csName, { Domain: domainName1 } );

   var domainName = dbcs.getDomainName();
   assert.equal( domainName, domainName1 );
   assert.equal( typeof ( domainName ), "string" );

   dbcs.setDomain( { Domain: domainName2 } );
   var domainName = dbcs.getDomainName();
   assert.equal( domainName, domainName2 );

   dbcs.removeDomain();
   var domainName = dbcs.getDomainName();
   assert.equal( domainName, "" );

   commDropCS( db, csName );
   commDropDomain( db, domainName1 );
   commDropDomain( db, domainName2 );
}