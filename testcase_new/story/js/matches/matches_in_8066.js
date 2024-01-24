/************************************************************************
*@Description:   seqDB-8066:使用$in查询，给定值为空（如{$in:[]}） 
                      seqDB-8063, value为1个
                     dataType: array
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8066";
   var cl = readyCL( clName );

   var rawData = [{ b: 2147483648 }, { c: 1.7E+308 }];
   insertRecs( cl, rawData );

   var rc1 = findRecs( cl, { a: { $in: [] } } );
   var rc2 = findRecs( cl, { a: { $in: [{ b: 2147483648 }] } } );

   checkResult( rc1, rc2, rawData );

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

function checkResult ( rc1, rc2, rawData )
{
   //-----------------------check result for $in[]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 0;
   assert.equal( findRtn.length, expLen );

   //-----------------------check result for $in[]---------------------

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare results
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"][0]["b"] !== rawData[0]["b"] ||
      findRtn[0]["a"][1]["c"] !== rawData[1]["c"] )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[{b:" + rawData[0]["b"] + "}, {c:" + rawData[1]["c"] + "}]" + "[{b:" + findRtn[0]["a"][0]["b"]
         + "}, {c:" + findRtn[0]["a"][1]["c"] + "}]" );
   }
}