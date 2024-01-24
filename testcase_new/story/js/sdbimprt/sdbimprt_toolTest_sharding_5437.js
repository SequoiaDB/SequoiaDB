/************************************************************************
*@Description:   seqDB-5437:指定--sharding导入csv格式数据
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dataGroupNames = commGetDataGroupNames( db );
   if( dataGroupNames.length < 2 )
   {
      return;
   }

   var srcRG = dataGroupNames[0];
   var dstRG = dataGroupNames[1];
   var csName = COMMCSNAME;
   var recsNum = 1000;
   var expRecs = new Array();
   var imprtFile = tmpFileDir + "5437.csv";
   readyData( imprtFile, recsNum, expRecs );

   // range cl, --sharding true
   var clName1 = COMMCLNAME + "_5437_range";
   var opt1 = { "ShardingKey": { "a": 1 }, "ShardingType": "range", "Group": srcRG };
   var cl1 = readyCL( csName, clName1, opt1, "range cl" );

   cl1.insert( { "a": recsNum / 2 } );
   cl1.split( srcRG, dstRG, 50 );
   cl1.remove();

   importData( csName, clName1, recsNum, imprtFile, true );
   checkCLData( cl1, recsNum, expRecs );

   // hash cl, --sharding true
   var clName2 = COMMCLNAME + "_5437_hash";
   var opt2 = { "ShardingKey": { "a": 1 }, "ShardingType": "hash", "Group": srcRG };
   var cl2 = readyCL( csName, clName2, opt2, "hash cl" );
   cl2.split( srcRG, dstRG, 50 );

   importData( csName, clName2, recsNum, imprtFile, true );
   checkCLData( cl2, recsNum, expRecs );

   // --sharding false
   cl1.remove();
   importData( csName, clName1, recsNum, imprtFile, false );
   checkCLData( cl1, recsNum, expRecs );

   // clean
   cleanCL( csName, clName1 );
   cleanCL( csName, clName2 );
   cmd.run( "rm " + imprtFile );
}

function readyData ( imprtFile, recsNum, expRecs )
{
   // sharding value array, value random sort
   // ready sharding value array
   var tmpArray = new Array();
   for( var i = 0; i < recsNum; i++ )
   {
      tmpArray.push( i );
   }
   // random sort
   tmpArray.sort(
      function() 
      {
         return Math.random() > 0.5 ? -1 : 1;
      }
   );

   // ready data for import file
   var str = "a int, b int";
   for( var i = 0; i < recsNum; i++ )
   {
      str = str + "\n" + tmpArray[i] + "," + tmpArray[i];

      expRecs.push( { "a": i, "b": i } );
   }
   var file = fileInit( imprtFile );
   file.write( str );
   file.close();
}

function importData ( csName, clName, recsNum, imprtFile, isSharding )
{
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --sharding ' + isSharding
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: " + recsNum;
   var expImportedRecords = "Imported records: " + recsNum;
   var expImportFailure = "Imported failure: 0";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   var actImportFailure = rcObj[5];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords
      || expImportFailure !== actImportFailure )
   {
      throw new Error( expParseRecords + ", " + expImportedRecords + ", " + expImportFailure + "\n"
         + actParseRecords + ", " + actImportedRecords + ", " + actImportFailure );
   }
}

function checkCLData ( cl, recsNum, expRecs )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== recsNum || actRecs !== JSON.stringify( expRecs ) )
   {
      throw new Error( "expCnt: " + recsNum + ", expRecs: " + JSON.stringify( expRecs ) + "\n"
         + "actCnt: " + actCnt + ", actRecs: " + actRecs );
   }
}
