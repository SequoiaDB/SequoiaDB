/************************************************************************
*@Description: seqDB-18513:tables.fields.doubleQuote配置true/false
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
      clName = "cl18513_" + getRandomInt( 0, 100 );

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cl = readyCL( csName, clName, { Group: groupName } );
      readyData( cl );

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18513.conf";
      getOutputConfFile( groupName, csName, clName, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, clName );
      // replay
      var clNameArr = [csName + "." + clName];
      var confPath = tmpFileDir + csName + "." + clName + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "replica", confPath );

      // check results
      checkResults( rtCmd, clName );

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

function readyData ( cl )
{
   cl.insert( {
      "str1": "", "str2": " ",
      "int1": -2147483648, "int2": 2147483647,
      "long1": NumberLong( "-9223372036854775808" ), "long2": NumberLong( "9223372036854775807" ),
      "decimal1": { $decimal: "-1.8888E+308" }, "decimal2": { $decimal: "1.8888E+308" },
      "time1": Timestamp( "1902-01-01-00.00.00.000000" ),
      "time2": Timestamp( "2037-12-31-23.59.59.999999" )
   }, SDB_INSERT_RETURN_ID );

   cl.update( {
      $set: {
         "str1": '_\_', "str2": ' " ',
         "int1": -2147483647, "int2": 2147483646,
         "long1": NumberLong( "-9223372036854775807" ), "long2": NumberLong( "9223372036854775806" ),
         "decimal1": { $decimal: "-1.8888E+309" }, "decimal2": { $decimal: "1.8888E+309" },
         "time1": Timestamp( "1902-01-01-00.00.00.000001" ),
         "time2": Timestamp( "2037-12-31-23.59.59.999998" )
      }
   } );

   cl.remove();
}

function checkResults ( rtCmd, clName )
{
   var tmpDecimal =
      '188880000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000';
   var suffStr1 = '"", ,'
      + '"-2147483648",2147483647,'
      + '"-9223372036854775808",9223372036854775807,'
      + '"-' + tmpDecimal + '",' + tmpDecimal + ','
      + '"1902-01-01 00.00.00.000000",2037-12-31 23.59.59.999999';
   var suffStr2 = '"_\_", " ,'
      + '"-2147483647",2147483646,'
      + '"-9223372036854775807",9223372036854775806,'
      + '"-' + tmpDecimal + '0",' + tmpDecimal + '0,'
      + '"1902-01-01 00.00.00.000001",2037-12-31 23.59.59.999998';
   var expDataArr = ['I,' + suffStr1,
   'B,' + suffStr1,
   'A,' + suffStr2,
   'D,' + suffStr2];
   checkCsvFile( rtCmd, clName, expDataArr );
}