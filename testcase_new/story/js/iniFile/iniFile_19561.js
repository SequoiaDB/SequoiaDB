/************************************
*@Description: seqDB-19561 添加section的注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19561/";
   var fileName = "file19561";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "; " + comment1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 添加section的注释，已有注释，添加多个注释
   var iniFile = new IniFile( fileFullPath );
   iniFile.addSectionComment( section1, comment2 );
   iniFile.addSectionComment( section1, comment2 );
   iniFile.addSectionComment( section1, comment2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getSectionComment( section1 );
   compareValue( comment1 + "\n" + comment2 + "\n" + comment2 + "\n" + comment2, checkValue1 );

   // 无注释，添加多个注释
   deleteIniFile( fileFullPath );
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );
   var iniFile = new IniFile( fileFullPath );
   iniFile.addSectionComment( section1, comment2 );
   iniFile.addSectionComment( section1, comment2 );
   iniFile.addSectionComment( section1, comment2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getSectionComment( section1 );
   compareValue( comment2 + "\n" + comment2 + "\n" + comment2, checkValue1 );

   deleteIniFile( filePath );
}
