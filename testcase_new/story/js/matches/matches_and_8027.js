/************************************************************************
*@Description:    seqDB-8027:使用$and查询，给定值为空（如{$and:[]}） 
*@Author:  2016/5/24  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8027";
   var cl = readyCL( clName );

   insertRecs( cl );
   var rc1 = findRecs( cl, { $and: [{ a: 0 }] } );
   var rc2 = findRecs( cl, { $and: [] } );
   checkResult( rc1, rc2 );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0 },
   { a: 1, b: null }] );
}

function findRecs ( cl, cond )
{

   var rc = cl.find( cond ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc1, rc2 )
{
   //---------------------------------check results for find[$and:[{a:1}]]-----------------------------

   var findRtn = new Array();
   while( tmpRecs = rc1.next() )  //rc1
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 1;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"] !== 0 )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[a:" + 0 + "]" +
         "[a:" + findRtn[0]["a"] + "]" );
   }

   //---------------------------------check results for find[$and:[{a:1}]]-----------------------------

   var findRtn = new Array();
   while( tmpRecs = rc2.next() )  //rc2
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["a"] !== 0 || findRtn[1]["a"] !== 1 )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[a:" + 0 + ", a:" + 1 + "]" +
         "[a:" + findRtn[0]["a"] + ", a:" + findRtn[1]["a"] + "]" );
   }
}