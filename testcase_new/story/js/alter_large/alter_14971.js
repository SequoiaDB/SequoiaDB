/* *****************************************************************************
@discretion: cl alter compressionType from snappy to lzw, the test scenario is as follows:
test a: cl no records
test b: cl exists records
@author��2018-4-25 wuyan  Init
***************************************************************************** */
var clName1 = CHANGEDPREFIX + "_alterclcompression_14971a";
var clName2 = CHANGEDPREFIX + "_alterclcompression_14971b";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName1, true, true, "drop CL in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1, { Compressed: true, CompressionType: "snappy" } );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2, { Compressed: true, CompressionType: "snappy" } );

   //test a: cl no records, alter compressiontype is lzw
   dbcl1.setAttributes( { CompressionType: "lzw" } );
   checkAlterResult( clName1, "CompressionType", 1 );
   checkAlterResult( clName1, "CompressionTypeDesc", "lzw" );

   //test b: cl exist records, alter compressiontype is lzw
   insertRecs( dbcl2 );
   dbcl2.setAttributes( { CompressionType: "lzw" } );

   var insertRecsNum = 200000;
   insertBuildDictionaryRecs( dbcl2, insertRecsNum );

   //get random 10 records to check data
   var checkRecsNum = 10; //get random 3 records
   checkRecords( dbcl2, insertRecsNum, checkRecsNum );

   //check the compression fields
   checkAlterResult( clName2, "CompressionType", 1 );
   checkAlterResult( clName2, "CompressionTypeDesc", "lzw" );
   checkResultByDataNode( clName2 );

   //clean
   commDropCL( db, COMMCSNAME, clName1, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, clName2, true, true, "clear collection in the beginning" );
}

function insertBuildDictionaryRecs ( cl, insertRecsNum )
{

   for( k = 100; k < insertRecsNum; k += 20000 )
   {
      var doc = [];
      for( i = 0 + k; i < 20000 + k; i++ )
      {
         doc.push( { total_account: i, account_id: i, tx_number: "testR" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/20160101_1.png.png", "text": "paddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingData" } );
      }
      cl.insert( doc );
   }
}

function insertRecs ( cl )
{
   var expRecs = [];
   for( var i = 0; i < 100; i++ )
   {
      var rec = { total_account: i, account_id: i, tx_number: "test" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" };
      expRecs.push( rec );
   }
   cl.insert( expRecs );
}

function checkRecords ( cl, insertRecsNum, checkRecsNum )
{
   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      if( i < 100 )
      {
         var recsCnt = cl.find( { total_account: i, account_id: i, tx_number: "test" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" } ).count();
      } else
      {
         var recsCnt = cl.find( { total_account: i, account_id: i, tx_number: "testR" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/20160101_1.png.png", "text": "paddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingDatapaddingData" } ).count();
      }

      var expctCnt = 1;
      assert.equal( parseInt( recsCnt ), expctCnt );
   }
}

function checkResultByDataNode ( clName )
{
   var clGroup = getSrcGroup( COMMCSNAME, clName );
   var rg = db.getRG( clGroup );

   var sdb = new Sdb( rg.getMaster() );
   var clFullName = COMMCSNAME + "." + clName;
   var cur = sdb.snapshot( 4, { "Name": clFullName } );
   var tmpcur = cur.current().toObj()["Details"][0];

   var actAttribute = tmpcur.Attribute;
   var actCompressionType = tmpcur.CompressionType;
   var actDictionaryCreated = tmpcur.DictionaryCreated;

   var sleepInteval = 10;
   var sleepDuration = 0;
   var maxSleepDuration = 30000;
   while( actDictionaryCreated != true && sleepDuration < maxSleepDuration )
   {
      sleep( sleepInteval );
      sleepDuration += sleepInteval;
      var cur1 = sdb.snapshot( 4, { "Name": clFullName } ).current().toObj()["Details"][0];
      var actDictionaryCreated = cur1.DictionaryCreated;
   }
   assert.equal( actAttribute, "Compressed" );
   assert.equal( actCompressionType, "lzw" );
   assert.equal( actDictionaryCreated, true );
}

function getSrcGroup ( csName, clName )
{
   var clFullName = csName + "." + clName;
   var clInfo = db.snapshot( 8, { Name: clFullName } );
   while( clInfo.next() )
   {
      var clInfoObj = clInfo.current().toObj();
      var srcGroupName = clInfoObj.CataInfo[0].GroupName;
   }

   return srcGroupName;
}