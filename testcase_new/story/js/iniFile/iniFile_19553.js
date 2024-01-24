/************************************
*@Description: seqDB-19553 获取item的注释，item key和section名相同
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19553/";
   var fileName = "file19553";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "key1";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "; " + comment2 + "\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 获取item的注释 指定section获取item注释
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkValue1 );

   // 不指定section获取item注释
   var checkValue1 = checkFile.getComment( key1 );
   compareValue( comment2, checkValue1 );

   deleteIniFile( filePath );
}
