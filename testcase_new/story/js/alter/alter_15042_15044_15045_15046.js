/* *****************************************************************************
@discretion: alter cs, the cs exist cl, the test scenario is as follows:
test 15042: only test setDomain to add Domain, cl autosplit does not automate.
test 15044: alter domain, new domain contains the group of cl
test 15045: alter domain, new domain contains the group of cl
test 15046: remove domain
@author��2018-4-27 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15042";
var clName = CHANGEDPREFIX + "_cs15042";
var domainName1 = CHANGEDPREFIX + "_domain15042";
var domainName2 = CHANGEDPREFIX + "_domain15044";
var domainName3 = CHANGEDPREFIX + "_domain15045";


main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var groupNames = getGroupName( db );
   if( 1 === groupNames.length )
   {
      return;
   }

   //clean environment before test
   commDropCS( db, csName, true, "drop cs" );
   commDropDomain( db, domainName1 );
   commDropDomain( db, domainName2 );
   commDropDomain( db, domainName3 );

   //create domain, cs, cl
   commCreateDomain( db, domainName1, [groupNames[0]] );
   commCreateDomain( db, domainName2, groupNames );
   commCreateDomain( db, domainName3, [groupNames[1]] );
   var dbcs = commCreateCS( db, csName, false, "create CS" );
   var dbcl = commCreateCL( db, csName, clName, { Group: groupNames[0] } );

   //testcase15042:add domain to cs
   dbcs.setDomain( { Domain: domainName1 } );
   checkAlterCSResult( csName, "Domain", domainName1 );


   //testcase15044:alter domain, new domain contains the group of cl
   dbcs.setDomain( { Domain: domainName2 } );
   checkAlterCSResult( csName, "Domain", domainName2 );

   //testcase15045:alter domain, new domain contains the group of cl
   assert.tryThrow( SDB_CAT_GROUP_NOT_IN_DOMAIN, function()
   {
      dbcs.setDomain( { Domain: domainName3 } );
   } );
   checkAlterCSResult( csName, "Domain", domainName2 );


   //testcase15046:remove domain
   dbcs.removeDomain();
   checkRemoveDomainResult( csName );

   //clean
   commDropCS( db, csName, true, "clear cs" );
   commDropDomain( db, domainName1 );
   commDropDomain( db, domainName2 );
   commDropDomain( db, domainName3 );
}

function checkRemoveDomainResult ( csName )
{
   var rg = db.getRG( "SYSCatalogGroup" );
   var dbca = new Sdb( rg.getMaster() );
   var cur = dbca.SYSCAT.SYSCOLLECTIONSPACES.find( { "Name": csName } );
   while( cur.next() )
   {
      var tempinfo = cur.current().toObj();
   }
   var csInfo = JSON.stringify( tempinfo );

   //domain no exists return -1
   assert.equal( -1, csInfo.search( "Domain" ) );
}