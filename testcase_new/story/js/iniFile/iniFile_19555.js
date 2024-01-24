/************************************
*@Description: seqDB-19555 item获取多个注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19555/";
   var fileName = "file19555";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 使用addComment()添加注释
   var iniFile = new IniFile( fileFullPath );
   iniFile.addComment( section1, key1, comment1 );
   iniFile.addComment( section1, key1, comment1 );
   iniFile.addComment( section1, key1, comment1 );
   iniFile.addComment( section1, key1, comment2, false );
   iniFile.addComment( section1, key1, comment2, false );
   iniFile.addComment( section1, key1, comment2, false );
   iniFile.save();

   // tem获取多个注释 获取多个前置注释
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1 + "\n" + comment1 + "\n" + comment1, checkValue1 );

   // 获取多个后置注释
   var checkValue1 = checkFile.getComment( section1, key1, false );
   compareValue( comment2 + " " + comment2 + " " + comment2, checkValue1 );

   deleteIniFile( filePath );
}
