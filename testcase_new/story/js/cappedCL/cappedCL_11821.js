/************************************
*@Description: 创建固定集合空间，参数校验
*@author:      luweikang
*@createdate:  2017.7.7
*@testlinkCase:seqDB-11821
**************************************/

main( test );
function test ()
{
   var csName1 = COMMCAPPEDCSNAME + "_11821_CS1";
   var csName2 = COMMCAPPEDCSNAME + "_11821_CS2";
   var csName3 = COMMCAPPEDCSNAME + "_11821_CS3";
   var csName4 = COMMCAPPEDCSNAME + "_11821_CS4";
   var csName5 = COMMCAPPEDCSNAME + "_11821_CS5";
   var csName6 = COMMCAPPEDCSNAME + "_11821_CS6";

   //clean cs before test
   commDropCS( db, csName1, true, "drop CS in the end" );
   commDropCS( db, csName2, true, "drop CS in the end" );
   commDropCS( db, csName3, true, "drop CS in the end" );
   commDropCS( db, csName4, true, "drop CS in the end" );
   commDropCS( db, csName5, true, "drop CS in the end" );
   commDropCS( db, csName6, true, "drop CS in the end" );

   //check cappedCS options

   //check Capped : true
   var options1 = { Capped: true };
   checkCreateCSOptions( csName1, options1, true );

   //check Capped : false
   var options2 = { Capped: false };
   checkCreateCSOptions( csName2, options2, true );

   //check Capped : "",集群下会报-6错误，单机不会报错
   var options3 = { Capped: "" };
   checkCreateCSOptions( csName3, options3, true );

   //check Capped : "abc"
   var options4 = { Capped: "abc" };
   checkCreateCSOptions( csName4, options4, true );

   //check options = ""
   var options5 = {};
   checkCreateCSOptions( csName5, options5, true );

   //check Caped,集群下会报-6错误，单机不会报错
   var options6 = { Caped: true };
   checkCreateCSOptions( csName6, options6, true );

}

function checkCreateCSOptions ( csName, options, result )
{
   try
   {
      db.createCS( csName, options );
      assert.equal( result, true );
      commDropCS( db, csName, true, "drop CS in the end" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}