/************************************
*@Description: seqDB-19531 不指定section设置item的值
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19531/";
   var fileName = "file19531";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var newValue1 = "b432-7ad1-428df49d-000c292210af-11e9";
   var key2 = "port";
   var value2 = "3306";
   var newValue2 = "6603";
   var key3 = "log_error";
   var value3 = "/opt/sequoiasql/mysql/myinst.log";
   var newValue3 = "/opt/mysql/sequoiasql/myconf.log";
   var key4 = "pid-file";
   var value4 = "/opt/sequoiasql/mysql/database/3306//mysqld.pid";

   var content = key1 + "=" + value1 + "\n" +
      key2 + "=" + value2 + "\n" +
      "[" + section1 + "]\n" +
      key2 + "=" + value2 + "\n" +
      key3 + "=" + value3 + "\n" +
      "[" + section2 + "]\n" +
      key3 + "=" + value3 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );

   //item不属于任一section
   iniFile.setValue( key1, newValue1 );

   //item有section和无section
   iniFile.setValue( key2, newValue2 );

   //item无section并属于多个section
   iniFile.setValue( key3, newValue3 );

   //item不存在
   iniFile.setValue( key4, value4 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( newValue1, checkValue1 );

   var checkValue2 = checkFile.getValue( key2 );
   var checkValue2_section = checkFile.getValue( section1, key2 );
   compareValue( newValue2, checkValue2 );
   compareValue( value2, checkValue2_section );

   var checkValue3 = checkFile.getValue( key3 );
   var checkValue3_section1 = checkFile.getValue( section1, key3 );
   var checkValue3_section2 = checkFile.getValue( section2, key3 );
   compareValue( newValue3, checkValue3 );
   compareValue( value3, checkValue3_section1 );
   compareValue( value3, checkValue3_section2 );

   var checkValue4 = checkFile.getValue( key4 );
   compareValue( value4, checkValue4 );

   deleteIniFile( filePath );
}