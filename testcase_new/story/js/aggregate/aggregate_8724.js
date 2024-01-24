
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $match: { interest: { $exists: 1 } } }, { $group: { _id: "$major", avg_age: { $avg: "$info.age" }, major: { $first: "$major" } } }, { $sort: { avg_age: -1, major: -1 } }, { $skip: 2 }, { $limit: 3 } )
   var expectResult = [{ "avg_age": 25, "major": "计算机科学与技术" }, { "avg_age": 22, "major": "计算机软件与理论" }, { "avg_age": 22, "major": "物理学" }];
   var parameters = "{$match:{interest:{$exists:1}}}, {$group:{_id:'$major', avg_age:{$avg:'$info.age'}, major:{$first:'$major'}}}, {$sort:{avg_age:-1, major:-1}}, {$skip:2}, {$limit:3}";
   checkResult( cursor, expectResult, parameters );

   cl.drop();
}
