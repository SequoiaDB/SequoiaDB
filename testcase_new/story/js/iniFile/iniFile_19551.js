/************************************
*@Description: seqDB-19551 指定section获取item注释
*@author:      yinzhen
*@createDate:  2019.10.11
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19551/";
   var fileName = "file19551";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var fileContent = "[" + section1 + "]\n" +
      ";" + comment1 + "\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 指定section获取item注释 section存在
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkValue1 );

   // section不存在
   var checkValue1 = checkFile.getComment( "section2", key1 );
   compareValue( undefined, checkValue1 );

   deleteIniFile( filePath );
}
