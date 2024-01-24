/******************************************************
 * @Description: seqDB-6651:sdbimprt批量导入数据
 * @Author: linsuqiang 
 * @Date: 2016-12-29
 ******************************************************/
testConf.skipStandAlone = true;

main( test );

function test()
{
   var clName = COMMCLNAME + "_6651";
   var groupName = commGetGroups ( db )[0][0].GroupName;

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL ( db, COMMCSNAME, clName, { Compressed: true, CompressionType: "lzw", Group: groupName } );
   
   var ranStr = getRandomString();
   prepareData( ranStr );
   importData( COMMCSNAME, clName, "6651_1.csv" );
   checkDictCreated( COMMCSNAME, clName );

   importData( COMMCSNAME, clName, "6651_2.csv" );
   checkCompressed( COMMCSNAME, clName );
   checkCLData( cl, ranStr );

   commDropCL( db, COMMCSNAME, clName, false, false );
}

function prepareData ( ranStr )
{
   // records for creating dictionary
   var imprtFile = tmpFileDir + "6651_1.csv";
   var file = fileInit( imprtFile );
   var headline = "a int, ran string\n";
   file.write( headline );
   for( i = 0; i < 150; i++ )
   {
      var rec = i + "," + ranStr + i + "\n";
      file.write( rec );
   }
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();

   // records for testing compression
   var imprtFile = tmpFileDir + "6651_2.csv";
   var file = fileInit( imprtFile );
   var headline = "a int, ran string\n";
   file.write( headline );
   for( i = 150; i < 160; i++ )
   {
      var rec = i + "," + ranStr + i + "\n";
      file.write( rec );
   }
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function getRandomString ()
{
   var str = "";
   var base = ['a', 'b', 'c'];
   var length = 512 * 1024;
   for( i = 0; i < length; i++ )
   {
      pos = Math.round( Math.random() * ( base.length - 1 ) );
      str += base[pos];
   }
   return str;
}

function importData ( csName, clName, imprtFile )
{
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv'
      + ' --headerline=true'
      + ' --file ' + tmpFileDir + imprtFile;
   println( imprtOption );
   var rc = cmd.run( imprtOption );
   println( rc );

   var rcObj = rc.split( "\n" );
   if( imprtFile === "6651_1.csv" )
   {
      var expParseRecords = "parsed records: 150";
      var expImportedRecords = "imported records: 150";
   }
   else if( imprtFile === "6651_2.csv" )
   {
      var expParseRecords = "parsed records: 10";
      var expImportedRecords = "imported records: 10";
   }
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "[" + expParseRecords + ", " + expImportedRecords + "]" + "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }
}

function checkDictCreated ( csName, clName )
{
   var waitSec = 180;
   var currSec = 0;
   var created = false;
   for( currSec = 0; currSec < waitSec; currSec++ )
   {
      // connect to cl data node
      var groupNameArray = getDataGroupsName();
      var clGroupName = groupNameArray[0];
      var dataGroup = db.getRG( clGroupName );
      var url = dataGroup.getMaster();
      var dataDB = new Sdb( url );
      // get details of snapshot
      var snapshot = dataDB.snapshot( 4, { Name: csName + "." + clName } );
      var detail = snapshot.next().toObj().Details[0];
      // check whether dictionary is created
      if( detail.DictionaryCreated === true )
      {
         created = true;
         break;
      }
      sleep( 1000 ); // try again after 1 second
   }
   if( !created )
   {
      throw new Error( " Dictionary is not created!" );
   }
}

function checkCompressed ( csName, clName )
{
   var tryTimes = 10;
   var ratioRight = false;
   var attrRight = false;
   var typeRight = false;
   var actRes = "";
   for( i = 0; i < tryTimes; i++ )
   {
      // connect to cl data node
      var groupNameArray = getDataGroupsName();
      var clGroupName = groupNameArray[0];
      var dataGroup = db.getRG( clGroupName );
      var url = dataGroup.getMaster();
      var dataDB = new Sdb( url );
      // get details of snapshot
      var snapshot = dataDB.snapshot( 4, { Name: csName + "." + clName } );
      var detail = snapshot.next().toObj().Details[0];
      // check whether data is compressed
      actRes = "";
      if( detail.CurrentCompressionRatio < 1 )
      {
         ratioRight = true;
         actRes += "CompressionRatio: " + detail.CurrentCompressionRatio + "\n";
      }
      if( detail.Attribute === "Compressed" )
      {
         attrRight = true;
         actRes += "Attribute: " + detail.Attribute + "\n";
      }
      if( detail.CompressionType === "lzw" )
      {
         typeRight = true;
         actRes += "CompressionType: " + detail.CompressionType + "\n";
      }
      if( ratioRight && attrRight && typeRight ) break;
      sleep( 1000 ); // try again after 1 second
   }
   if( !( ratioRight && attrRight && typeRight ) )
   {
      var expRes = "";
      expRes += "CompressionRatio: <1\n";
      expRes += "Attribute: Compressed\n";
      expRes += "CompressionType: lzw\n";
      throw new Error( "expRes: " + expRes + ", actRes: " + actRes );
   }
}

function checkCLData ( cl, ranStr )
{
   println( "\n---Begin to check cl data." );

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 160;
   var actCnt = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw new Error( "expCnt: " + expCnt + ", actCnt: " + actCnt );
   }

   for( i = 0; i < recsArray.length; i++ )
   {
      expRec = { a: i, ran: ranStr + i };
      if( JSON.stringify( recsArray[i] ) !== JSON.stringify( expRec ) )
      {  // show field 'a' only, because value of field 'ranStr' is too long!
         throw new Error( "expRec.a: " + expRec.a + ", actRec.a: " + recsArray[i].a );
      }
   }
}
