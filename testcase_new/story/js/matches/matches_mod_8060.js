/************************************************************************
*@Description:  seqDB-8060:使用$mod查询，被除数为0
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{

   var clName = COMMCLNAME + "_matches8060";
   var cl = readyCL( clName );

   insertRecs( cl );
   cl.find( { a: { $mod: [0, 1] } } );
   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{
   cl.insert( [{ a: 3 }] );
}
