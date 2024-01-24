main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   cl.bulkInsert();
   var cursor = cl.execAggregate( { $group: { _id: { dep: "$dep", major: "$major" } } } );
   var expectResult = [{ "no": 1010, "score": 93, "major": "光学", "dep": "物电学院", "info": { "name": "Coco", "age": 27, "sex": "女" } },
   { "no": 1006, "score": 84, "interest": ["basketball", "football", "movie", "photo"], "major": "物理学", "dep": "物电学院", "info": { "name": "Lily", "age": 28, "sex": "女" } },
   { "no": 1013, "score": 86, "interest": ["basketball", "movie", "photo"], "major": "电学", "dep": "物电学院", "info": { "name": "Jaden", "age": 20, "sex": "男" } },
   { "no": 1004, "score": 69, "interest": ["basketball", "football", "movie"], "major": "计算机工程", "dep": "计算机学院", "info": { "name": "Coll", "age": 26, "sex": "男" } },
   { "no": 1000, "score": 80, "interest": ["basketball", "football"], "major": "计算机科学与技术", "dep": "计算机学院", "info": { "name": "Tom", "age": 25, "sex": "男" } },
   { "no": 1002, "score": 85, "interest": ["movie", "photo"], "major": "计算机软件与理论", "dep": "计算机学院", "info": { "name": "Holiday", "age": 22, "sex": "女" } }];
   var parameters = "{$group:{_id:{dep:'$dep', major:'$major'}}}"
   checkResult( cursor, expectResult, parameters );

   cl.drop();
}
