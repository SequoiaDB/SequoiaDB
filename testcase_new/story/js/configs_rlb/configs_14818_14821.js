/******************************************************************************
 * @Description : test update run config 
 *                seqDB-14818:更新配置文件中run级别配置
 *                seqDB-14821:更新run级别配置，且配置参数不存在conf文件中
 * @author      : Liang XueWang 
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var nodeNum = 1;
   var groupName = "rg_14818_14821";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeOption = { diaglevel: 3 };
   var nodes = commCreateRG( db, groupName, nodeNum, hostName, nodeOption );

   //指定diaglevel创建节点会将此参数写入节点conf文件，由于后面会校验节点conf文件里的配置参数，将此参数从conf文件里删除，以便后面校验
   var config = { "diaglevel": 1 };
   var options = { "HostName": nodes[0].hostname, "ServiceName": nodes[0].svcname.toString() };
   deleteConf( db, config, options );

   //当前值为默认值，修改参数值为默认值
   config = getRandomRunConfig( "defaultVal" );
   updateConf( db, config, options );

   var snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   var fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo );

   //删除配置参数值，将配置恢复为默认值
   deleteConf( db, config, options );

   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo, true );

   //当前值为默认值，修改参数值为其他值
   config = getRandomRunConfig( "validVal" );
   updateConf( db, config, options );

   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo );

   //删除配置参数值，将配置恢复为默认值
   deleteConf( db, config, options );

   //补充问题单SEQUOIADBMAINSTREAM-4809中关于非法值的测试点
   testInvalidValue( nodes, groupName, options );

   var key = Object.getOwnPropertyNames( config )[0];
   config[key] = getConfigs( "defaultVal" )["runConfigs"][key];
   snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( config, snapshotInfo );
   fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( config, fileInfo, true );

   db.removeRG( groupName );
}

/*******************************************************************************
 @Description : 补充问题单SEQUOIADBMAINSTREAM-4809中的测试点的测试点
 @Modify list : 2020.9.5 yipan
 *******************************************************************************/
function testInvalidValue ( nodes, groupName, options )
{
   var data = [];
   data.push( new configuration( "preferedinstance", "runConfigs", "1,N,2,M,Y", "M,1,2" ) );
   data.push( new configuration( "preferedinstance", "runConfigs", "3,4,A", "3,4,A" ) );
   data.push( new configuration( "preferedinstance", "runConfigs", "aaa", "M" ) );
   data.push( new configuration( "preferedinstancemode", "runConfigs", "ordered", "ordered" ) );
   data.push( new configuration( "preferedinstancemode", "runConfigs", "aaa", "random" ) );
   data.push( new configuration( "diagnum", "runConfigs", -10, -1 ) );
   data.push( new configuration( "auditnum", "runConfigs", -100, -1 ) );
   data.push( new configuration( "transisolation", "runConfigs", -10, 0 ) );
   //mvcc分支和主干、3.2隔离级别最大值不一致，屏蔽最大值测试
   //data.push( new configuration( "transisolation", "runConfigs", 100, 2 ) );
   data.push( new configuration( "maxreplsync", "runConfigs", 300, 200 ) );
   data.push( new configuration( "maxreplsync", "expFail", -1, 10 ) );
   data.push( new configuration( "numpreload", "expFail", -1, 0 ) );
   data.push( new configuration( "numpreload", "rebootConfigs", 2000, 100 ) );
   data.push( new configuration( "maxprefpool", "expFail", -1, 0 ) );
   data.push( new configuration( "maxprefpool", "rebootConfigs", 2000, 1000 ) );

   for( var i = 0; i < data.length; i++ )
   {
      var key = data[i]["name"];
      //updateConf对象
      var config = {};
      config[key] = data[i]["invalidVal"];
      //预期结果
      var expResult = {};
      expResult[ key] = data[i]["expResult"]
      //修改配置
      if( data[i]["type"] == "expFail" )
      {
         //期望失败
         assert.tryThrow( SDB_INVALIDARG, function()
         {
            db.updateConf( config, options );
         } );
      } else if( data[i]["type"] == "rebootConfigs" )
      {
         //重启生效
         updateConf( db, config, options, SDB_RTN_CONF_NOT_TAKE_EFFECT );
         db.getRG( groupName ).stop();
         db.getRG( groupName ).start();
         var snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
         checkResult( expResult, snapshotInfo );
         var fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
         checkResult( expResult, fileInfo );
         
         deleteConf(db, config, options, SDB_RTN_CONF_NOT_TAKE_EFFECT);
         db.getRG( groupName ).stop();
         db.getRG( groupName ).start();
      } else if( data[i]["type"] == "runConfigs" )
      {
         //在线生效
         db.updateConf( config, options );
         var actResult = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
         checkResult( expResult, actResult );
         var fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
         checkResult( expResult, fileInfo );
         
         deleteConf(db, config, options);
      }
   }
};
function configuration ( name, type, invalidVal, expResult )
{
   this.name = name;//属性名
   this.type = type;//类型
   this.invalidVal = invalidVal;//非法输入的数据
   this.expResult = expResult;//预期结果
}


