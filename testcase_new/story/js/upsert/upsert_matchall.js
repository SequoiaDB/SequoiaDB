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

   for( i = 0; i < count; i++ )
   {
      varCL.insert( { id: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "黑龙江", city: "佳木斯市" } } );
   }

   varCL.upsert( { $set: { "localtion.street": "人民路12号" } } );

   var rc = varCL.find();

   var size = 0;
   while( rc.next() )
   {
      recordObj = rc.current().toObj();
      recordStr = rc.current().toJson();

      assert.equal( recordObj["localtion"]["street"], "人民路12号" );
      size++;
   }

   assert.equal( size, count );

   // clear
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
}