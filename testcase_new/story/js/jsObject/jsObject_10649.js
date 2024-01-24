/******************************************************************************
*@Description: seqDB-10649:System对象获取当前用户环境信息
*@author: Zhao Xiaoni
******************************************************************************/
function test ()
{
   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].getUserEnv();
   }
}

SystemTest.prototype.getUserEnv = function()
{
   this.init();

   var tmpObj = {};
   var keys = [];
   var values = [];
   var environ = this.system.getUserEnv().toObj();
   var cmdEnvInfo = this.cmd.run( "env" ).split( "\n" );

   // 将cmdEnvInfo数组中不包含"="的元素与前一个元素合并，加在前一个元素的末尾，输出为tmpInfo数组
   var tmpInfo = [];
   for( var i = 0; i < cmdEnvInfo.length; i++ )
   {
      if( cmdEnvInfo[i].indexOf( "=" ) != -1 )
      {
         tmpInfo.push( cmdEnvInfo[i] );
      }
      else
      {
         tmpInfo[tmpInfo.length - 1] = tmpInfo[tmpInfo.length - 1] + cmdEnvInfo[i];
      }
   }

   for( var i = 0; i < tmpInfo.length - 1; i++ )
   {
      var index = tmpInfo[i].indexOf( "=" );
      keys[i] = tmpInfo[i].slice( 0, index );
      values[i] = tmpInfo[i].slice( index + 1 );
      tmpObj[keys[i]] = values[i];
   }

   for( var j in tmpObj )
   {
      if( tmpObj[j] !== environ[j] )
      {
         throw new Error( "tmpObj[" + j + "]: " + tmpObj[j] + ", environ[" + j + "]: " + environ[j] );
      }
   }

   this.release();
}

main( test );
