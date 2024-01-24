/************************************************************************
*@Description: seqDB-18464: tables配置多个不同集合，集合间字段包含相同和不同字段 
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
   var clName1 = "clName_18464_1";
   var clName2 = "clName_18464_2";
   var groupNames = getDataGroupNames();

   var cl1 = readyCL( csName, clName1, { Group: groupNames[0] } );
   var cl2 = readyCL( csName, clName2, { Group: groupNames[0] } );

   var expDataArr = [];
   var minLSN = getMinLSN( groupNames );
   for( var i = 0; i < 100; i++ )
   {
      cl1.insert( { a: i, b: i } );
      cl1.insert( { a: 100 + i, b: 100 + i } );
      cl1.update( { $set: { b: 200 + i } }, { b: i } );
      cl1.remove( { a: 100 + i } );
      cl2.insert( { a: i, c: i } );
      cl2.insert( { a: 100 + i, c: 100 + i } );
      cl2.update( { $set: { c: 200 + i } }, { c: i } );
      cl2.remove( { a: 100 + i } );

      expDataArr.push( '"I","' + i + '","' + i + '"' );
      expDataArr.push( '"I","' + ( 100 + i ) + '","' + ( 100 + i ) + '"' );
      expDataArr.push( '"B","' + i + '","' + i + '"' );
      expDataArr.push( '"A","' + i + '","' + ( 200 + i ) + '"' );
      expDataArr.push( '"D","' + ( 100 + i ) + '","' + ( 100 + i ) + '"' );
   }

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );

   try
   {
      var confName = "sdbreplay_18464.conf";
      getOutputConfFile( groupNames[0], csName, clName1, confName );
      configOutputConfFile( rtCmd, csName, clName1, clName2 );

      var clName1Arr = [csName + "." + clName1];
      var clNameArr = ["\"" + csName + "." + clName1 + "\"", "\"" + csName + "." + clName2 + "\""];
      var filter = filter = '\'{CL: [' + clNameArr + '], MinLSN:' + minLSN + ' }\'';
      execSdbReplay( rtCmd, groupNames[0], clName1Arr, undefined, undefined, undefined, undefined, undefined, filter );

      checkCsvFile( rtCmd, clName1, expDataArr );
      checkCsvFile( rtCmd, clName2, expDataArr );

      cleanCL( csName, clName1 );
      cleanCL( csName, clName2 );
      cleanFile( rtCmd );
   } catch( e )
   {
      backupFile( rtCmd, clName1 );
      throw e;
   }
}
function configOutputConfFile ( rtCmd, csName, clName1, clName2 )
{
   var targetConfPath = tmpFileDir + csName + "." + clName1 + ".conf";
   rtCmd.run( "sed -i 's/csName.clName_source1/" + csName + "." + clName1 + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_target1/" + csName + "." + clName1 + "'/g " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_source2/" + csName + "." + clName2 + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/csName.clName_target2/" + csName + "." + clName2 + "'/g " + targetConfPath );
}