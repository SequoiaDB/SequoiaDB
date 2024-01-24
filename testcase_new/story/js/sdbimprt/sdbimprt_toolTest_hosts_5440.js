/************************************************************************
*@Description:    seqDB-5440:连接多个coord并发导入数据，指定并发数大于连接指定的coord数
                  seqDB-5445
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5440";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5440.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "a int,b int\n1,1\n2,2\n3,3\n4,4" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   //get coord address
   var coordAddrs = String( getCoordAdrr() );

   var imprtOption = installDir + 'bin/sdbimprt'
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --hosts "' + coordAddrs
      + '" -n 1 -j 8 -v'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import operation results
   var parIndex = rc.indexOf("Parsed records: ");
   var impIndex = rc.indexOf("Imported records: "); 
   var expParseRecords = "Parsed records: 4";
   var expImportedRecords = "Imported records: 4";
   var actParseRecords = "Parsed records: " + rc[parIndex + 16];
   var actImportedRecords = "Imported records: " + rc [impIndex + 18];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 4;
   var expRecs = '[{"a":1,"b":1},{"a":2,"b":2},{"a":3,"b":3},{"a":4,"b":4}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}

function getCoordAdrr ()
{
   var nodeArray = [];
   var tmpInfo = db.listReplicaGroups().toArray();
   for( var i = 0; i < tmpInfo.length; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" );
      if( tmpObj.GroupName == "SYSCoord" )
      {
         var tmpGroupObj = tmpObj.Group;
         for( var j = 0; j < tmpGroupObj.length; ++j )
         {
            var tmpNodeObj = tmpGroupObj[j];
            nodeArray.push( tmpNodeObj.HostName + ":" + tmpNodeObj.Service[0].Name );
         }
      }
   }
   return nodeArray;
}