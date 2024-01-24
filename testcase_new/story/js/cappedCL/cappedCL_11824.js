/************************************
*@Description: 创建固定集合空间集合，参数Max校验
*@author:      luweikang
*@createdate:  2017.7.11
*@testlinkCase:seqDB-11824
**************************************/

main( test );
function test ()
{
   var clName1 = COMMCAPPEDCLNAME + "_11824_CL1";
   var clName2 = COMMCAPPEDCLNAME + "_11824_CL2";
   var clName3 = COMMCAPPEDCLNAME + "_11824_CL3";
   var clName4 = COMMCAPPEDCLNAME + "_11824_CL4";
   var clName5 = COMMCAPPEDCLNAME + "_11824_CL5";
   var clName6 = COMMCAPPEDCLNAME + "_11824_CL6";
   var clName7 = COMMCAPPEDCLNAME + "_11824_CL7";

   //check cappedCL Capped

   //check Max : 10000000
   var options1 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName1, options1, true );

   //check Max : -1
   var options2 = { Capped: true, Size: 1024, Max: -1, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName2, options2, false );

   //check Max : 0
   var options3 = { Capped: true, Size: 1024, Max: 0, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName3, options3, true );

   //check Max : 132.456,单机创建成功
   var options4 = { Capped: true, Size: 1024, Max: 123.456, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName4, options4, true );

   //check Max : "abc"
   var options5 = { Capped: true, Size: 1024, Max: "abc", AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName5, options5, false );

   //check 不指定Max,单机创建成功
   var options6 = { Capped: true, Size: 1024, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName6, options6, true );

   //check Mx,单机创建成功
   var options7 = { Capped: true, Size: 1024, Mx: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName7, options7, true );

   //clean environment after test  
   for( var i = 1; i <= 7; i++ )
   {
      commDropCL( db, COMMCAPPEDCSNAME, COMMCAPPEDCLNAME + "_11824_CL" + i, true, true, "drop CL in the end" );
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