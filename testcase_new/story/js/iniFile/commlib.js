import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/************************************
*@Description: 创建ini配置文件
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
function makeIniFile ( filePath, fileName )
{
   var fileFullPath = filePath + "/" + fileName;
   File.mkdir( filePath );
   try
   {
      File.remove( fileFullPath );
   }
   catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }
   new File( fileFullPath ).close();
}

/************************************
*@Description: 删除文件
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
function deleteIniFile ( filePath )
{
   try
   {
      File.remove( filePath );
   }
   catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }
}

/************************************
*@Description: 比较结果
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
function compareValue ( expValue, actValue )
{
   assert.equal( expValue, actValue );
}

/************************************
*@Description: 初始化ini配置文件
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
function initFile ( filePath, content )
{
   var file = new File( filePath );
   file.write( content );
   file.close();
}