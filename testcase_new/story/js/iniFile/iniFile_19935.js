/************************************
*@Description: seqDB-19935 IniFile类SDB_INIFILE_STRICTMODE测试
*@author:      yinzhen
*@createDate:  2019.10.10
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19935/";
   var fileName = "file19935";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   // 指定SDB_INIFILE_STRICTMODE获取IniFile对象
   // 文件包含多个相同名的section
   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = "[" + section1 + "]\n" + key1 + "=" + value1 + "\n[" + section1 + "]\n" + key1 + "=";
   initFile( fileFullPath, fileContent );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      new IniFile( fileFullPath, SDB_INIFILE_STRICTMODE );
   } );
   deleteIniFile( filePath );

   // section下有多个相同key的item
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section1 + "]\n" + key1 + "=" + value1 + "\n" + key1 + "=" + value1;
   initFile( fileFullPath, fileContent );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      new IniFile( fileFullPath, SDB_INIFILE_STRICTMODE );
   } );
   deleteIniFile( filePath );

   // 文件内的section名都是唯一、section下仅有唯一key的item
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section1 + "]\n" + key1 + "=" + value1 + "\n";
   initFile( fileFullPath, fileContent );
   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_STRICTMODE );

   // 修改section注释和设置item值和注释
   var sectionComment1 = "sectionComment1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var newValue1 = "newValue1";
   iniFile.setSectionComment( section1, sectionComment1 );
   iniFile.setComment( section1, key1, comment1 );
   iniFile.setComment( section1, key1, comment2, false );
   iniFile.save();
   iniFile.setValue( section1, key1, newValue1 );
   iniFile.save();

   // ini 配置文件写入，内容正确
   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_STRICTMODE );
   var checkValue1 = checkFile.toString();
   var fileContent = "; " + sectionComment1 + "\n[" + section1 + "]\n; " + comment1 + "\n" + key1 + "=" + newValue1 + " ; " + comment2;
   compareValue( fileContent, checkValue1 );

   deleteIniFile( filePath );
}
