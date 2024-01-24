/************************************
*@Description: seqDB-19565 设置结尾的注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19565/";
   var fileName = "file19565";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "[" + section2 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; " + comment1;
   initFile( fileFullPath, fileContent );

   // 设置结尾的注释 已存在结尾注释
   var iniFile = new IniFile( fileFullPath );
   iniFile.setLastComment( comment2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( comment2, checkValue1 );

   // 不存在结尾注释
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section2 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );
   var iniFile = new IniFile( fileFullPath );
   iniFile.setLastComment( comment2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( comment2, checkValue1 );

   // 设置为空
   deleteIniFile( fileFullPath );
   makeIniFile( filePath, fileName );
   var fileContent = "[" + section2 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; " + comment1;
   initFile( fileFullPath, fileContent );
   var iniFile = new IniFile( fileFullPath );
   iniFile.setLastComment( "" );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( "", checkValue1 );

   deleteIniFile( filePath );
}
