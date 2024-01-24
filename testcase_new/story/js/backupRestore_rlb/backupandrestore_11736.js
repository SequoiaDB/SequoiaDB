/******************************************************************************
 * @Description   : Backup all and restore for cluster
 * @Author        : wenjing Wang
 * @CreateTime    : 2018.01.10
 * @LastEditTime  : 2022.01.21
 * @LastEditors   : 钟子明
 ******************************************************************************/
function backupTestCase11736 ()
{

}

backupTestCase11736.prototype = new backupTestCase( db );
backupTestCase11736.prototype.constructor = backupTestCase11736;
backupTestCase11736.prototype.csName = COMMCSNAME + "_11736";
backupTestCase11736.prototype.clName = COMMCLNAME + "_11736";
backupTestCase11736.prototype.reInit =
   function()
   {

   }

backupTestCase11736.prototype.createShardingCL =
   function()
   {
      this.groupNames = [];
      this.groups = commGetGroups( this.db );
      var hosts = getAllHosts( this.groups );
      createBackupRestoreGroup( db, hosts );
      this.group = db.getRG( backupandrestoreGroup ).getDetail().next().toObj();

      for( var i = 0; i < this.groups.length; ++i )
      {
         this.groupNames.push( this.groups[i][0].GroupName );
      }
      this.groupNames.push( backupandrestoreGroup );
      this.domainName = "backup11736";

      commDropCS( this.db, this.csName, true, "dropCS in the beginning" );
      commDropDomain( this.db, this.domainName );
      commCreateDomain( this.db, this.domainName, this.groupNames, { AutoSplit: true } );

      var csOpt = { Domain: this.domainName, LobPageSize: 4096 };
      commCreateCS( this.db, this.csName, false, "create cs in begin", csOpt );
      var clOpt = { ShardingType: 'hash', ShardingKey: { no: 1 }, ReplSize: -1 }
      this.cl = commCreateCL( this.db, this.csName, this.clName, clOpt, true, false,
         "Create collection in the beginning" );
   }

backupTestCase11736.prototype.init =
   function()
   {
      this.createShardingCL();
      for( var i = 0; i < this.group.Group.length; ++i )
      {
         if( this.group.PrimaryNode === this.group.Group[i].NodeID || i === this.group.Group.length - 1 )
         {
            var hostName = this.group.Group[i].HostName;
            var svcName = this.group.Group[i].Service[0].Name;
            var dbPath = this.group.Group[i].dbpath;
            this.nodeinfo = new nodeInfo( this.group.GroupName, hostName, svcName, dbPath );
            this.cmd = getCmdByHostName( this.localCmd, hostName );
            break;
         }
      }

      return true;
   }

backupTestCase11736.prototype.execTest =
   function( backupName, path )
   {
      var bakInfo = new backUpInfo( backupName, this.nodeinfo.dbPath + "bakfile" );
      this.groupNames.push( "SYSCatalogGroup" );
      this.docs = bakInsertData( this.cl );
      this.oids.push( sdbPutLob( this.cl, path ) );

      // ȫ������
      bakBackup( this.db, { "Name": backupName } );
      this.checkBackupRes( bakInfo, 1, this.groupNames );

      this.docs = bakInsertData( this.cl );
      this.oids.push( sdbPutLob( this.cl, path ) );

      for( var i = 0; i < 1000; ++i )
      {
         commCreateCL( this.db, this.csName, this.clName + "_" + i, { ReplSize: -1 }, true, false );
      }

      bakBackup( this.db, { "Name": backupName, EnsureInc: true } );
      this.checkBackupRes( bakInfo, 2, this.groupNames );
      if( this.group !== undefined )
      {
         this.removeNodeExceptPrimary();
      }
      sdbRestore( this.sdb, this.cmd, bakInfo, this.nodeinfo );
      this.checkResult( 2 );
   }

backupTestCase11736.prototype.tearDown =
   function()
   {
      bakRemoveBackups( this.db, CHANGEDPREFIX, true );
      commDropCS( this.db, this.csName, true, "dropCS in the end" );
      commDropCS( this.db, backupTestCase11736.prototype.csName, true, "finally: dropCS in the end" );
      commDropDomain( this.db, this.domainName );
      db.removeRG( backupandrestoreGroup );
   }

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   try
   {
      var testBackUp = new backupTestCase11736();
      if( testBackUp.setUp() )
      {
         testBackUp.test();
      }
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( e.fileName + ":" + e.lineNumber + " throw " + e.message );
         println( e.stack );
      }

      var backupDir = WORKDIR + "ci/rsrvnodelog/11736";
      commMakeDir( COORDHOSTNAME, backupDir );

      //该变量未定义有可能是文件权限引起的，此处不应该让这个runtime异常阻挡真正的异常
      //抛出，因为它只是一个错误记录
      if( this.logSourcePaths != undefined )
         for( var i = 0; i < this.logSourcePaths.length; i++ )
         {
            File.scp( this.logSourcePaths[i], backupDir + "/sdbdiag" + i + ".log" );
         }
      throw e;
   }
   finally
   {
      testBackUp.tearDown();
   }
}