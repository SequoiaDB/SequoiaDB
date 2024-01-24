/******************************************************************************
 * @Description   : seqDB-24286: 重放唯一键冲突的记录时验证keepshardingkey隐藏参数
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.02.17
 * @LastEditTime  : 2022.04.25
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
main( test );

function test ()
{
   if ( commIsStandalone( db ) )
   {
      return;
   }
   //创建集合, 覆盖分区表和主子表
   var csName1 = "csName_24286_1";
   var clName = "clName_24286";
   var csName2 = "csName_24286_2";
   var mclName = "mclName_24286";
   var sclName = "sclName_24286";
   var groupNames = getDataGroupNames();

   var cl1 = readyCL( csName1, clName, { Group: groupNames[0], ShardingKey: { b: 1 }, ShardingType: "range" } );
   var mcl = readyCL( csName2, mclName, { IsMainCL: true, ShardingKey: { date: 1 }, ShardingType: "range" } );
   var scl = readyCL( csName2, sclName, { Group: groupNames[0], ShardingKey: { date: 1 }, ShardingType: "range" } );
   mcl.attachCL( csName2 + "." + sclName, { LowBound: { date: "20220101" }, UpBound: { date: "20220131" } } );

   //创建唯一索引
   commCreateIndex( cl1, 'idx1', { b: 1 }, { Unique: true } );
   cl1.insert( { _id: 1, a: 1, b: 1 } );
   cl1.insert( { _id: 2, a: 2, b: 2 } );
   commCreateIndex( scl, 'idx2', { date: 1 }, { Unique: true } );
   scl.insert( { _id: 1, weather: "sunny", date: "20220115" } );
   scl.insert( { _id: 2, weather: "cloudy", date: "20220116" } );

   //切分数据
   cl1.split( groupNames[0], groupNames[1], 50 );
   scl.split( groupNames[0], groupNames[1], 50 );

   //插入唯一键冲突的记录
   cl1.insert( { a: 3, b: 1 }, SDB_INSERT_REPLACEONDUP );
   mcl.insert( { weather: "windy", date: "20220115" }, SDB_INSERT_REPLACEONDUP );

   var expDataArr1 = [];
   expDataArr1.push( '"I","1","1"' );
   expDataArr1.push( '"I","2","2"' );
   expDataArr1.push( '"D","2","2"' );
   expDataArr1.push( '"B","1","1"' );
   expDataArr1.push( '"A","3","1"' );
   var expDataArr2 = [];
   expDataArr2.push( '"I","sunny","20220115"' );
   expDataArr2.push( '"I","cloudy","20220116"' );
   expDataArr2.push( '"D","cloudy","20220116"' );
   expDataArr2.push( '"B","sunny","20220115"' );
   expDataArr2.push( '"A","windy","20220115"' );

   var rtCmd = getRemoteCmd( groupNames[0] );
   initTmpDir( rtCmd );
   var type = "replica";
   var keepshardingkey = false;

   try
   {
      //重放分区表数据
      var fieldType1 = "MAPPING_INT";
      var confName1 = "sdbreplay_24286_1.conf";
      getOutputConfFile( groupNames[0], csName1, clName, confName1 );
      configOutputConfFile( rtCmd, groupNames[0], csName1, clName, fieldType1 );
      var clNameArray1 = [csName1 + "." + clName];
      var filter1 = '\'{CL: ["' + csName1 + '.' + clName + '"] }\'';
      sdbReplay( rtCmd, groupNames[0], clNameArray1, type, undefined, undefined, undefined, undefined, filter1, undefined );
      sdbReplay( rtCmd, groupNames[0], clNameArray1, type, undefined, undefined, undefined, undefined, filter1, keepshardingkey );
      checkCsvFile( rtCmd, clName, expDataArr1 );

      cleanCL( csName1, clName );
   } catch ( e )
   {
      backupFile( rtCmd, clName );
      throw e;
   }

   try
   {
      //重放主子表数据
      var fieldType2 = "MAPPING_STRING";
      var confName2 = "sdbreplay_24286_2.conf";
      getOutputConfFile( groupNames[0], csName2, sclName, confName2 );
      configOutputConfFile( rtCmd, groupNames[0], csName2, sclName, fieldType2 );
      var clNameArray2 = [csName2 + "." + sclName];
      var filter2 = '\'{CL: ["' + csName2 + '.' + sclName + '"] }\'';
      sdbReplay( rtCmd, groupNames[0], clNameArray2, type, undefined, undefined, undefined, undefined, filter2, undefined );
      sdbReplay( rtCmd, groupNames[0], clNameArray2, type, undefined, undefined, undefined, undefined, filter2, keepshardingkey );
      checkCsvFile( rtCmd, sclName, expDataArr2 );

      cleanCL( csName2, mclName );
      cleanFile( rtCmd );
   } catch ( e )
   {
      backupFile( rtCmd, sclName );
      throw e;
   }
}

function sdbReplay ( rtCmd, groupName, clNameArr, type, confPath, statusPath, daemon, watch, filter, keepshardingkey )
{
   var confName = clNameArr[0] + ".conf";
   var statusName = clNameArr[0] + ".status";
   var tmpCLNameArr = [];
   for ( i = 0; i < clNameArr.length; i++ )
   {
      var tmpCLName = '\"' + clNameArr[i] + '\"';
      tmpCLNameArr.push( tmpCLName );
   }
   if ( typeof ( confPath ) == "undefined" ) { confPath = tmpFileDir + confName; }
   if ( typeof ( statusPath ) == "undefined" ) { statusPath = tmpFileDir + statusName; }
   if ( typeof ( type ) == "undefined" ) { type = "archive"; }  //archive is the main user scenario.
   if ( typeof ( daemon ) == "undefined" ) { daemon = false; }
   if ( typeof ( watch ) == "undefined" ) { watch = false; }
   if ( typeof ( filter ) == "undefined" ) 
   {
      filter = '\'{CL: [' + tmpCLNameArr + '], OP: ["insert", "update", "delete"] }\'';
   }
   if ( typeof ( keepshardingkey ) == "undefined" ) { keepshardingkey = true; }

   // ready file
   var dbPath = getMasterDBPath( groupName );
   var logPath = "";
   if ( type === "archive" ) 
   {
      logPath = dbPath + "archivelog";
   }
   else if ( type === "replica" )
   {
      logPath = dbPath + "replicalog";
   }

   var command = installDir + 'bin/sdbreplay'
      + ' --type ' + type
      + ' --path ' + logPath
      + ' --filter ' + filter
      + ' --outputconf ' + confPath
      + ' --status ' + statusPath
      + ' --daemon ' + daemon
      + ' --watch ' + watch
      + ' --keepshardingkey ' + keepshardingkey;

   var totalRetryTimes = 200;
   var currentRetryTimes = 0;
   var clName = clNameArr[0].split( "." )[1];
   var lsCommand = "ls " + tmpFileDir + " | grep " + clName + " | grep csv";

   //重放前对之前的操作进行刷盘
   db.sync();
   while ( true ) 
   {
      var rcSdbreplay = rtCmd.run( "cd " + tmpFileDir + "; " + command );
      var rcLS;
      try 
      {
         rcLS = rtCmd.run( lsCommand ).split( "\n" )[0];
         break;
      } catch ( e )
      {
         currentRetryTimes++;
         sleep( 100 );
         rtCmd.run( "cd " + tmpFileDir + "; rm *" + clName + "*.status" );
         if ( currentRetryTimes >= totalRetryTimes ) 
         {
            throw new Error( "Failed to get csv file, after retry " + currentRetryTimes + "." );
         }
      }
   }

   return rcSdbreplay;
}
