/************************************************************************
*@Description:    seqDB-5542:导入数据成功后对数据做基本操作，并切分
*@Author:           2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var groupsArray = getDataGroupsName();
   if( groupsArray.length < 2 )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5542";
   var optObj = { ShardingKey: { a: 1 }, ShardingType: "range", Group: groupsArray[0], ReplSize: 0 };
   var cl = readyCL( csName, clName, optObj );

   var imprtFile = tmpFileDir + "5542.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );
   splitOper( cl, groupsArray );

   checkCLData( csName, clName, groupsArray );
   cleanCL( csName, clName );

}

function splitOper ( cl, groupsArray )
{

   cl.split( groupsArray[0], groupsArray[1], { a: 50 }, { a: 100 } );
}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   for( i = 0; i < 100; i++ )
   {
      file.write( i + "\n" );
   }
   /*
   var fileInfo = cmd.run( "cat "+ imprtFile );
   */
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "a int"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );
   var rcObj = rc.split( "\n" );

   var expParseRecords = "Parsed records: 100";
   var expImportedRecords = "Imported records: 100";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }
}

function checkCLData ( csName, clName, groupsArray )
{

   var cl = db.getCS( csName ).getCL( clName );
   var rc = cl.find( {}, { _id: { $include: 0 } } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 100;
   var actCnt = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + "]" +
         "[cnt:" + actCnt + "]" );
   }

   var srcAddr = db.getRG( groupsArray[0] ).getMaster().toString();
   var actCnt1 = new Sdb( srcAddr ).getCS( csName ).getCL( clName ).find( { a: { $lt: 50 } } ).count();
   var expCnt1 = 50;
   if( Number( actCnt1 ) !== expCnt1 )
   {
      throw new Error( "check source group data", null, "[find.count]",
         "[srcDataCnt:" + expCnt1 + "]" +
         "[srcDataCnt:" + actCnt1 + "]" );
   }

   var trgAddr = db.getRG( groupsArray[1] ).getMaster().toString();
   var actCnt2 = new Sdb( trgAddr ).getCS( csName ).getCL( clName ).find( { a: { $gte: 50 } } ).count();
   var expCnt2 = 50;
   if( Number( actCnt2 ) !== expCnt2 )
   {
      throw new Error( "check target group data", null, "[find.count]",
         "[trgDataCnt:" + expCnt2 + "]" +
         "[trgDataCnt:" + actCnt2 + "]" );
   }
}