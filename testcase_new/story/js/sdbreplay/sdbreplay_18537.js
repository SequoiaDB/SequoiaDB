/************************************************************************
*@Description: seqDB-18537: tables.fields.defaultValue配置默认值，fieldType配置为MAPPING_STRING 
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
   var clName = "clName_18537";
   var groupNames = getDataGroupNames();

   var cl = readyCL( csName, clName, { Group: groupNames[0] } );

   var expDataArr = [];
   var minLSN = getMinLSN( groupNames );
   var expDefaultValue = new Array( 1024 * 1024 * 5 ).join( "a" );
   for( var i = 0; i < 1; i++ )
   {
      cl.insert( { a: i } );
      cl.insert( { a: 1 + i } );
      cl.update( { $set: { a: 2 + i } }, { a: i } );
      cl.remove( { a: 1 + i } );
      expDataArr.push( '"I","' + i + '","","' + expDefaultValue + '"' );
      expDataArr.push( '"I","' + ( 1 + i ) + '","","' + expDefaultValue + '"' );
      expDataArr.push( '"B","' + i + '","","' + expDefaultValue + '"' );
      expDataArr.push( '"A","' + ( 2 + i ) + '","","' + expDefaultValue + '"' );
      expDataArr.push( '"D","' + ( 1 + i ) + '","","' + expDefaultValue + '"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var confName = "sdbreplay_18537.conf";
      getOutputConfFile( groupNames[0], csName, clName, confName );
      configOutputConfFile( rtCmd, csName, clName );

      var clNameArr = [csName + "." + clName];
      var filter = '\'{CL: ["' + clNameArr + '"], MinLSN:' + minLSN + ' }\'';
      execSdbReplay( rtCmd, groupNames[0], clNameArr, undefined, undefined, undefined, undefined, undefined, filter );

      checkCsvFileLocal( rtCmd, clName, expDataArr, groupNames[0] );

      cleanCL( csName, clName );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}
function configOutputConfFile ( rtCmd, csName, clName )
{
   var targetConfPath = tmpFileDir + csName + "." + clName + ".conf";
   rtCmd.run( "sed -i 's/csName.clName_source/" + csName + "." + clName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_target/" + csName + "." + clName + "/g' " + targetConfPath );
}
function getRemote ( groupName )
{
   var hostName = getMasterHostName( groupName );
   var remote = new Remote( hostName, CMSVCNAME );
   return remote;
}
function checkCsvFileLocal ( rtCmd, clName, expDataArr, groupName )
{
   var expCsvFileName = clName + ".result";
   var expCsvFilePath = tmpFileDir + expCsvFileName;
   var actCsvFileName = rtCmd.run( "ls " + tmpFileDir + " | grep " + clName + " | grep csv" ).split( "\n" )[0];
   var actCsvFilePath = tmpFileDir + actCsvFileName;
   var remote = getRemote( groupName );
   var file = remote.getFile( expCsvFilePath );
   for( var i = 0; i < expDataArr.length; i++ )
   {
      file.write( expDataArr[i] );
      if( i != expDataArr.length - 1 )
      {
         file.write( "\n" );
      }
   }
   var cmd = "diff -w " + expCsvFilePath + " " + actCsvFilePath;
   //diff比较结果不一致会抛错
   rtCmd.run( cmd );
}
