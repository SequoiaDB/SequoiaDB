main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: "$dep", addtoset_major: { $addtoset: "$major" }, push_no: { $push: "$no" } } }, { $skip: 1 }, { $limit: 1 } );
   var expectResult = [
      {
         "addtoset_major": ["计算机科学与技术", "计算机软件与理论", "计算机工程"],
         "push_no": [1000, 1001, 1002, 1003, 1004, 1005]
      }];
   var parameters = "{$group:{_id:'$dep', addtoset_major:{$addtoset:'$major'}, push_no:{$push:'$no'}}}, {$skip:1}, {$limit:1}"
   checkResult( cursor, expectResult, parameters );

   cl.drop();
}