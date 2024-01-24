/****************************************************
@description: seqDB-4510:createCS������options��PageSize��Ч�ַ��ͱ߽�
@author:
2019-6-4 wuyan init
****************************************************/
main( test );
function test ()
{
   var csName = CHANGEDPREFIX + "cs4510";
   var pageSizes = [0, 4096, 8192, 16384, 32768, 65536];

   for( var i = 0; i < pageSizes.length; i++ )
   {
      var pageSize = pageSizes[i];
      commDropCS( db, csName, true, "clear cs in the beginning." );
      createCSAndCheckResult( csName, pageSize );
      commDropCS( db, csName, false, "clear cs in the ending." );
   }
}

function createCSAndCheckResult ( csName, pageSize )
{

   var options = { PageSize: pageSize };
   //create cs; 
   var dbcs = db.createCS( csName, options );

   //create cl in the cs, "ReplSize" need to set, avoid -264
   var clName = "cl4510";
   dbcs.createCL( clName, { "ReplSize": 0 } );

   //check the options
   db.sync( { "CollectionSpace": csName } );

   var cursor = db.snapshot( 5, { Name: csName, NodeSelect: "master" } );
   var actPageSize = 0;
   while( cursor.next() )
   {
      var curInfo = cursor.current();
      actPageSize = curInfo.toObj().PageSize;
   }

   var expPageSize = pageSize;
   if( pageSize == 0 )
   {
      expPageSize = 65536;
   }

   assert.equal( expPageSize, actPageSize );

}