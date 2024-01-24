/************************************
*@Description: 在普通集合空间上创建与固定集合空间上同名的普通集合
               在固定集合空间上创建与普通集合空间上同名的固定集合
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11840,seqDB-11841
**************************************/

main( test );
function test ()
{
   var normal_csName = COMMCSNAME + "_11840";
   var capped_csName = COMMCAPPEDCSNAME + "_11841";

   var clName1 = COMMCLNAME + "_11840_11841";
   var clName2 = COMMCAPPEDCLNAME + "_11840_11841";

   //drop CS and create cappedCS
   commDropCS( db, normal_csName, true, "drop CS in the beginning" );
   commCreateCS( db, normal_csName, false, "beginning to create CS", null );
   initCappedCS( capped_csName );

   //normalCS quick cappedCS
   db.getCS( normal_csName ).createCL( clName1 );
   db.getCS( capped_csName ).createCL( clName1, { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false } );

   //cappedCS quick normalCS
   db.getCS( capped_csName ).createCL( clName2, { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false } );
   db.getCS( normal_csName ).createCL( clName2 );

   //clean environment after test
   commDropCS( db, normal_csName, true, "drop CS in the end" );
   commDropCS( db, capped_csName, true, "drop CS in the end" );
}