/************************************
*@Description: seqDB-19543 不指定section设置item注释
*@author:      yinzhen
*@createDate:  2019.10.11
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19543/";
   var fileName = "file19543";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var newComment1 = "newComment1";
   var fileContent = "; " + comment1 + "\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "\n";
   initFile( fileFullPath, fileContent );

   // 不指定section设置item的注释 item不属于任何section 多个section下有相同item 
   var iniFile = new IniFile( fileFullPath );
   iniFile.setComment( key1, newComment1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( key1 );
   compareValue( newComment1, checkValue1 );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkValue1 );
   var checkValue1 = checkFile.getComment( section2, key1 );
   compareValue( comment1, checkValue1 );

   deleteIniFile( filePath );
}
