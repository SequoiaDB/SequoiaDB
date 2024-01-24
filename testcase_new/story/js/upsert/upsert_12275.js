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

   var docs = [];
   var expectDocs = [];
   for( i = 0; i < count - 10; i++ )
   {
      docs.push( { id: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "上海", city: "上海市" } } );
      expectDocs.push( { id: i, mineName: "上海矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "上海", city: "上海市" } } );
   }
   for( ; i < count; i++ )
   {
      docs.push( { id: i, mineName: "北京矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "北京", city: "北京市" } } );
      expectDocs.push( { id: i, mineName: "北京矿场", mineTime: "2013-06-14", localtion: { resId: 0, resourceName: null, country: "中国", state: "北京", city: "北京市", street: "人民路12号" } } );
   }

   varCL.insert( docs );

   varCL.upsert( { $set: { "localtion.street": "人民路12号" } }, { mineName: "北京矿场" } );


   checkResult( varCL, {}, expectDocs );
   // clear
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );

}