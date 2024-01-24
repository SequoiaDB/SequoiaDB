/************************************
*@Description: createCL固定集合相关参数名校验
*@author:      liuxiaoxuan
*@createdate:  2019.7.17
*@testlinkCase:seqDB-11825
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName1 = COMMCAPPEDCLNAME + "_11825_CL1";
   var clName2 = COMMCAPPEDCLNAME + "_11825_CL2";
   var clName3 = COMMCAPPEDCLNAME + "_11825_CL3";
   var clName4 = COMMCAPPEDCLNAME + "_11825_CL4";

   //check invalid Capped 
   var options1 = { Capp: true, Size: 1, Max: 1, AutoIndexId: false };
   checkCreateCLInvalid( COMMCAPPEDCSNAME, clName1, options1 );

   //check invalid Max 
   var options2 = { Capped: true, Size: 1, Ma: 1, AutoIndexId: false };
   checkCreateCLInvalid( COMMCAPPEDCSNAME, clName2, options2 );

   //check invalid Size 
   var options3 = { Capped: true, Siz: 1, Max: 1, AutoIndexId: false };
   checkCreateCLInvalid( COMMCAPPEDCSNAME, clName3, options3 );

   //check invalid AutoIndexId 
   var options4 = { Capped: true, Size: 1, Max: 1, AutoIndex: false };
   checkCreateCLInvalid( COMMCAPPEDCSNAME, clName4, options4 );

   for( var i = 1; i <= 4; i++ )
   {
      commDropCL( db, COMMCAPPEDCSNAME, COMMCAPPEDCLNAME + "_11825_CL" + i, true, true, "drop CL in the end" );
   }
}

function checkCreateCLInvalid ( csName, clName, options )
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getCS( csName ).createCL( clName, options );
   } );
}