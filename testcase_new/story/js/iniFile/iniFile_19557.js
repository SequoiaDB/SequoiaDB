/************************************
*@Description: seqDB-19557 删除item多个注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19557/";
   var fileName = "file19557";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "[" + section1 + "]\n" +
      "; " + comment1 + "\n" +
      "; " + comment1 + "\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "; " + comment2 + " " + comment2 + " " + comment2;
   initFile( fileFullPath, fileContent );

   // 删除item多个注释 多个前置注释
   var iniFile = new IniFile( fileFullPath );
   iniFile.delComment( section1, key1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( "", checkValue1 );

   // 多个后置注释
   iniFile.delComment( section1, key1, false );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( section1, key1, false );
   compareValue( "", checkValue1 );

   deleteIniFile( filePath );
}
