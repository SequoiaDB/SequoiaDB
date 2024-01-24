/************************************************************************
*@Description: seqDB-18619:重放日志到文件，覆盖支持的filedType类型相关的所有更新符
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
      clName = "cl18619_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );
      cl.insert( { "int": 1, "long": { "$numberLong": "1" }, "decimal": { "$decimal": "1.0" }, "time": { "$timestamp": "1969-12-31-23.59.59.999999" }, "str": "test" } );

      cl.update( { "$inc": { "int": 1, "long": 2, "decimal": 0.2 } } );
      cl.update( { "$set": { "str": "" } } );
      cl.update( { "$unset": { "str": "" } } );
      cl.update( { "$replace": { "int": 2, "long": { "$numberLong": "3" }, "decimal": { "$decimal": "3.0" }, "time": { "$timestamp": "1970-01-01-00.00.00:000000" }, "str": " " } } );

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18619.conf";
      getOutputConfFile( groupName, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );
      // replay
      var clNameArr = [csName + "." + clName];
      var confPath = tmpFileDir + csName + "." + clName + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "replica", confPath );

      // check results
      var expDataArr = ['"I","1","1","1.0","1969-12-31 23.59.59.999999","test"',
         '"B","1","1","1.0","1969-12-31 23.59.59.999999","test"',
         '"A","2","3","1.2","1969-12-31 23.59.59.999999","test"',
         '"B","2","3","1.2","1969-12-31 23.59.59.999999","test"',
         '"A","2","3","1.2","1969-12-31 23.59.59.999999",""',
         '"B","2","3","1.2","1969-12-31 23.59.59.999999",""',
         '"A","2","3","1.2","1969-12-31 23.59.59.999999","test123"',
         '"B","2","3","1.2","1969-12-31 23.59.59.999999","test123"',
         '"A","2","3","3.0","1970-01-01 00.00.00.000000"," "'];
      checkCsvFile( rtCmd, clName, expDataArr );

      // clean env
      cleanCL( csName, clName );
      cleanFile( rtCmd );
   }
   catch( e )
   {
      sleep( 3000 );
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