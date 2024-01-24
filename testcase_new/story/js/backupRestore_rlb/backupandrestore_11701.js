/******************************************************************************
 * @Description   : Backup all and remove for data node
 * @Author        : wenjing Wang
 * @CreateTime    : 2018.01.10
 * @LastEditTime  : 2022.01.21
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.skipStandAlone = true;
function backupTestCase11701 ()
{

}

backupTestCase11701.prototype = new backupTestCase( db );
backupTestCase11701.prototype.constructor = backupTestCase11701;
backupTestCase11701.prototype.clName = COMMCLNAME + "_11701";
backupTestCase11701.prototype.execTest =
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
      if( this.group !== undefined )
      {
         this.checkBackupRes( bakInfo, 1, [this.group.GroupName] );
      }
      else
      {
         this.checkBackupRes( bakInfo, 1 );
      }

      bakRemoveBackups( this.db, backupName, false );
      if( this.group !== undefined )
      {
         this.checkBackupRes( bakInfo, 0, [this.group.GroupName] );
      }
      else
      {
         this.checkBackupRes( bakInfo, 0 );
      }

      if( !IsBakPathEmpty( this.cmd, bakInfo.bakPath ) )
      {
         throw new Error( "removeBackup expect backPath is empty, but real is not" );
      }
   }
main( test );

function test ()
{
   try
   {
      var testBackUp = new backupTestCase11701();
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

      var backupDir = WORKDIR + "ci/rsrvnodelog/11701";
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
      commDropCL( db, COMMCSNAME, backupTestCase11701.prototype.clName, true, true, "finally ：Drop CL in the end" );
      db.removeRG( backupandrestoreGroup );
   }
}
