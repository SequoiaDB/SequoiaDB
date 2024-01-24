main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: "$major", major: { $first: "$major" }, score: { $first: "$score" } } }, { $sort: { major: -1 } }, { $project: { major: 0, score: 1 } }, { $limit: 3 } );
   var expectResult = [{ "score": 85 }, { "score": 80 }, { "score": 69 }];
   var parameters = "{$group:{_id:'$major', major:{$first:'$major'}, score:{$first:'$score'}}}, {$sort:{major:-1}}, {$project:{major:0, score:1}}, {$limit:3}";

   checkResult( cursor, expectResult, parameters );

   cl.drop();
}