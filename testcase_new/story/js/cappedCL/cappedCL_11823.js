/************************************
*@Description: �����̶����ϣ�����SizeУ��
*@author:      luweikang
*@createdate:  2017.7.11
*@testlinkCase:seqDB-11823
**************************************/

main( test );
function test ()
{
   var clName1 = COMMCAPPEDCLNAME + "_11823_CL1";
   var clName2 = COMMCAPPEDCLNAME + "_11823_CL2";
   var clName3 = COMMCAPPEDCLNAME + "_11823_CL3";
   var clName4 = COMMCAPPEDCLNAME + "_11823_CL4";
   var clName5 = COMMCAPPEDCLNAME + "_11823_CL5";
   var clName6 = COMMCAPPEDCLNAME + "_11823_CL6";
   var clName7 = COMMCAPPEDCLNAME + "_11823_CL7";
   var clName8 = COMMCAPPEDCLNAME + "_11823_CL8";
   var clName9 = COMMCAPPEDCLNAME + "_11823_CL9";

   //check cappedCL Capped

   //check Size : 33554432
   var options1 = { Capped: true, Size: 32, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName1, options1, true, 33554432 );

   //check Size : 8796093022208
   var options2 = { Capped: true, Size: 8388608, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName2, options2, true, 8796093022208 );

   //check Size : 12.456
   var options3 = { Capped: true, Size: 12.456, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName3, options3, true, 33554432 );

   //check Size : 123.456
   var options9 = { Capped: true, Size: 123.456, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName9, options9, true, 134217728 );

   //check Size : 8796093022209
   var options4 = { Capped: true, Size: 8388609, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName4, options4, false );

   //check Size : "abc"
   var options5 = { Capped: true, Size: "abc", Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName5, options5, false );

   //check Size
   var options6 = { Capped: true, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName6, options6, false );

   //check Size : -1
   var options7 = { Capped: true, Size: -1, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName7, options7, false );

   //check Sie : -1
   var options8 = { Capped: true, Sie: 32, Max: 10000000, AutoIndexId: false };
   checkCreateCLOptions( COMMCAPPEDCSNAME, clName8, options8, false );

   //clean environment after test  
   for( var i = 1; i <= 9; i++ )
   {
      commDropCL( db, COMMCAPPEDCSNAME, COMMCAPPEDCLNAME + "_11823_CL" + i, true, true, "drop CL in the end" );
   }
}

function checkCreateCLOptions ( csName, clName, options, result, expectSize )
{
   if( typeof ( expectSize ) == "undefined" ) { expectSize = 0; }
   try
   {
      db.getCS( csName ).createCL( clName, options );
      assert.equal( result, true );

      if( true !== commIsStandalone( db ) )
      {
         var cursor = db.snapshot( 8, { Name: csName + "." + clName } );
         while( cursor.next() )
         {
            var size = cursor.current().toObj().Size;
            assert.equal( size, expectSize );
         }
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
