/******************************************************************************
 * @Description   : 
 * @Author        : Zhang Yanan
 * @CreateTime    : 2022.03.10
 * @LastEditTime  : 2022.04.05
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
/******************************************************************************
 * @Description : seqDB-14836:删除run级别配置，且配置参数不存在conf文件中
 * @author      : Lu weikang
 * @date        ：2018.3.30
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var group = commGetGroups( db )[0];
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;

   //当前值为默认值，删除配置
   var config = getRandomRunConfig( "defaultVal" );
   var options = { HostName: hostName, svcname: svcName };
   deleteConf( db, config, options );

   var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
   checkResult( config, snapshotInfo );
   var fileInfo = getConfFromFile( hostName, svcName );
   checkResult( config, fileInfo, true );

   //当前值为非默认值，删除配置
   var config = getRandomRunConfig( "validVal" );
   var options = { HostName: hostName, svcname: svcName };
   updateConf( db, config, options );

   var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
   checkResult( config, snapshotInfo );
   var fileInfo = getConfFromFile( hostName, svcName );
   checkResult( config, fileInfo );

   deleteConf( db, config, options );

   var key = Object.getOwnPropertyNames( config )[0];
   config[key] = getConfigs( "defaultVal" )["runConfigs"][key];
   var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
   checkResult( config, snapshotInfo );
   var fileInfo = getConfFromFile( hostName, svcName );
   checkResult( config, fileInfo, true );
}

