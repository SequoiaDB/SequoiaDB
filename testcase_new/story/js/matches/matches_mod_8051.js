/************************************************************************
*@Description:  seqDB-8051:使用$mod查询，除数/被除数/余数都为整数，不走索引查询
*@Author:  2016/5/19  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8051";
   var cl = readyCL( clName );

   var rawData = [-2147483648, 2147483647];
   insertRecs( cl, rawData );
   var rc1 = findRecs( cl, 2, 0 );  //[div, rem]---[2,0]
   var rc2 = findRecs( cl, 2, 1 );
   checkResult( rc1, rc2, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{
   cl.insert( [{ a: rawData[0] }, { a: rawData[1] }] );
}

function findRecs ( cl, div, rem )
{

   var rc = cl.find( { a: { $mod: [div, rem] } } );

   return rc;
}

function checkResult ( rc1, rc2, rawData )
{
   //-----------------------check result for $mod[2,0]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"] !== rawData[0] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[a:" + rawData[0] + "]" +
         "[a:" + findRtn[0]["a"] + "]" );
   }

   //-----------------------check result for $mod[2,1]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"] !== rawData[1] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[a:" + rawData[1] + "]" +
         "[a:" + findRtn[0]["a"] + "]" );
   }
}