/************************************************************************
*@Description:   seqDB-8032:使用$elemMatch查询，目标字段为对象，不走索引查询 
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8032";
   var cl = readyCL( clName );

   var rawData = [{ a: 0, b: {} },
   { a: 1, b: { c: "test" } },
   { a: 2, b: { c: "test", d: "" } }];
   insertRecs( cl, rawData );

   var cond = { b: { $elemMatch: { c: "test", d: "" } } };
   var findRecsArray = findRecs( cl, cond );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{
   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( rawData[i] )
   }
}

function findRecs ( cl, cond )
{

   var rc = cl.find( cond ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   //compare records number after find
   var expLen = 1;
   var actLen = findRecsArray.length;
   assert.equal( actLen, expLen );


   //compare records
   var actB = findRecsArray[0]["b"].toString();
   var expB = rawData[2]["b"].toString();
   assert.equal( actB, expB );
}