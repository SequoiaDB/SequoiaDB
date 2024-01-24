/************************************************************************
*@Description: seqDB-18540:tables.fields.defaultValue配置默认值，fieldType配置为MAPPING_DECIMAL
*@Author: 2019-6-28  xiaoni huang init
************************************************************************/
main( test );

function test ()
{
   var clName;
   var rtCmd;

   try
   {
      if( commIsStandalone( db ) )
      {
         return;
      }

      var groupNames = getDataGroupNames();
      var groupName = groupNames[getRandomInt( 0, groupNames.length )];
      var csName = COMMCSNAME;
      clName = "cl18540_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );
      cl.insert( { t: 1 } );

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18540.conf";
      getOutputConfFile( groupName, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );
      // replay
      var clNameArr = [csName + "." + clName];
      var confPath = tmpFileDir + csName + "." + clName + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "replica", confPath );

      // check results
      var expDataArr = [
         '"I","0","-1.8888E+308","-188880000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","0.0","1.000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","MAX","MIN","-INF","INF","NAN","max","MiX","-iNf","inf","nan","nAn"'];
      checkCsvFile( rtCmd, clName, expDataArr );

      // clean env
      cleanCL( csName, clName );
      cleanFile( rtCmd );
   }
   catch( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }
}

function configOutputConfFile ( rtCmd, groupName, csName, clName )
{
   var fullCLName = csName + "." + clName;
   var targetConfPath = tmpFileDir + fullCLName + ".conf";

   rtCmd.run( "sed -i 's/filePrefix_ori/test_" + groupName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/source_fullCLName_ori/" + fullCLName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/target_fullCLName_ori/" + fullCLName + "_new/g' " + targetConfPath );
}