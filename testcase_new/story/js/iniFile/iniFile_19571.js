/************************************
*@Description: seqDB-19571 不指定section注释item
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19571/";
   var fileName = "file19571";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      key1 + "=" + value1 + "\n";
   initFile( fileFullPath, fileContent );

   // 不指定section注释item item不属于任何section 多个section下有相同item 
   var iniFile = new IniFile( fileFullPath );
   iniFile.disableItem( key1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( undefined, checkValue1 );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( value1, checkValue1 );
   var checkValue1 = checkFile.getValue( section2, key1 );
   compareValue( value1, checkValue1 );

   deleteIniFile( filePath );
}
