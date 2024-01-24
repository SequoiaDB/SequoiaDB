/************************************************************************
*@Description:   使用$type查询，value取无效编码
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_matches8093";
   var cl = readyCL( clName );

   insertRecs( cl );
   cl.find( { a: { $type: 1, $et: 6 } } );
   commDropCL( db, COMMCSNAME, clName, false, false );
}

function insertRecs ( cl )
{

   cl.insert( { a: "test" } );
}