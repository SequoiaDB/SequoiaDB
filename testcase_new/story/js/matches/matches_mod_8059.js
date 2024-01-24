/************************************************************************
*@Description:  seqDB-8059:使用$mod查询，除数为0，余数取0 
*@Author:  2016/5/19  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8059";
   var cl = readyCL( clName );

   insertRecs( cl );
   var rc = findRecs( cl, 2, 0 );  //[div, rem]---[2,0]
   checkResult( rc );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{
   cl.insert( [{ a: 0 }] );
}

function findRecs ( cl, div, rem )
{

   var rc = cl.find( { a: { $mod: [div, rem] } } );

   return rc;
}

function checkResult ( rc )
{

   var findRtn = new Array();
   while( tmpRecs = rc.next() )
   {
      findRtn.push( tmpRecs.toObj() );
   }
   //compare records
   if( findRtn.length !== 1 || findRtn[0]["a"] !== 0 )
   {
      throw new Error( "checkResult fail,[compare records]" +
         "[findRtnLen:" + 1 + ", a:" + 0 + "]" +
         "[findRtnLen:" + findRtn.length + ", a:" + findRtn[0]["a"] + "]" );
   }
}