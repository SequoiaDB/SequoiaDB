/************************************
*@Description: 创建固定集合，测试update、delete、truncate接口
*@author:      luweikang
*@createdate:  2018.2.11
*@testlinkCase:seqDB-11831
**************************************/

main( test );
function test ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "11831";
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   var cl = commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, false, "create cappedCL" );

   //insertData
   var doc = buildData();
   cl.insert( doc );

   //update
   var options = { $inc: { No: 1 } };
   updateData( cl, options );
   checkResult( cl, doc );

   //delete
   removeData( cl );
   checkResult( cl, doc );

   //truncate,32M>record
   insertBigData( cl, 32 );
   cl.truncate();
   checkResult( cl, null );

   //truncate,32M<record<96M
   insertBigData( cl, 64 );
   cl.truncate();
   checkResult( cl, null );

   //truncate,96M<record
   insertBigData( cl, 108 );
   cl.truncate();
   checkResult( cl, null );

   //insert data check _id
   insertData( cl );

   //clean environment after test
   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function buildData ()
{
   var doc = [{ No: 1, a: 10 }, { No: 2, a: 50 }, { No: 3, a: -1001 },
   { No: 4, a: { $decimal: "123.456" } }, { No: 5, a: 101.02 },
   { No: 6, a: { $numberLong: "9223372036854775807" } }, { No: 7, a: { $numberLong: "-9223372036854775808" } },
   { No: 8, a: { $date: "2017-05-01" } }, { No: 9, a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { No: 10, a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { No: 11, a: { $regex: "^z", $options: "i" } },
   { No: 12, a: null },
   { No: 13, a: { $oid: "123abcd00ef12358902300ef" } },
   { No: 14, a: "abc" },
   { No: 15, a: { MinKey: 1 } },
   { No: 16, a: { MaxKey: 1 } },
   { No: 17, a: true }, { No: 18, a: false },
   { No: 19, a: { name: "Jack" } },
   { No: 20, a: [1] },
   { No: 21, a: [3] },
   { No: 22, a: 22 },
   { No: 23, a: [23, 23.3, "23"] },
   { No: 24, a: "" }];
   return doc;
}

function checkResult ( cl, expRec )
{
   if( expRec === null )
   {
      assert.tryThrow( [SDB_DMS_EOC, SDB_DMS_CONTEXT_IS_CLOSE], function()
      {
         cl.find().current();
      } );
   }
   else
   {
      var cursor = cl.find();
      checkRec( cursor, expRec );
   }
}

function updateData ( cl, options )
{
   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      cl.update( options );
   } );
}

function removeData ( cl )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( { a: 'test_delete_last_record' } );
      var id = cl.findOne().sort( { _id: -1 } ).current().toObj()._id;
      cl.remove( { _id: id } );
      cl.remove();
   } );
}

function insertData ( cl )
{
   var doc1 = [{ a: "abcde" },
   { a: "hjsdh" },
   { a: "jdwji" },
   { a: "qwieu" },
   { a: "niwew" }];
   var recordSize = recordHeader + 5 ;
   if ( recordSize % 4 != 0 )
   {
      recordSize = recordSize + ( 4 - recordSize % 4 );
   }
   cl.insert( doc1 );
   for( i = 0; i < 5; i++ )
   {
      var cursor = cl.find().sort( { _id: 1 } ).skip( i ).limit( 1 );
      var actId = cursor.current().toObj()._id;
      var expId = i * recordSize;
      assert.equal( actId, expId );
   }

}

function insertBigData ( cl, size )
{
   var recordnum = size * 102;
   var str = "sdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjksdhfjksdhsdjfhjks";
   var doc3 = new Array();
   var record = { a: str };
   for( i = 0; i < 10; i++ )
   {
      doc3.push( record );
   }
   for( j = 0; j < recordnum; j++ )
   {
      cl.insert( doc3 );
   }
}
