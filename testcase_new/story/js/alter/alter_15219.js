/* *****************************************************************************
@discretion: alter cs name/pageSize/lobpagesize/domain
@author��2018-4-28 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15219";
var domainName = CHANGEDPREFIX + "_domain15219";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }

   //clean environment before test
   commDropCS( db, csName, true, "drop cs" );
   commDropDomain( db, domainName );

   //create domain, cs, cl
   var groupNames = getGroupName( db );
   commCreateDomain( db, domainName, [groupNames[0]] );
   var dbcs = commCreateCS( db, csName, false, "create CS" );

   //alter pageSize /domain/lobpageSize
   alterMultiCSIgnoreError( dbcs );

   //alter pageSize
   alterMultiCSErrorAbort( dbcs );

   //clean
   commDropCS( db, csName, true, "clear cs" );
   commDropDomain( db, domainName );
}

//abort execution of modification after fail
function alterMultiCSErrorAbort ( dbcs )
{
   try
   {
      var orgLobPageSize = 131072;
      var pageSize = 32768;
      var lobpageSize = 8192;
      dbcs.alter( {
         Alter: [{ Name: "set attributes", Args: { PageSize: pageSize } },
         { Name: "set attributes", Args: { Name: "testcs" } },
         { Name: "set attributes", Args: { LobPageSize: lobpageSize } },
         { Name: "set domain", Args: { Domain: domainName } }],
         Options: { IgnoreException: false }
      } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }

   //check the alter result
   checkAlterCSResult( csName, "PageSize", pageSize );
   checkAlterCSResult( csName, "Domain", domainName );
   checkAlterCSResult( csName, "LobPageSize", orgLobPageSize );
   checkAlterCSResult( csName, "Name", csName );
}

function alterMultiCSIgnoreError ( dbcs )
{
   var pageSize = 16384;
   var lobpageSize = 131072;
   dbcs.alter( {
      Alter: [{ Name: "set attributes", Args: { PageSize: pageSize } },
      { Name: "set attributes", Args: { LobPageSize: lobpageSize } },
      { Name: "set attributes", Args: { Name: "testcs" } },
      { Name: "set domain", Args: { Domain: domainName } }],
      Options: { IgnoreException: true }
   } );
   checkAlterCSResult( csName, "PageSize", pageSize );
   checkAlterCSResult( csName, "Domain", domainName );
   checkAlterCSResult( csName, "LobPageSize", lobpageSize );
   checkAlterCSResult( csName, "Name", csName );
}