/************************************
*@Description: seqDB-19567 设置结尾的注释内容校验
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19567/";
   var fileName = "file19567";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 设置结尾的注释内容 包含分隔符号
   var iniFile = new IniFile( fileFullPath );
   var comment1 = "key2=value2";
   iniFile.setLastComment( comment1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( "", checkValue1 );

   // 包含注释符 包含空格 包含转义符 包含单双引号 包含中文
   var comment1 = ";com ment \n\r\t ' \" 中文";
   iniFile.setLastComment( comment1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( comment1, checkValue1 );

   // 设置为空
   var comment1 = "";
   iniFile.setLastComment( comment1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getLastComment();
   compareValue( comment1, checkValue1 );

   deleteIniFile( filePath );
}
