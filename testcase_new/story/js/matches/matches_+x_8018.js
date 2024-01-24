/************************************************************************
*@Description:   seqDB-8018:使用$+标识符查询，目标字段为嵌套数组，不走索引查询
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8018";
   var cl = readyCL( clName );

   var rawData = [{ a: 0, b: [1, 2, [3, 4, 5]] },
   { a: 1, b: [1, 3, [4, 5]] },
   { a: 2, b: [4, 2, 1] }];
   insertRecs( cl, rawData );

   var cond = { "b.2.$1": 5 };
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

   for( i = 0; i < rawData.length - 1; i++ )
   {
      //compare records number after find
      var expLen = 2;
      var actLen = findRecsArray.length;
      assert.equal( actLen, expLen )

      //compare records
      var actB = findRecsArray[i]["b"].toString();
      var expB = rawData[i]["b"].toString();
      assert.equal( actB, expB );
   }
}