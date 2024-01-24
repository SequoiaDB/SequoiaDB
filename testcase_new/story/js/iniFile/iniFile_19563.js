/************************************
*@Description: seqDB-19563 获取section的注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19563/";
   var fileName = "file19563";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var fileContent = "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 设置section的注释 section存在
   var iniFile = new IniFile( fileFullPath );
   iniFile.setSectionComment( section1, comment1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getSectionComment( section1 );
   compareValue( comment1, checkValue1 );

   // section不存在
   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setSectionComment( "section3", comment1 );
   } );

   // 注释为空
   iniFile.setSectionComment( section2, "" );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getSectionComment( section2 );
   compareValue( "", checkValue1 );

   deleteIniFile( filePath );
}
