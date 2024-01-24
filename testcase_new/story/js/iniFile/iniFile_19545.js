/************************************
*@Description: seqDB-19545 指定不存在的item设置注释
*@author:      yinzhen
*@createDate:  2019.10.11
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19545/";
   var fileName = "file19545";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   initFile( fileFullPath, "[" + section1 + "]" );

   // 指定不存在的item设置注释：指定section进行设置 不指定section进行设置
   var iniFile = new IniFile( fileFullPath );
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setComment( section1, key1, comment1 );
   } );

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setComment( key1, comment1 );
   } );

   // 存在该item已注释：指定section进行设置 不指定section进行设置
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   var fileContent = "; " + key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      "; " + key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   var iniFile = new IniFile( fileFullPath );
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setComment( section1, key1, comment1 );
   } );

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setComment( key1, comment1 );
   } );

   deleteIniFile( filePath );
}
