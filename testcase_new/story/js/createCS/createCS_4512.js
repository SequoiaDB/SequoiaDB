/****************************************************
@description: seqDB-4512:createCS������options:LobPageSize��Ч�ַ��ͱ߽�
@author:
2019-6-4 wuyan init
****************************************************/
main( test );
function test ()
{
   var csName = CHANGEDPREFIX + "cs4512";
   var lobPageSizes = [0, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288];

   for( var i = 0; i < lobPageSizes.length; i++ )
   {
      var lobPageSize = lobPageSizes[i];
      commDropCS( db, csName, true, "clear cs in the beginingng." );
      createCSAndCheckResult( csName, lobPageSize );
      commDropCS( db, csName, false, "clear cs in the ending." );
   }
}

function createCSAndCheckResult ( csName, lobPageSize )
{

   var options = { LobPageSize: lobPageSize };
   //create cs
   var dbcs = db.createCS( csName, options );

   //create cl in the cs, "ReplSize" need to set, avoid -264
   var clName = "cl4512";
   dbcs.createCL( clName, { "ReplSize": 0 } );

   //check the options
   db.sync( { "CollectionSpace": csName } );

   var cursor = db.snapshot( 5, { Name: csName, NodeSelect: "master" } );
   var actPageSize = 0;
   while( cursor.next() )
   {
      var curInfo = cursor.current();
      actPageSize = curInfo.toObj().LobPageSize;
   }

   var expPageSize = lobPageSize;
   if( lobPageSize == 0 )
   {
      //0��ΪĬ��ֵ262144
      expPageSize = 262144;
   }
   assert.equal( expPageSize, actPageSize );
}