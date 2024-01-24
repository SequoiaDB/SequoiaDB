/************************************
*@Description: seqDB-19569 删除结尾的注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19569/";
   var fileName = "file19569";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; " + comment1;
   initFile( fileFullPath, fileContent );

   // 删除结尾的注释 存在单个注释
   var iniFile = new IniFile( fileFullPath );
   iniFile.delLastComment();
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.toString();
   var expValue1 = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   compareValue( expValue1, checkValue1 );

   // 注释为空
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      ";";
   initFile( fileFullPath, fileContent );

   var iniFile = new IniFile( fileFullPath );
   iniFile.delLastComment();
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.toString();
   compareValue( expValue1, checkValue1 );

   // 不存在注释
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   var iniFile = new IniFile( fileFullPath );
   iniFile.delLastComment();
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.toString();
   compareValue( expValue1, checkValue1 );

   // 存在多个注释
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; " + comment1 + "\n" +
      "; " + comment1 + "\n" +
      "; " + comment1;
   initFile( fileFullPath, fileContent );

   var iniFile = new IniFile( fileFullPath );
   iniFile.delLastComment();
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.toString();
   compareValue( expValue1, checkValue1 );

   deleteIniFile( filePath );
}
