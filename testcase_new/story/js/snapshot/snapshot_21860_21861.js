/******************************************************************************
*@Description : seqDB-21860:子表开启压缩类型，分别插入不同的数据量，指定ShowMainCLMode为main，查询集合快照 
                seqDB-21861:子表开启压缩类型，分别插入不同的数据量（都需大于64M），指定ShowMainCLMode为main，查询集合快照
*@author      : Zhao xiaoni
*@Date        : 2020-02-20
******************************************************************************/
testConf.skipStandAlone = true;

//main( test );SEQUOIADBMAINSTREAM-5578

function test ()
{
   var mainCLName = "mainCL_21860_21861";
   var subCLName1 = "subCL_21860_21861_1";
   var subCLName2 = "subCL_21860_21861_2";
   var group = commGetGroups( db )[0];
   var groupName = group[0].GroupName;
   var primaryPos = group[0].PrimaryPos;
   var nodeName = group[primaryPos].HostName + ":" + group[primaryPos].svcname;

   commDropCL( db, COMMCSNAME, mainCLName );
   commDropCL( db, COMMCSNAME, subCLName1 );
   commDropCL( db, COMMCSNAME, subCLName2 );
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, {
      IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range",
      ReplSize: 0
   } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1, {
      ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true,
      Group: groupName, ReplSize: 0
   } );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, {
      ShardingKey: { a: 1 }, ShardingType: "hash", Compressed: true,
      Group: groupName, ReplSize: 0
   } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var arr = new Array( 1024 * 1024 );
   arr = arr.join( "a" );
   for( var i = 0; i < 101; i++ )
   {
      subCL1.insert( { a: i, b: arr } );
   }

   var expResult = { DictionaryCreated: false };
   var snapshotOption = "/*+use_option( ShowMainCLMode, main )*/";
   var cursor = db.exec( 'select * from $SNAPSHOT_CL where Name = "' + COMMCSNAME + '.' + mainCLName + '" ' + snapshotOption );
   checkParameters( cursor, expResult );

   for( var i = 0; i < 101; i++ )
   {
      subCL2.insert( { a: i, b: arr } );
   }

   //ReplSize为0，可以只校验主节点已创建压缩字典
   var doTimes = 0;
   var totalTimes = 500;
   while( !dictionaryCreated && doTimes < totalTimes )
   {
      doTimes++;
      sleep( 100 );
      var cursor = db.exec( 'select * from $SNAPSHOT_CL where Name = "' + COMMCSNAME + '.' + mainCLName + '" and NodeName = "' +
         nodeName + '" ' + snapshotOption );
      var dictionaryCreated = cursor.current().toObj().Details[0].DictionaryCreated;
   }
   if( doTimes === totalTimes )
   {
      throw new Error( "\nactResult: " + JSON.stringify( actResult ) + "\nexpResult: " + JSON.stringify( expResult ) );
   }

   commDropCL( db, COMMCSNAME, mainCLName, false, false );
}

