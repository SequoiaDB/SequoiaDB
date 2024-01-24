
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   loadData( cl );
   testSortDesc( cl );
   testSortMix( cl );
   testSortAndGroup( cl );
   cl.drop();
}
function loadData ( cl )
{
   var docs = [{ no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } },
   { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } },
   { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } },
   { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } },
   { no: 1004, score: 80, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } },
   { no: 1005, score: 90, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } },
   { no: 1006, score: 82, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } },
   { no: 1007, score: 93, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } },
   { no: 1008, score: 75, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } },
   { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } },
   { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } },
   { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } },
   { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } },
   { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 20, sex: "男" } },
   { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } },
   { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } },
   { no: 1016, score: 92, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } }];
   cl.bulkInsert( docs );
}

function testSortDesc ( cl )
{
   var cursor = cl.execAggregate( { $sort: { no: -1 } } );
   var expectResult = [{ no: 1016, score: 92, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } },
   { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } },
   { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } },
   { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 20, sex: "男" } },
   { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } },
   { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } },
   { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } },
   { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } },
   { no: 1008, score: 75, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } },
   { no: 1007, score: 93, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } },
   { no: 1006, score: 82, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } },
   { no: 1005, score: 90, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } },
   { no: 1004, score: 80, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } },
   { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } },
   { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } },
   { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } },
   { no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } }];
   var parameters = "{$sort:{no: -1}";
   checkResult( cursor, expectResult, parameters );

}

function testSortMix ( cl )
{
   var cursor = cl.execAggregate( { $sort: { score: -1, "info.age": 1 } } );

   var expectResult = [{ no: 1007, score: 93, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } },
   { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } },
   { no: 1016, score: 92, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } },
   { no: 1005, score: 90, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } },
   { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } },
   { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 20, sex: "男" } },
   { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } },
   { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } },
   { no: 1006, score: 82, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } },
   { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } },
   { no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } },
   { no: 1004, score: 80, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } },
   { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } },
   { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } },
   { no: 1008, score: 75, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } },
   { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } },
   { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } }];
   var parameter = "{$sort:{score:-1, 'info.age':1}}";
   checkResult( cursor, expectResult, parameter );
}

function testSortAndGroup ( cl )
{
   var cursor = cl.execAggregate( { $sort: { no: 1, score: -1, "info.age": 1 } }, { $project: { major: 1 } }, { $match: { $and: [{ no: { $gt: 1010 } }, { dep: "物电学院" }] } }, { $limit: 4 }, { $skip: 1 }, { $group: { _id: "$major", avg_score: { $avg: "$score" }, Major: { $first: "$major" } } } )
   var expectResult = [{ "avg_score": null, "Major": null }];
   var parameter = "{$sort:{score:-1, 'info.age':1}}"
   checkResult( cursor, expectResult, parameter );
}
