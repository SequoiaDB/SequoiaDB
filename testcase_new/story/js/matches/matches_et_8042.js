/************************************************************************
*@Description:     seqDB-8042:使用$et查询，目标字段与给定值为不同类型，不走索引查询
                     data type: int/long, array/subObj
                     index:{b:1}
*@Author:  2016/5/18  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8042";

   var cl = readyCL( clName );

   insertRecs( cl );

   var intRc = findRecs( cl, "int" );  // int/long
   var arrRc = findRecs( cl, "array" );  // array/subObj

   checkResult( intRc, arrRc );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0, b: 1 },
   { a: 1, b: NumberLong( 1 ) },
   { a: 2, b: [{ "c": 1 }] },
   { a: 3, b: { "0": { "c": 1 } } }] );
}

function findRecs ( cl, dataType )
{
   if( dataType === "int" )
   {
      var rc = cl.find( { b: { $et: 1 } } ).sort( { a: 1 } );
   }
   else if( dataType === "array" )
   {
      var rc = cl.find( { "b.0.c": { $et: 1 } } ).sort( { a: 1 } );
   }
   return rc;
}

function checkResult ( intRc, arrRc )
{
   //-----------------------check result for dataType[int/long]---------------------

   var findRtn = new Array();
   while( tmpRecs = intRc.next() )  //incRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["b"] !== 1 || findRtn[1]["b"] !== 1 )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + 1 + ",b:" + 1 + "]" +
         "[b:" + findRtn[0]["b"] + ",b:" + findRtn[0]["b"] + "]" );
   }

   //-----------------------check result for dataType[array/subObj]---------------------

   var findRtn = new Array();
   while( tmpRecs = arrRc.next() )  //arrRc
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["b"][0]["c"] !== 1 || findRtn[1]["b"][0]["c"] !== 1 )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + 1 + ",b:" + 1 + "]" +
         "[b:" + findRtn[0]["b"][0]["c"] + ",b:" + findRtn[1]["b"][0]["c"] + "]" );
   }
}