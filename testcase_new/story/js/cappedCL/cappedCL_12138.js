/************************************
*@Description: 创建固定集合空间集合，参数OverWrite校验
*@author:      luweikang
*@createdate:  2017.7.14
*@testlinkCase:seqDB-12138
**************************************/

main( test );
function test ()
{
   var clName1 = COMMCAPPEDCLNAME + "_12138_CL1";
   var clName2 = COMMCAPPEDCLNAME + "_12138_CL2";
   var clName3 = COMMCAPPEDCLNAME + "_12138_CL3";
   var clName4 = COMMCAPPEDCLNAME + "_12138_CL4";
   var clName5 = COMMCAPPEDCLNAME + "_12138_CL5";
   var clName6 = COMMCAPPEDCLNAME + "_12138_CL6";
   var clName7 = COMMCAPPEDCLNAME + "_12138_CL7";

   //check cappedCL Capped

   //check OverWrite : true
   var options1 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OverWrite: true };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName1, options1, true );

   //check OverWrite : 1
   var options2 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OverWrite: 1 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName2, options2, false );

   //check OverWrite : 0
   var options3 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OverWrite: 0 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName3, options3, false );

   //check OverWrite : false
   var options4 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OverWrite: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName4, options4, true );

   //check OverWrite : "abc"
   var options5 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OverWrite: "abc" };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName5, options5, false );

   //check OverWrite:123.456
   var options6 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OverWrite: 123.456 };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName6, options6, false );

   //check OvWre:true,单机模式下不会报错
   var options7 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false, OvWre: true };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName7, options7, true );

   //clean environment after test  
   for( var i = 1; i <= 7; i++ )
   {
      commDropCL( db, COMMCAPPEDCSNAME, COMMCAPPEDCLNAME + "_12138_CL" + i, true, true, "drop CL in the end" );
   }
}

function checkCreateCLOptions ( csName, clName, options, result )
{
   try
   {
      db.getCS( csName ).createCL( clName, options );
      if( result !== true )
      {
         throw new Error( "ERR_CREATE_CAPPEDCL" );
      }
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}