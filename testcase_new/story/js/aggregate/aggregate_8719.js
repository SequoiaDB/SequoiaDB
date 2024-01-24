main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: "$info.sex" } } );
   var expectResult = [{ "no": 1002, "score": 85, "interest": ["movie", "photo"], "major": "计算机软件与理论", "dep": "计算机学院", "info": { "name": "Holiday", "age": 22, "sex": "女" } },
   { "no": 1000, "score": 80, "interest": ["basketball", "football"], "major": "计算机科学与技术", "dep": "计算机学院", "info": { "name": "Tom", "age": 25, "sex": "男" } }];
   var parameters = "{$group:{_id:'$info.sex'}}"
   checkResult( cursor, expectResult, parameters );

   cl.drop();
}
