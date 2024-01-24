/************************************************************************
*@Description: seqDB-18451:主子表，多张子表其中包括普通表和分区表，重放复制日志到文件   
*@Author: 2019-7-2  xiaoni zhao init
************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var mainClName = "mainClName_18451";
   var subClName1 = "subClName_18451_1";
   var subClName2 = "subClName_18451_2";
   var groupNames = getDataGroupNames();

   var mainCl = readyCL( csName, mainClName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", Group: groupNames[0] } );
   var subCl1 = readyCL( csName, subClName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: groupNames[0] } );
   var subCl2 = readyCL( csName, subClName2, { Group: groupNames[0] } );

   mainCl.attachCL( csName + "." + subClName1, { LowBound: { a: 0 }, UpBound: { a: 200 } } );
   mainCl.attachCL( csName + "." + subClName2, { LowBound: { a: 200 }, UpBound: { a: 400 } } );

   var expDataArr = [];
   var minLSN = getMinLSN( groupNames );
   for( var i = 0; i < 100; i++ )
   {
      mainCl.insert( { a: i } );
      mainCl.insert( { a: 200 + i } );
      mainCl.insert( { a: 300 + i } );
      mainCl.update( { $set: { a: 100 + i } }, { a: 200 + i } );
      mainCl.remove( { a: 300 + i } );

      expDataArr.push( '"I","' + i + '"' );
      expDataArr.push( '"I","' + ( 200 + i ) + '"' );
      expDataArr.push( '"I","' + ( 300 + i ) + '"' );
      expDataArr.push( '"D","' + ( 300 + i ) + '"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var confName = "sdbreplay_18451.conf";
      getOutputConfFile( groupNames[0], csName, mainClName, confName );
      configOutputConfFile( rtCmd, csName, subClName1, subClName2, mainClName );

      var mainClNameArr = [csName + "." + mainClName];
      var subClNameArr = ["\"" + csName + "." + subClName1 + "\"", "\"" + csName + "." + subClName2 + "\""];
      var filter = '\'{CL: [' + subClNameArr + '], MinLSN: ' + minLSN + ' }\'';
      execSdbReplay( rtCmd, groupNames[0], mainClNameArr, "replica", undefined, undefined, undefined, undefined, filter );

      checkCsvFile( rtCmd, mainClName, expDataArr );

      cleanCL( csName, mainClName );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, mainClName );
      throw e;
   }
}
function configOutputConfFile ( rtCmd, csName, subClName1, subClName2, mainClName, targetConfPath )
{
   var targetConfPath = tmpFileDir + csName + "." + mainClName + ".conf";
   rtCmd.run( "sed -i 's/csName.clName_source_1/" + csName + "." + subClName1 + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_source_2/" + csName + "." + subClName2 + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_target/" + csName + "." + mainClName + "/g' " + targetConfPath );
}
