/******************************************************************************
 * @Description   : Backup all and restore for data node
 * @Author        : wenjing Wang
 * @CreateTime    : 2018.01.10
 * @LastEditTime  : 2022.01.21
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.skipStandAlone = true;
function backupTestCase11668 ()
{

}

backupTestCase11668.prototype = new backupTestCase( db );
backupTestCase11668.prototype.constructor = backupTestCase11668;
backupTestCase11668.prototype.clName = COMMCLNAME + "_11668";
backupTestCase11668.prototype.reInit =
   function()
   {
      if( this.group === undefined )
      {
         this.sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
         this.db = this.sdb
      }
      else
      {
         this.db = new Sdb( this.nodeinfo.hostName, this.nodeinfo.svcName );
      }
   }

backupTestCase11668.prototype.execTest =
   function( backupName, path )
   {
      this.docs = bakInsertData( this.cl );
      this.oids.push( sdbPutLob( this.cl, path ) );

      bakBackup( this.db, { "Name": backupName } );
      if( this.group !== undefined )
      {
         this.removeNodeExceptPrimary();
      }

      if( this.nodeinfo !== undefined )
      {
         var bakInfo = new backUpInfo( backupName, this.nodeinfo.dbPath + "bakfile" );
      }
      else
      {
         var dbPath = db.snapshot( 6 ).current().toObj()["Disk"]["DatabasePath"];
         var bakInfo = new backUpInfo( backupName, dbPath + "bakfile" );
      }
      this.checkBackupRes( bakInfo, 1 );
      sdbRestore( this.sdb, this.cmd, bakInfo, this.nodeinfo );
      this.checkResult();
   }

main( test );

function test ()
{
   try
   {
      var testBackUp = new backupTestCase11668();
      if( testBackUp.setUp() )
      {
         testBackUp.test();
      }
      testBackUp.tearDown();
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         println( e.fileName + ":" + e.lineNumber + " throw " + e.message );
         println( e.stack );
      }

      var backupDir = WORKDIR + "ci/rsrvnodelog/11668";
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
      //因为默认teardown没有删除组操作，所以此处保持不变
      commDropCL( db, COMMCSNAME, backupTestCase11668.prototype.clName, true, true, "finally ：Drop CL in the end" );
      db.removeRG( backupandrestoreGroup );
   }

}
