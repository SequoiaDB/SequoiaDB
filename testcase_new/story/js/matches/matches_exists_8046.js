/************************************************************************
*@Description:   seqDB-8046:使用$exists:1查询
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8046";
   var cl = readyCL( clName );

   insertRecs( cl );
   var rc = findRecs( cl );
   checkResult( rc );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{

   cl.insert( [{ a: 0 },
   { a: 1, b: null },
   { a: 2, b: "" }] );
}

function findRecs ( cl )
{

   var rc = cl.find( { b: { $exists: 1 } } ).sort( { a: 1 } );

   return rc;
}

function checkResult ( rc )
{

   var findRtn = new Array();
   while( tmpRecs = rc.next() ) 
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare number
   var expLen = 2;
   assert.equal( findRtn.length, expLen );
   //compare records
   if( findRtn[0]["b"] !== null || findRtn[1]["b"] !== "" )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[b:" + null + ", b:" + "" + "]" +
         "[b:" + findRtn[0]["b"] + ", b:" + findRtn[1]["b"] + "]" );
   }
}