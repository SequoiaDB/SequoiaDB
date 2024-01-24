/************************************************************************
*@Description: seqDB-18447:主子表，多张子表其中包括普通表和分区表，重放归档日志到文件
*@Author: 2019-6-28  xiaoni huang init
************************************************************************/
main( test );

function test ()
{
   var sclName1;
   var rtCmd;

   try
   {
      if( commIsStandalone( db ) )
      {
         return;
      }

      var groupNames = getDataGroupNames();
      var groupName = groupNames[getRandomInt( 0, groupNames.length )];
      var rdmNum = getRandomInt( 0, 100 );
      var csName = COMMCSNAME;
      var mclName = "cl18447_mcl";
      sclName1 = "cl18447_scl1_" + rdmNum;
      var sclName2 = "cl18447_scl2_" + rdmNum;

      rtCmd = getRemoteCmd( groupName );
      initTmpDir( rtCmd );

      // ready cl data
      var cs = db.getCS( csName );
      var mcl = readyCL( csName, mclName, { IsMainCL: true, ShardingKey: { a: 1 } } );
      cs.createCL( sclName1, { Group: groupName } );
      cs.createCL( sclName2, { Group: groupName, ShardingKey: { a: 1 } } );
      mcl.attachCL( csName + "." + sclName1, { LowBound: { a: 0 }, UpBound: { a: 1 } } );
      mcl.attachCL( csName + "." + sclName2, { LowBound: { a: 1 }, UpBound: { a: 2 } } );

      var rc1 = mcl.insert( { a: 0, b: "test0" }, SDB_INSERT_RETURN_ID );
      var oid1 = rc1.toObj()._id.$oid;
      mcl.update( { $set: { b: "test_0" } }, { a: 0 } );
      mcl.remove( { a: 0 } );

      var rc2 = mcl.insert( { a: 1, b: "test1" }, SDB_INSERT_RETURN_ID );
      var oid2 = rc2.toObj()._id.$oid;
      mcl.update( { $set: { b: "test_1" } }, { a: 1 } );
      mcl.remove( { a: 1 } );

      // ready outputconf for sdbreplay
      var tmpConfName = "sdbreplay_18447.conf";
      getOutputConfFile( groupName, csName, sclName1, tmpConfName );
      configOutputConfFile( rtCmd, groupName, csName, sclName1, sclName2 );
      // replay
      var clNameArr = [csName + "." + sclName1, csName + "." + sclName2];
      var confPath = tmpFileDir + csName + "." + sclName1 + ".conf";
      execSdbReplay( rtCmd, groupName, clNameArr, "archive", confPath );

      // check results
      var expDataArr = ['"I","' + oid1 + '",0,"test0"',
      '"B","' + oid1 + '",0,"test0"',
      '"A","' + oid1 + '",0,"test_0"',
      '"D","' + oid1 + '",0,"test_0"',

      '"I","' + oid2 + '",1,"test1"',
      '"B","' + oid2 + '",1,"test1"',
      '"A","' + oid2 + '",1,"test_1"',
      '"D","' + oid2 + '",1,"test_1"'];
      checkCsvFile( rtCmd, sclName1, expDataArr );

      // clean env
      cleanCL( csName, mclName );
      cleanFile( rtCmd );
   }
   catch( e )
   {
      backupFile( rtCmd, sclName1 );
      throw e;
   }
}

function configOutputConfFile ( rtCmd, groupName, csName, sclName1, sclName2 )
{
   var fullSclName1 = csName + "." + sclName1;
   var fullSclName2 = csName + "." + sclName2;
   var targetConfPath = tmpFileDir + fullSclName1 + ".conf";

   rtCmd.run( "sed -i 's/filePrefix_ori/test_" + groupName + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/source_fullCLName_1_ori/" + fullSclName1 + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/source_fullCLName_2_ori/" + fullSclName2 + "/g' " + targetConfPath );
   rtCmd.run( "sed -i 's/target_fullCLName_ori/" + fullSclName1 + "_new/g' " + targetConfPath );
}