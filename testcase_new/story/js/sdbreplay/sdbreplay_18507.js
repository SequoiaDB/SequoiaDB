/************************************************************************
*@Description: seqDB-18507:delimiter配置字段间分隔符包含转义字符
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
      clName = "cl18507_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );
      cl.insert( { a: "test" } );
      cl.update( { $set: { a: "test2" } } );
      cl.remove();

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18507.conf";
      getOutputConfFile( groupName, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );
      // replay
      var clNameArr = [csName + "." + clName];
      var confPath = tmpFileDir + csName + "." + clName + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "replica", confPath );

      // check results
      var delimiter = '_a""b_';
      var actDelim = '_a"b_';
      var expDataArr = ['"I"' + actDelim + '"test"',
      '"B"' + actDelim + '"test"',
      '"A"' + actDelim + '"test2"',
      '"D"' + actDelim + '"test2"'];
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