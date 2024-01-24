/*******************************************************************************
*@Description : Aggregate match|group|match combination
*@Modify list :
*               2016-03-18  wenjing wang rewrite
*******************************************************************************/
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $match: { no: { $gt: 1001 }, "info.sex": "男" } }, { $group: { _id: "$major", major: { $first: "$major" }, last_score: { $last: "$score" } } }, { $match: { last_score: { $gte: 70 } } } );
   var expectResult = [{ major: "光学", last_score: 78 }, { major: "电学", last_score: 92 }, { major: "计算机软件与理论", last_score: 90 }];
   var parameters = "{$match:{no:{$gt:1001}, info.sex:'男'}}, {$group:{_id:'$major', major:{$first:'$major'}, last_score:{$last:'$score'}}}, {$match:{last_score:{$gte:70}}}";
   checkResult( cursor, expectResult, parameters );
   cl.drop();
}
