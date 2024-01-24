/************************************
*@Description: seqDB-19943 打开不存在的ini文件
*@author:      yinzhen
*@createDate:  2019.10.11
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19943/";
   var fileName = "file19943";
   var fileFullPath = filePath + fileName;

   assert.tryThrow( SDB_FNE, function()
   {
      new IniFile( fileFullPath );
   } );
}
