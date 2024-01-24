/* *****************************************************************************
@discretion: alter cs name/pageSize/lobpagesize/domain
@author��2018-4-28 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15049";
var domainName = CHANGEDPREFIX + "_domain15049";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var groupNames = getGroupName( db );

   //clean environment before test
   commDropCS( db, csName, true, "drop cs" );
   commDropDomain( db, domainName );

   //create domain, cs, cl
   commCreateDomain( db, domainName, [groupNames[0]] );
   var dbcs = commCreateCS( db, csName, false, "create CS" );

   //alter pageSize /domain/lobpageSize
   var pageSize = 16384;
   var lobpageSize = 131072;
   dbcs.alter( { PageSize: pageSize, LobPageSize: lobpageSize, Domain: domainName } );
   checkAlterCSResult( csName, "PageSize", pageSize );
   checkAlterCSResult( csName, "Domain", domainName );
   checkAlterCSResult( csName, "LobPageSize", lobpageSize );

   //alter cs name
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcs.alter( { Name: "testcs" } );
   } );
   checkAlterCSResult( csName, "Name", csName );

   //clean
   commDropCS( db, csName, true, "clear cs" );
   commDropDomain( db, domainName );
}