/************************************************************************
*@Description: seqDB-18538:tables.fields.defaultValue配置默认值，fieldType配置为MAPPING_INT
               seqDB-18539:tables.fields.defaultValue配置默认值，fieldType配置为MAPPING_LONG
               seqDB-18541:tables.fields.defaultValue配置默认值，fieldType配置为MAPPING_TIMESTAMP
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
      clName = "cl18538_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );
      cl.insert( { t: 1 } );
      cl.update( { $inc: { t: 2 } } );
      cl.remove();

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18538.conf";
      getOutputConfFile( groupName, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );
      // replay
      var clNameArr = [csName + "." + clName];
      var confPath = tmpFileDir + csName + "." + clName + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "replica", confPath );

      // check results
      var expDataArr = [
         '"I",""," ","-2147483648","2147483647","-9223372036854775808","9223372036854775807","1902-01-01-00.00.00.000000","2037-12-31-23.59.59.999999"',
         '"B",""," ","-2147483648","2147483647","-9223372036854775808","9223372036854775807","1902-01-01-00.00.00.000000","2037-12-31-23.59.59.999999"',
         '"A",""," ","-2147483648","2147483647","-9223372036854775808","9223372036854775807","1902-01-01-00.00.00.000000","2037-12-31-23.59.59.999999"',
         '"D",""," ","-2147483648","2147483647","-9223372036854775808","9223372036854775807","1902-01-01-00.00.00.000000","2037-12-31-23.59.59.999999"'];
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