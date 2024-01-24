/************************************
*@Description: seqDB-19534 指定空value的item设置值
*@author:      yinzhen
*@createDate:  2019.10.09
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19534/";
   var fileName = "file19534";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var key2 = "key2";
   var key3 = "key3";
   var key4 = "key4";
   var initContent = "; " + key1 + "=\n" + key2 + "=\n[" + section1 + "]\n; " + key3 + "=\n" + key4 + "=";
   initFile( fileFullPath, initContent );

   // 指定空value的item设置值 
   // item属于section
   var iniFile = new IniFile( fileFullPath );
   var value1 = "value1";
   var value2 = "value2";
   iniFile.setValue( section1, key1, value1 );
   iniFile.setValue( section1, key2, value2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( value1, checkValue1 );
   var checkValue2 = checkFile.getValue( section1, key2 );
   compareValue( value2, checkValue2 );

   // item不属于section
   var value3 = "value3";
   var value4 = "value4";
   iniFile.setValue( key3, value3 );
   iniFile.setValue( key4, value4 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key3 );
   compareValue( value3, checkValue1 );
   var checkValue2 = checkFile.getValue( key4 );
   compareValue( value4, checkValue2 );

   deleteIniFile( filePath );
}
