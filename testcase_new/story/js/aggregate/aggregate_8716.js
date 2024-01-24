main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: { dep: "$dep" }, Major: { $first: "$major" }, first_score: { $first: "$score" }, last_score: { $last: "$score" } } } );
   var expectResult = [{ "Major": "物理学", "first_score": 84, "last_score": 92 },
   { "Major": "计算机科学与技术", "first_score": 80, "last_score": 70 }];
   var parameters = "{$group:{_id:{dep:'$dep'}, Major:{$first:'$major'}, first_score:{$first:'$score'}, last_score:{$last:'$score'}}}"
   checkResult( cursor, expectResult, parameters );

   cl.drop();
}