/************************************************************************
*@Description:   seqDB-8049:使用$field查询，其他所有匹配符组合使用
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8049";
   var cl = readyCL( clName );

   var rawData = insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   var rawData = [{
      a: 0, a2: 1,
      int: -2147483648, int2: -2147483648,
      tmp3: null
   }];
   cl.insert( rawData );

   return rawData;
}

function findRecs ( cl )
{

   var tmpCond = [{ a: { $ne: { $field: "a2" } } },
   { int: { $et: { $field: "int2" } } },
   { null: { $isnull: { $field: "tmp3" } } }];
   var rmNum = parseInt( Math.random() * tmpCond.length );
   var rc = cl.find( tmpCond[rmNum], { _id: { $include: 0 } } ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray, rawData )
{

   var expLen = 1;
   assert.equal( findRecsArray.length, expLen );
   var actRecs = JSON.stringify( findRecsArray[0] );
   var extRecs = JSON.stringify( rawData[0] );
   assert.equal( actRecs, extRecs );
}