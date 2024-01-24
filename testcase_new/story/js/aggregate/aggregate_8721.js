
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   testGroupByNull( cl, docNumber );
   testGroupByNotExist( cl );
   cl.drop();
}

function testGroupByNull ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $group: { _id: null } } );
   var retNumber = getRetNumber( cursor );
   assert.equal( retNumber, docNumber );
}

function testGroupByNotExist ( cl )
{
   var expectResult = [{ no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } }]
   var cursor = cl.execAggregate( { $group: { _id: "$field" } } );
   var parameters = "{$group:{_id:'$field'}}";
   checkResult( cursor, expectResult, parameters );
}