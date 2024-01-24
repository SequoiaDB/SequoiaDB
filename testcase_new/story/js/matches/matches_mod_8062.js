/************************************************************************
*@Description:  seqDB-8062:使用$mod查询，除数为其他类型数据 
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8062";
   var cl = readyCL( clName );

   insertRecs( cl );
   var rc = findRecs( cl, 1, 0 );  //[div, rem]---[1,0]
   checkResult( rc );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{
   cl.insert( [{ a: "test" }] );
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
   assert.equal( findRtn.length, 0 );
}