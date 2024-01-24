/************************************
*@Description: 创建固定集合，参数校验
*@author:      luweikang
*@createdate:  2017.7.11
*@testlinkCase:seqDB-11822
**************************************/

main( test );
function test ()
{
   var clName1 = COMMCAPPEDCLNAME + "_11822_CL1";
   var clName2 = COMMCAPPEDCLNAME + "_11822_CL2";
   var clName3 = COMMCAPPEDCLNAME + "_11822_CL3";
   var clName4 = COMMCAPPEDCLNAME + "_11822_CL4";
   var clName5 = COMMCAPPEDCLNAME + "_11822_CL5";
   var clName6 = COMMCAPPEDCLNAME + "_11822_CL6";
   var clName7 = COMMCAPPEDCLNAME + "_11822_CL7";

   //check cappedCL Capped

   //check Capped : true
   var options1 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName1, options1, true );

   //check Capped : false
   var options2 = { Capped: false, Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName2, options2, false );

   //check Capped
   var options3 = { Capped: "", Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName3, options3, false );

   //check Capped : "abc"
   var options4 = { Capped: "abc", Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName4, options4, false );

   //check options = ""
   var options5 = { Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName5, options5, false );

   //check Caped
   var options6 = { Caped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName6, options6, false );

   //check no Capped
   var options7 = { Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName7, options7, false );

   //clean environment after test  
   for( var i = 1; i <= 7; i++ )
   {
      commDropCL( db, COMMCAPPEDCSNAME, COMMCAPPEDCLNAME + "_11822_CL" + i, true, true, "drop CL in the end" );
   }
}

function checkCreateCLOptions ( csName, clName, options, result )
{
   try
   {
      db.getCS( csName ).createCL( clName, options );
      assert.equal( result, true );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG && e.message != SDB_OPERATION_INCOMPATIBLE )
      {
         throw e;
      }
   }
}
