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
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   loadData( cl );
   var cursor = cl.execAggregate( { $project: { no: 1, Major: "$major", dep: 0, name: "$info.name", "info.sex": 0 } } )
   var expectRes = [{ "no": 1000, "Major": "计算机科学与技术", "name": "Tom" }, { "no": 1001, "Major": "计算机科学与技术", "name": "Json" },
   { "no": 1002, "Major": "计算机软件与理论", "name": "Holiday" }, { "no": 1003, "Major": "计算机软件与理论", "name": "Sam" },
   { "no": 1004, "Major": "计算机工程", "name": "Coll" }, { "no": 1006, "Major": "计算机工程", "name": "Jim" },
   { "no": 1007, "Major": "物理学", "name": "Lily" }, { "no": 1014, "Major": "物理学", "name": "Kiki" },
   { "no": 1015, "Major": "物理学", "name": "Appie" }, { "no": 1008, "Major": "物理学", "name": "Lucy" },
   { "no": 1009, "Major": "光学", "name": "Coco" }, { "no": 1010, "Major": "光学", "name": "Jack" },
   { "no": 1011, "Major": "光学", "name": "Mike" }, { "no": 1013, "Major": "电学", "name": "Jaden" },
   { "no": 1016, "Major": "电学", "name": "Iccra" }, { "no": 1017, "Major": "电学", "name": "Jay" },
   { "no": 1018, "Major": "电学", "name": "Kate" }];
   var parameter = "{$project:{no:1, Major:'$major', dep:0, name:'$info.name', 'info.sex':0}}"
   var ret = checkResult( cursor, expectRes, parameter );
   cl.drop();
}