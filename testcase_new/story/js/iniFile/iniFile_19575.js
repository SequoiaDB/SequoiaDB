/************************************
*@Description: seqDB-19575 不指定section取消注释item
*@author:      yinzhen
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19575/";
   var fileName = "file19575";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var section2 = "section2";
   var key1 = "key1";
   var value1 = "value1";
   var comment1 = "comment1";
   var comment2 = "comment2";
   var fileContent = "; " + key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      "; " + key1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      "; " + key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 不指定section取消注释item：item不属于任何section 多个section下有相同item
   var iniFile = new IniFile( fileFullPath );
   iniFile.enableItem( key1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( value1, checkValue1 );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( undefined, checkValue1 );
   var checkValue1 = checkFile.getValue( section2, key1 );
   compareValue( undefined, checkValue1 );

   deleteIniFile( filePath );
}
