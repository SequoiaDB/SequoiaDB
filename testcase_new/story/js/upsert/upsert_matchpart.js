// upsert record.
// normal case.

main( test );
function test ()
{
   // clear
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   // create cs, cl
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false );
   var count = 100;

   for( i = 0; i < count - 10; i++ )
   {
      varCL.insert( { id: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "上海", city: "上海市" } } );
   }
   for( ; i < count; i++ )
   {
      varCL.insert( { id: i, mineName: "北京矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "北京", city: "北京市" } } );
   }

   varCL.upsert( { $set: { "localtion.street": "人民路12号" } }, { mineName: "北京矿场" } );

   var rc = varCL.find();

   var size = 0;
   while( rc.next() )
   {
      recordObj = rc.current().toObj();
      recordStr = rc.current().toJson();

      if( recordObj["mineName"] == "北京矿场" )
      {
         assert.equal( recordObj["localtion"]["street"], "人民路12号" );
      }
      else
      {
         assert.equal( recordObj["localtion"]["street"], null );
      }

      size++;
   }

   assert.equal( size, count );

   // clear
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );

}
