/* *****************************************************************************
@discretion: alter cs, the test scenario is as follows:
test 15038: alter pagesize
test 15040: alter lobPageSize
@author��2018-4-27 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15038";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCS( db, csName, true, "drop cs" );

   //create cs
   var dbcs = commCreateCS( db, csName, false, "create CS" );

   //testcase:15038/15040
   var pageSize = 4096;
   var lobPageSize = 524288;
   dbcs.setAttributes( { PageSize: pageSize, LobPageSize: lobPageSize } );
   checkAlterCSResult( csName, "PageSize", pageSize );
   checkAlterCSResult( csName, "LobPageSize", lobPageSize );

   var defPageSize = 65536;
   var defLobPageSize = 262144;
   dbcs.setAttributes( { PageSize: 0, LobPageSize: 0 } );
   checkAlterCSResult( csName, "PageSize", defPageSize );
   checkAlterCSResult( csName, "LobPageSize", defLobPageSize );

   //clean
   commDropCS( db, csName, true, "clear cs" );
}