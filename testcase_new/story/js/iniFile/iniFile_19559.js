/************************************
*@Description: seqDB-19559 不指定section删除item的注释
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19559/";
   var fileName = "file19559";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "; " + comment1 + "\n" +
      key1 + "=" + value1 + "; " + comment2 + "\n" +
      "[" + section1 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "; " + comment2 + "\n" +
      "[" + section2 + "]\n" +
      "; " + comment1 + "\n" +
      key1 + "=" + value1 + "; " + comment2;
   initFile( fileFullPath, fileContent );

   // 删除item的注释：item不属于任何section 多个section下有相同item
   var iniFile = new IniFile( fileFullPath );
   iniFile.delComment( key1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( key1 );
   compareValue( "", checkValue1 );
   var checkValue1 = checkFile.getComment( section1, key1 );
   compareValue( comment1, checkValue1 );
   var checkValue2 = checkFile.getComment( section2, key1 );
   compareValue( comment1, checkValue2 );

   // 删除后置注释
   iniFile.delComment( key1, false );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getComment( key1, false );
   compareValue( "", checkValue1 );
   var checkValue1 = checkFile.getComment( section1, key1, false );
   compareValue( comment2, checkValue1 );
   var checkValue2 = checkFile.getComment( section2, key1, false );
   compareValue( comment2, checkValue2 );

   deleteIniFile( filePath );
}
