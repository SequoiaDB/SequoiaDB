
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   loadData( cl );
   var cursor = cl.execAggregate( { $match: { interest: { $exists: 1 } } }, { $project: { major: 1, "info.age": 1 } }, { $group: { _id: "$major", avg_age: { $avg: "$info.age" } } }, { $sort: { avg_age: -1 } } );
   var expectRes = [{ "avg_age": 28 }, { "avg_age": 26 }, { "avg_age": 25 }, { "avg_age": 22 }, { "avg_age": 22 }, { "avg_age": 19.5 }];
   var parameter = "{$match:{interest:{$exists:1}}}, {$project:{major:1, 'info.age':1}}, {$group:{_id:'$major', avg_age:{$avg:'$info.age'}}}, {$sort:{avg_age:-1}}"
   checkResult( cursor, expectRes, parameter );
   cl.drop();
}

function loadData ( cl )
{
   var docs = [{ no: 1000, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } },
   { no: 1001, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } },
   { no: 1002, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } },
   { no: 1003, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } },
   { no: 1004, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } },
   { no: 1006, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } },
   { no: 1007, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } },
   { no: 1014, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } },
   { no: 1015, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } },
   { no: 1008, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } },
   { no: 1009, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } },
   { no: 1010, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } },
   { no: 1011, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } },
   { no: 1013, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 20, sex: "男" } },
   { no: 1016, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } },
   { no: 1017, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } },
   { no: 1018, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } }];
   cl.bulkInsert( docs );

}