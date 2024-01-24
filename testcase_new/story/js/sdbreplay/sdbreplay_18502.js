/************************************************************************
*@Description: seqDB-18502: 配置filePrefix和fileSuffix，值不为空  
*@Author: 2019-7-4  xiaoni zhao init
************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = "clName_18502";
   var groupNames = getDataGroupNames();

   var cl = readyCL( csName, clName, { Group: groupNames[0] } );

   var cursor = db.list( SDB_SNAP_SYSTEM, { GroupName: groupNames[0] } );
   var svcName = cursor.current().toObj().Group[0].Service[0].Name;
   cursor = db.snapshot( 6, { ServiceName: svcName, RawData: true, IsPrimary: true } );
   var minLSN = cursor.current().toObj().CompleteLSN;

   var expDataArr = [];
   for( var i = 0; i < 100; i++ )
   {
      cl.insert( { a: i } );
      cl.insert( { a: 100 + i } );
      cl.update( { $set: { a: 200 + i } }, { a: i } );
      cl.remove( { a: 100 + i } );

      expDataArr.push( '"I","' + i + '"' );
      expDataArr.push( '"I","' + ( 100 + i ) + '"' );
      expDataArr.push( '"B","' + i + '"' );
      expDataArr.push( '"A","' + ( 200 + i ) + '"' );
      expDataArr.push( '"D","' + ( 100 + i ) + '"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      //配置filePrefix和fileSuffix为普通字符串
      var filePrefix = "start";
      var fileSuffix = "end";
      var fieldType = "MAPPING_INT";
      var clNameArr = [csName + "." + clName];
      var filter = filter = '\'{CL: ["' + clNameArr + '"], MinLSN: ' + minLSN + ' }\'';
      getOutputConfFile( groupNames[0], csName, clName );
      configOutputFile( rtCmd, csName, clName, filePrefix, fileSuffix, fieldType );
      execSdbReplay( rtCmd, groupNames[0], clNameArr, undefined, undefined, undefined, undefined, undefined, filter );
      checkCsvFile( rtCmd, filePrefix, expDataArr );
      cleanFile( rtCmd );

      //配置filePrefix和fileSuffix为特殊字符串
      filePrefix = "$";
      fileSuffix = ".";
      getOutputConfFile( groupNames[0], csName, clName );
      configOutputFile( rtCmd, csName, clName, filePrefix, fileSuffix, fieldType );
      execSdbReplay( rtCmd, groupNames[0], clNameArr, undefined, undefined, undefined, undefined, undefined, filter );
      checkCsvFile( rtCmd, filePrefix, expDataArr );

      cleanCL( csName, clName );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}

function configOutputFile ( rtCmd, csName, clName, filePrefix, fileSuffix, fieldType )
{
   var fullCLName = csName + "." + clName;
   var outputConfFilePath = tmpFileDir + fullCLName + ".conf";
   rtCmd.run( "sed -i 's/filePrefix_ori/" + filePrefix + "/g' " + outputConfFilePath );
   rtCmd.run( "sed -i 's/fileSuffix: \"\"/fileSuffix: \"" + fileSuffix + "\"/g' " + outputConfFilePath );
   rtCmd.run( "sed -i 's/source_fullCLName_ori/" + fullCLName + "/g' " + outputConfFilePath );
   rtCmd.run( "sed -i 's/target_fullCLName_ori/" + fullCLName + "/g' " + outputConfFilePath );
   rtCmd.run( "sed -i 's/fieldType_ori/" + fieldType + "/g' " + outputConfFilePath );
   rtCmd.run( "sed -i 's/delimiter_ori/,/g' " + outputConfFilePath );
}

function checkCsvFile ( rtCmd, parameter, expDataArr )
{

   var csvFileName = rtCmd.run( "ls " + tmpFileDir + " | grep " + parameter + " | grep csv" ).split( "\n" )[0];

   var actDataArr = rtCmd.run( "cd " + tmpFileDir + "; cat \\" + csvFileName ).split( "\n" );
   for( i = 0; i < actDataArr.length; i++ )
   {
      if( actDataArr[i] !== expDataArr[i] ) 
      {
         throw new Error( "checkCsvFile fail,[check csv file data, line: " + i + "]" +
            "[" + expDataArr[i] + "]" +
            "[" + actDataArr[i] + "]" );
      }
   }
}
