/*******************************************************************************
*@Description : Aggregate limit test
*@Modify list :
*               2016-03-18  wenjing wang rewrite
*******************************************************************************/
main( test );
function test ()
{
   var cl = new collection( db, COMMCSNAME, COMMCLNAME );
   cl.create();
   var docNumber = cl.bulkInsert();
   testLimitMinBound( cl, docNumber );
   testLimitAnyValue( cl );
   testLimitMaxBound( cl, docNumber );
   cl.drop();
}
function testLimitMinBound ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $limit: -1 } );
   var retNumber = getRetNumber( cursor )
   assert.equal( retNumber, docNumber );
}

function testLimitMaxBound ( cl, docNumber )
{
   var cursor = cl.execAggregate( { $limit: 18 } );
   var retNumber = getRetNumber( cursor );
   assert.equal( retNumber, docNumber );
}

function testLimitAnyValue ( cl )
{
   cursor = cl.execAggregate( { $limit: 3 } );
   var expectResult = [{ no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } },
   { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } },
   { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } }]
   var parameters = "{$limit:3}";
   checkResult( cursor, expectResult, parameters );
}
