/******************************************************************************
 * @Description   : seqDB-12267:and组合条件，执行删除
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.27
 * @LastEditTime  : 2021.07.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12267";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var docs = [];
   docs.push( { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: ">女" } } );
   docs.push( { no: 1005, score: 70, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } } );
   docs.push( { no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } } );
   docs.push( { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } } );
   docs.push( { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } } );
   docs.push( { no: 1004, score: 69, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } } );
   docs.push( { no: 1008, score: 72, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } } );
   docs.push( { no: 1006, score: 84, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } } );
   docs.push( { no: 1007, score: 73, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } } );
   docs.push( { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } } );
   docs.push( { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } } );
   docs.push( { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } } );
   docs.push( { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } } );
   docs.push( { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } } );
   docs.push( { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 28, sex: "男" } } );
   docs.push( { no: 1016, score: 99, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } } );
   docs.push( { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } } );
   varCL.insert( docs );

   varCL.remove( { $and: [{ score: { $gte: 85, $lte: 96, $ne: 90 } }, { "info.age": { $gt: 25, $lt: 30 } }, { "info.name": { $nin: ["Kate", "Tom"] } }, { "info.sex": { $in: ["男"] } }] } );
   docs.splice( 14, 1 );
   var cursor = varCL.find();
   commCompareResults( cursor, docs );
}