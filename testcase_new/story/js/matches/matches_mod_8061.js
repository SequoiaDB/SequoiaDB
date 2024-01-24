/************************************************************************
*@Description:  seqDB-8061:使用$mod查询，被除数为小于1的小数 
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_matches8061";
   var cl = readyCL( clName );

   insertRecs( cl );
   cl.find( { a: { $mod: [0.6, 1] } } );

   commDropCL( db, COMMCSNAME, clName, false, false );

}

function insertRecs ( cl )
{
   cl.insert( [{ a: 3 }] );
}
