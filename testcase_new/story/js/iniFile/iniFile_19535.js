/************************************
*@Description: seqDB-19535 设置item的值内容校验
*@author:      luweikang
*@createDate:  2019.10.12
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19535/";
   var fileName = "file19535";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var fileContent = key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1;
   initFile( fileFullPath, fileContent );

   // 设置item的值 包含分隔符号
   var iniFile = new IniFile( fileFullPath );
   var newValue1 = "key2=value2";
   iniFile.setValue( key1, newValue1 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( newValue1, checkValue1 );

   // 包含注释符
   var newValue2 = "value2; value3";
   iniFile.setValue( key1, newValue2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue2 = checkFile.getValue( key1 );
   compareValue( "value2", checkValue2 );

   // 包含空格
   var newValue3 = "value2 value3";
   iniFile.setValue( key1, newValue3 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue3 = checkFile.getValue( key1 );
   compareValue( newValue3, checkValue3 );

   // 包含单双引号
   var newValue4 = "val'u\"e4";
   iniFile.setValue( key1, newValue4 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue4 = checkFile.getValue( key1 );
   compareValue( newValue4, checkValue4 );

   // 包含中文
   var newValue5 = "中文";
   iniFile.setValue( key1, newValue5 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue5 = checkFile.getValue( key1 );
   compareValue( newValue5, checkValue5 );

   // 设置为空
   var newValue6 = "";
   iniFile.setValue( key1, newValue6 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue6 = checkFile.getValue( key1 );
   compareValue( newValue6, checkValue6 );

   deleteIniFile( filePath );
}
