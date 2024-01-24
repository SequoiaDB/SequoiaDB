/****************************************************
@description: seqDB-4517:createCS��createCS��options:PageSize��Чȡֵ
seqDB-4521:createCS��createCS��options:LobPageSize��Чȡֵ
@author:
2019-6-4 wuyan init
****************************************************/
main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   var csName = "cs4517";
   commDropCS( db, csName, true, "clear cs in the beginning." );

   var pageSizes = ["", 4097, -4096];
   var lobPageSize = 4096;
   for( var i = 0; i < pageSizes.length; i++ )
   {
      var pageSize = pageSizes[i];
      createCSWithLobPageSize( csName, lobPageSize, pageSize );
   }

   var lobPageSizes = ["", 1, -4096];
   var pageSize = 4096;
   for( var i = 0; i < lobPageSizes.length; i++ )
   {
      var lobPageSize = lobPageSizes[i];
      createCSWithLobPageSize( csName, lobPageSize, pageSize );
   }

}

function createCSWithLobPageSize ( csName, lobPageSize, pageSize )
{
   //create cs; 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var options = { LobPageSize: lobPageSize, PageSize: pageSize };
      db.createCS( csName, options );
   } );

   //check cs is not exist; 
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );

}