/************************************************************************
*@Description:    seqDB-10237 : 使用$field查询，并按_id字段排序
*@Author:  2016/10/18  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches10237";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( { a: 1, b: 1 } );
   cl.insert( { a: 2, b: 2 } );
   cl.insert( { a: 1, b: 2 } );
}

function findRecs ( cl )
{

   var rc = cl.find( { a: { $field: "b" } } ).sort( { _id: 1 } )

   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray )
{

   var expLen = 2;
   assert.equal( findRecsArray.length, expLen );
   var actA1 = findRecsArray[0]["a"];
   var actB1 = findRecsArray[0]["b"];
   var actA2 = findRecsArray[1]["a"];
   var actB2 = findRecsArray[1]["b"];
   var actRecs = '{a:' + actA1 + ', b:' + actB1 + '}, {a:' + actA2 + ', b:' + actB2 + '}';
   var expRecs = '{a:1, b:1}, {a:2, b:2}';
   assert.equal( actRecs, expRecs );
}