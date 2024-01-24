/************************************
*@Description: seqDB-22240:指定-m/--mountpoint挂载目录
*              seqDB-22249:指定-m/--mountpoint卸载目录
*@Author:      2020/07/17  liuli
**************************************/

// main( test );
function test ()
{
   var alias = 'sequoiafs_22240_01';

   // 清理
   cleanFsPid( alias );
   cleanFsDir( 'sequoiafs_22240' );

   // 创建挂载目录，不存在配置文件
   cmd.run( 'mkdir -p ' + tmpFileDir + alias );
   var command = toolsDir + 'bin/fsstart.sh -m ' + tmpFileDir + alias;
   try
   {
      // 挂载不存在配置文件的目录
      cmd.run( command, '', 1000 );
   }
   catch( e )
   {
      if( e.message != 4 )
      {
         throw e;
      }
   }
   checkResults( alias, false );

   // 准备挂载目录和配置文件
   readyMountpointAndConf( alias );

   // 挂载目录
   fsstart( '-m ' + tmpFileDir + alias );
   checkResults( alias );
   // 卸载alias，卸载目录存在
   fsstop( '-m ' + tmpFileDir + alias );

   // 重新挂载该目录
   fsstart( '-m ' + tmpFileDir + alias );
   checkResults( alias );

   // 清理   
   cleanFsPid( alias );
   cleanFsDir( 'sequoiafs_22240' );

   var command = toolsDir + 'bin/fsstart.sh -m ' + tmpFileDir + alias;
   try
   {
      // 清理后alias目录不存在，重新挂载该目录
      cmd.run( command, '', 1000 );
   }
   catch( e )
   {
      if( e.message != 4 )
      {
         throw e;
      }
   }
   checkResults( alias, false );

   // 清理   
   cleanFsPid( alias );
   cleanFsDir( 'sequoiafs_22240' );
}
