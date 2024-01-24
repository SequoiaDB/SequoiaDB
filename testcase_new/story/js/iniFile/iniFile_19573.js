/************************************
*@Description: seqDB-19573 注释不存在的item
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19573/";
   var fileName = "file19573";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 注释不存在的item 指定section进行设置 
   var iniFile = new IniFile( fileFullPath );
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.disableItem( section1, "key2" );
   } );

   // 不指定section进行设置
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.disableItem( "key2" );
   } );

   // 存在该item已注释 
   var fileContent = "; key2=value2\n" +
      "[" + section1 + "]\n" +
      "; key2=value2\n" +
      key1 + "=" + value1;
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   initFile( fileFullPath, fileContent );

   // 注释不存在的item 指定section进行设置 
   var iniFile = new IniFile( fileFullPath );
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.disableItem( section1, "key2" );
   } );

   // 不指定section进行设置
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.disableItem( "key2" );
   } );

   deleteIniFile( filePath );
}
