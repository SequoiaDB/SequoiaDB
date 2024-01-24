main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: "$major", major: { "$first": "$major" }, maxscore: { $max: "$score" }, minscore: { $min: "$score" }, avg_score: { $avg: "$score" }, sum_score: { $sum: "$score" } } } );
   var expectResult = [{ major: "光学", maxscore: 93, minscore: 75, avg_score: 82, sum_score: 246 },
   { major: "物理学", maxscore: 84, minscore: 72, avg_score: 77.25, sum_score: 309 },
   { major: "电学", maxscore: 92, minscore: 74, avg_score: 83.25, sum_score: 333 },
   { major: "计算机工程", maxscore: 70, minscore: 69, avg_score: 69.5, sum_score: 139 },
   { major: "计算机科学与技术", maxscore: 82, minscore: 80, avg_score: 81, sum_score: 162 },
   { major: "计算机软件与理论", maxscore: 90, minscore: 85, avg_score: 87.5, sum_score: 175 }];
   var parameters = "{$group:{_id:'$major', major:{'$first':'$major'}, maxscore:{$max:'$score'}, " +
      "minscore:{$min:'$score'}, avg_score:{$avg:'$score'}, sum_score:{$sum:'$score'}}}"
   checkResult( cursor, expectResult, parameters );
   cl.drop();
}
