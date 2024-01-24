/* *****************************************************************************
@discretion: alter cs, the cs exist cl, the test scenario is as follows:
test 15039: alter pagesize
test 15041: alter lobPageSize
@author��2018-4-27 wuyan  Init
***************************************************************************** */
var csName = CHANGEDPREFIX + "_cs15039";
var clName = CHANGEDPREFIX + "_cs15039";

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
   var dbcl = dbcs.createCL( clName );

   //testcase15039:alter pagasize, cs exists cl
   alterPageSizeInThePresenceCL( dbcs );

   //testcase15041b:alter lobpagesize, there is no lob in cl
   dbcl.insert( { a: 1, b: 1 } );
   var lobPageSize = 524288;
   dbcs.setAttributes( { LobPageSize: lobPageSize } );
   checkAlterCSResult( csName, "LobPageSize", lobPageSize );

   //testcase15041a:alter lobpagesize, there is lob in cl
   putLob( dbcl );
   alterLobPageSizeExistLob( dbcs );

   //clean
   commDropCS( db, csName, true, "clear cs" );
}

function alterPageSizeInThePresenceCL ( dbcs )
{
   try
   {
      var pageSize = 4096;
      dbcs.setAttributes( { PageSize: pageSize } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      //-275:There are some collections in the collection space
      if( e.message != SDB_DMS_CS_NOT_EMPTY )
      {
         throw e;
      }
   }
}

function alterLobPageSizeExistLob ( dbcs )
{
   try
   {
      var lobPageSize = 8192;
      dbcs.setAttributes( { LobPageSize: lobPageSize } );
      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( e.message != SDB_OPTION_NOT_SUPPORT )
      {
         throw e;
      }
   }
}

function putLob ( dbcl )
{
   //generate a lob file
   var fileName = CHANGEDPREFIX + "_lobtest15039.file";
   var fileSize = "1024k";
   var cmd = new Cmd();
   var str = "dd if=/dev/zero of=" + fileName + " bs=" + fileSize + " count=1";
   cmd.run( str );

   //putLob
   dbcl.putLob( fileName );
   //clear file
   cmd.run( "rm -rf *" + CHANGEDPREFIX + "*.file" );
}