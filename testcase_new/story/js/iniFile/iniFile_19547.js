/************************************
*@Description: seqDB-19547 不指定section为item添加多个注释
*@author:      yinzhen
*@createDate:  2019.10.11
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19547/";
   var fileName = "file19547";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var fileContent = "; " + comment1 + "\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "\n";
   initFile( fileFullPath, fileContent );

   // 不指定section为item添加多个注释：item不属于任何section 多个section下有相同item
   var iniFile = new IniFile( fileFullPath );
   var newComment1 = "newComment1";
   var newComment2 = "newComment2";
   iniFile.addComment( key1, newComment1 );
   iniFile.addComment( key1, newComment2 );
   iniFile.addComment( key1, newComment1, false );
   iniFile.addComment( key1, newComment2, false );
   iniFile.save();

   // 不属于任何 section 的 item 可以被添加注释，而其他的 item 注释不变
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( key1 );
   var expValue = comment1 + "\n" + newComment1 + "\n" + newComment2;
   compareValue( expValue, checkValue1 );
   var checkValue1 = checkFile.getComment( key1, false );
   var expValue = newComment1 + " " + newComment2;
   compareValue( expValue, checkValue1 );

   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkValue1 );
   var checkValue1 = checkFile.getComment( section2, key1 );
   compareValue( comment1, checkValue1 );

   var checkValue1 = checkFile.getComment( section1, key1, false );
   compareValue( "", checkValue1 );
   var checkValue1 = checkFile.getComment( section2, key1, false );
   compareValue( "", checkValue1 );

   deleteIniFile( filePath );
}
