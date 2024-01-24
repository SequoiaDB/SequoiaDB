/************************************************************************
*@Description:     seqDB-8100:使用$regex查询，不走索引查询
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8100";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl, rawData );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{

   var rawData = [{ a: 0, str: "sequoiadb@163.com" },
   { a: 1, str: "18826411857" },
   { a: 2, str: " " }];
   for( i = 0; i < rawData.length; i++ )
   {
      cl.insert( rawData[i] )
   }
   return rawData;
}

function findRecs ( cl )
{

   var cond1 = { str: { $regex: '^[_a-z0-9]+@([_a-z0-9]+\.)+[_a-z0-9]{2,3}$', $options: 'i' } };
   var cond2 = { str: { $regex: '^[1][8][0-9]{9}$', $options: 'i' } };
   var cond3 = { str: { $regex: '^[ ]+$', $options: 'i' } };
   var condArray = [cond1, cond2, cond3];

   var findRecsArray = [];
   for( i = 0; i < condArray.length; i++ )
   {
      var rc = cl.find( condArray[i] ).sort( { a: 1 } );
      var tmpArray = [];
      while( tmpRecs = rc.next() )
      {
         tmpArray.push( tmpRecs.toObj() );
      }
      findRecsArray.push( tmpArray );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   //total results
   var expLen = 3;
   var actLen = findRecsArray.length;
   assert.equal( actLen, expLen );


   //compare resulst for each find
   for( i = 0; i < findRecsArray.length; i++ )
   {
      //compare number
      var expLen = 1;
      var actLen = findRecsArray[i].length;
      assert.equal( actLen, expLen )
      //compare records
      var actB = findRecsArray[i][0]["str"];
      var expB = rawData[i]["str"];
      assert.equal( actB, expB );

   }
}