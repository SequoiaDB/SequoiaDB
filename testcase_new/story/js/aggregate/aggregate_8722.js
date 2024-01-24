/************************************************************************
@Description:  Deal with only have "null" field value by using group
"avg/sum/min/max" compattible with MongDB
@Modify list:
2014/3/10   huxiaojun
************************************************************************/
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   loadData( cl );
   var cursor = cl.execAggregate( { $group: { max_age: { $max: "$info.age" }, min_age: { $min: "$info.age" }, avg_age: { $avg: "$info.age" }, sum_age: { $sum: "$info.age" } } } )
   var expectResult = [{ max_age: null, min_age: null, avg_age: null, sum_age: null }];
   var parameters = "{$group:{max_age:{$max:'$info.age'}, min_age:{$min:'$info.age'}, avg_age:{$avg:'$info.age'}, sum_age:{$sum:'$info.age'}}}";
   checkResult( cursor, expectResult, parameters );

   cl.drop();
}
function loadData ( cl )
{
   var docs = [{ no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: null, sex: "男" } },
   { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: null, sex: "男" } },
   { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: null, sex: "女" } },
   { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: null, sex: "男" } },
   { no: 1004, score: 69, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: null, sex: "男" } },
   { no: 1005, score: 70, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: null, sex: "女" } },
   { no: 1006, score: 84, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: null, sex: "女" } },
   { no: 1007, score: 73, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: null, sex: "女" } },
   { no: 1008, score: 72, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: null, sex: "女" } },
   { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: null, sex: "女" } },
   { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: null, sex: "女" } },
   { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: null, sex: "男" } },
   { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: null, sex: "男" } },
   { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: null, sex: "男" } },
   { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: null, sex: "男" } },
   { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: null, sex: "男" } },
   { no: 1016, score: 92, major: "电学", dep: "物电学院", info: { name: "Kate", age: null, sex: "男" } }
   ];
   cl.bulkInsert( docs );
}
