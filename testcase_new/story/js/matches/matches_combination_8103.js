/************************************************************************
*@Description:    seqDB-8103:相同字段组合测试
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8103";
   var cl = readyCL( clName );

   insertRecs( cl );

   var findRecsArray = findRecs( cl );
   checkResult( findRecsArray );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0 }, { a: 1 }] );
}

function findRecs ( cl )
{

   var rc = cl.find( { a: { $lt: 0 }, a: { $et: 1 } } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray )
{

   var expLen = 1;
   assert.equal( findRecsArray.length, expLen );
   var actA = findRecsArray[0]["a"];
   var expA = 1;
   if( actA !== expA )
   {
      throw new Error( "checkResult fail,[compare records]" +
         '["expA": ' + expA + ']' +
         '["actA": ' + actA + ']' );
   }
}