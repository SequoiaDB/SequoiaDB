/************************************************************************
 * @Description : seqDB-22239:指定-a/--all挂载所有目录
 *                   挂载集合不存在时系统会自动创建，存在时不报错，跑用例能覆盖到，不单独自动化
 *                seqDB-22248:指定-a/--all卸载所有目录
 * @Author : 2020/07/15  xiaoni huang init
************************************************************************/

//main( test ); //CI主机需要先安装fuse，待安装后放开
function test ()
{
   var aliasArr = ['sequoiafs_22239_01', 'sequoiafs_22239_02', 'sequoiafs_22239_03'];

   // 清理
   cleanFsPid( aliasArr );
   cleanFsDir( 'sequoiafs_22239' );

   // seqDB-22239:指定-a/--all挂载所有目录
   // 准备多个挂载目录和配置文件
   readyMountpointAndConf( aliasArr.slice( 0, 2 ) );
   // 创建挂载目录，不存在配置文件  
   cmd.run( 'mkdir -p ' + tmpFileDir + aliasArr[2] );
   // 挂载其中一个目录
   fsstart( '--alias ' + aliasArr[0], 2000 );
   checkResults( aliasArr[0] );
   // -a挂载所有目录
   try
   {
      cmd.run( toolsDir + 'bin/fsstart.sh -a', "", 3000 );
   }
   catch( e )
   {
      // 可能会有其他目录挂载失败，不捕获异常，只校验最终结果
      if( e.message != SDB_TIMEOUT )
      {
         sleep( 2 );
      }
   }
   checkResults( aliasArr.slice( 0, 2 ) );

   // seqDB-22239:指定-a/--all挂载所有目录
   // 停掉一个挂载目录
   fsstop( '--alias ' + aliasArr[0] );
   // 停掉所有挂载目录
   try
   {
      cmd.run( toolsDir + 'bin/fsstop.sh --all' );
   }
   catch( e )
   {
      // 可能会有其他目录去挂载失败，不捕获异常，只校验最终结果
   }
   checkResults( aliasArr, false );

   // 清理   
   cleanFsPid( aliasArr );
   cleanFsDir( 'sequoiafs_22239' );
}
