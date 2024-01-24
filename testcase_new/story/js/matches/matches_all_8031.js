/************************************************************************
*@Description:   seqDB-8031:使用$all查询，给定值为空（如{$all:[]}）  
                     dataType: array
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8031";
   var cl = readyCL( clName );

   var rawData = [{ b: 2147483648 }, { c: 1.7E+308 }, { d: "test" }];
   insertRecs( cl, rawData );

   var rc1 = findRecs( cl, { a: { $all: [] } } );
   var rc2 = findRecs( cl, { a: { $all: [{ b: 2147483648 }, { d: "test" }] } } );
   var rc3 = findRecs( cl, { a: { $all: [{ b: 2 }] } } );

   checkResult( rc1, rc2, rc3, rawData );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl, rawData )
{

   cl.insert( { a: rawData } );
}

function findRecs ( cl, cond )
{

   var rc = cl.find( cond );

   return rc;
}

function checkResult ( rc1, rc2, rc3, rawData )
{
   //-----------------------check result for $all[]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"][0]["b"] !== rawData[0]["b"] ||
      findRtn[0]["a"][1]["c"] !== rawData[1]["c"] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[{b:" + rawData[0]["b"]
         + "}, {c:" + rawData[1]["c"] + "}]",
         "[{b:" + findRtn[0]["a"][0]["b"]
         + "}, {c:" + findRtn[0]["a"][1]["c"] + "}]" );
   }

   //-----------------------check result for $all[]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"][0]["b"] !== rawData[0]["b"] ||
      findRtn[0]["a"][1]["c"] !== rawData[1]["c"] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[{b:" + rawData[0]["b"]
         + "}, {c:" + rawData[1]["c"] + "}]",
         "[{b:" + findRtn[0]["a"][0]["b"]
         + "}, {c:" + findRtn[0]["a"][1]["c"] + "}]" );
   }

   //-----------------------check result for $all[]---------------------
   var findRtn = new Array();
   while( tmpRecs = rc3.next() )  //rc3
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 0;
   assert.equal( findRtn.length, expLen );
}