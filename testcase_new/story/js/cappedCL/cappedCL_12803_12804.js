/************************************
*@Description: 修改固定集合为普通集合，修改参数为开启压缩/AutoSplit/AutoIndexId  
*@author:      liuxiaoxuan
*@createdate:  2017.9.30
*@testlinkCase:seqDB-12803/seqDB-12804
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "12803_12804";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCL( db, COMMCAPPEDCSNAME, clName, clOption, false, true );

   //check alter cappedCL 
   var alterOption1 = { Capped: false };
   checkAlterResult( dbcl, alterOption1, -32 );
   var alterOption2 = { Compressed: true };
   checkAlterResult( dbcl, alterOption2, -32 );
   var alterOption3 = { AutoIndexId: true };
   checkAlterResult( dbcl, alterOption3, -32 );
   var alterOption4 = { AutoSplit: true };
   checkAlterResult( dbcl, alterOption4, -32 );

   //create commonCS and commonCL
   var commonCLName = COMMCLNAME + "12803_12804";
   var dbcl = commCreateCL( db, COMMCSNAME, commonCLName );

   //check alter commonCL 
   var alterOption5 = { Capped: true };
   checkAlterResult( dbcl, alterOption5, -32 );
   var alterOption6 = { Size: 1024 };
   checkAlterResult( dbcl, alterOption6, -32 );
   var alterOption7 = { Max: 10000000 };
   checkAlterResult( dbcl, alterOption7, -32 );
   var alterOption8 = { OverWrite: true };
   checkAlterResult( dbcl, alterOption8, -32 );

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
   commDropCL( db, COMMCSNAME, commonCLName, true, true, "drop CL in the end" );
}

function checkAlterResult ( dbcl, options, expectErrorCode )
{
   assert.tryThrow( expectErrorCode, function()
   {
      dbcl.alter( options );
   } );
}
