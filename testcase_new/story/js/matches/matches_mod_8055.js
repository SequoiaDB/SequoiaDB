/************************************************************************
*@Description:  seqDB-8055:使用$mod查询，除数为数组/对象且元素为数值型，不走索引查询
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8055";
   var cl = readyCL( clName );

   var rawData = ["-9223372036854775808", "9223372036854775807"];
   insertRecs( cl, rawData );
   var rc1 = findRecs( cl, -2147483648, 2147483647 );  //[div, rem]---[2,0]
   var rc2 = findRecs( cl, 2147483647, -2 );
   checkResult( rc1, rc2, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{
   cl.insert( [{ a: [{ b: NumberLong( rawData[0] ) }] },      //array
   { a: { b: NumberLong( rawData[1] ) } }] );   //subObj
}

function findRecs ( cl, div, rem )
{

   var rc = cl.find( { "a.b": { $mod: [div, rem] } } );

   return rc;
}

function checkResult ( rc1, rc2, rawData )
{
   //-----------------------check result for $mod[-2147483648, 2147483647]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"]["b"]["$numberLong"].toString() !== rawData[1] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[a:" + rawData[1] + "]" +
         "[b:" + findRtn[0]["a"]["b"]["$numberLong"].toString() + "]" );
   }

   //-----------------------check result for $mod[2147483647, -2]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"][0]["b"]["$numberLong"].toString() !== rawData[0] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[a:" + rawData[0] + "]" +
         "[b:" + findRtn[0]["a"][0]["b"]["$numberLong"].toString() + "]" );
   }
}