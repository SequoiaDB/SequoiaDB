/*******************************************************************************
*@Description:  seqDB-5248:find操作，匹配符$eleMatch，多层嵌套_ST.basicOperate.find.eleMatch.008
*@Author:  2019-6-6  wangkexin
*@testlinkCase: seqDB-5248
********************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_cl_5248";

   //clean environment before test
   commDropCL( db, csName, clName, true, true, "drop CL in the beginning." );

   var cl = commCreateCL( db, csName, clName );
   cl.insert( { a: { a: { a: { a: { a: 1 } } } } } );
   var rc = cl.find( { "a": { $elemMatch: { a: { $elemMatch: { a: { $elemMatch: { a: { $elemMatch: { a: 1 } } } } } } } } } );
   var expRecs = [{ a: { a: { a: { a: { a: 1 } } } } }];
   checkRec( rc, expRecs );

   //drop cl
   commDropCL( db, csName, clName, true, true, "drop cl in the end" );
}