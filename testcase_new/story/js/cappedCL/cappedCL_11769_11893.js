/************************************
*@Description: 创建固定集合，指定AutoIndexId ,压缩的相关参数
*@author:      luweikang
*@createdate:  2017.7.6
*@testlinkCase:seqDB-11769,seqDB-11893
**************************************/

main( test );
function test ()
{
   var clName = COMMCAPPEDCLNAME + "_11769_11893_";

   //create cappedCL AutoIndexId:true
   var optionObj1 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: true };
   createCappedCL( db, COMMCAPPEDCSNAME, clName + 1, optionObj1 );

   //create cappedCL Compressed:true
   var optionObj2 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: true, Compressed: true };
   createCappedCL( db, COMMCAPPEDCSNAME, clName + 2, optionObj2 );

   //create cappedCL Compressed:false
   var optionObj3 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: true, Compressed: false };
   createCappedCL( db, COMMCAPPEDCSNAME, clName + 3, optionObj3 );

   //create cappedCL CompressionType:"snappy"
   var optionObj4 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: true, Compressed: true, CompressionType: "snappy" };
   createCappedCL( db, COMMCAPPEDCSNAME, clName + 4, optionObj4 );

   //create cappedCL CompressionType:"lzw"
   var optionObj5 = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: true, Compressed: true, CompressionType: "lzw" };
   createCappedCL( db, COMMCAPPEDCSNAME, clName + 5, optionObj5 );

   for( var i = 1; i <= 5; i++ )
   {
      commDropCL( db, COMMCAPPEDCSNAME, clName + i, true, true, "drop CL in the end" );
   }
}

function createCappedCL ( db, csName, clName, optionObj )
{
   try
   {
      db.getCS( csName ).createCL( clName, optionObj );
      if( optionObj.Compressed !== false )
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