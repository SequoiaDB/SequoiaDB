/***************************************************************************
@Description :seqDB-11979 :集合属性不影响固定集合 
@Modify list :
              2018-10-26  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11979";
   dropCL( db, COMMCSNAME, clName, true, true );

   var groups = commGetGroups( db );
   var arrayGroup = new Array();
   for( var i in groups )
   {
      arrayGroup.push( groups[i][0]["GroupName"] );
   }

   if( arrayGroup.length <= 0 )
   {
      throw new Error( "commGetGroups()", "commGetGroups", "can not get groups ", "success", "fail" );
   }
   //指定集合的replSize、group、AutoIndexId、压缩为非默认值
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 2, Group: arrayGroup[0], AutoIndexId: false, Compressed: true, CompressionType: "lzw" } );

   var textIndexName = "a_11979";
   commCreateIndex( dbcl, textIndexName, { content: "text" } );

   //固定集合属性为默认值(与原集合属性无关)
   var dbOperator = new DBOperator();
   var cappedCLName = dbOperator.getCappedCLName( dbcl, textIndexName );
   var cappedDB = db.getRG( arrayGroup[0] ).getMaster().connect();
   var cappedAttr = cappedDB.snapshot( 4, { Name: cappedCLName + "." + cappedCLName } );
   var cappedAttr = cappedAttr.next().toObj();

   var expCappedAttr = { Attribute: "NoIDIndex | Capped", CompressionType: "", Status: "Normal", Indexes: 0 };
   var actCappedAttr = {
      Attribute: cappedAttr["Details"][0]["Attribute"], CompressionType: cappedAttr["Details"][0]["CompressionType"],
      Status: cappedAttr["Details"][0]["Status"], Indexes: cappedAttr["Details"][0]["Indexes"]
   };

   for( var i in expCappedAttr )
   {
      if( expCappedAttr[i] != actCappedAttr[i] )
      {
         throw new Error( "expect attr: " + expCappedAttr[i] + ",expect attr: " + actCappedAttr[i] );
      }
   }

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
