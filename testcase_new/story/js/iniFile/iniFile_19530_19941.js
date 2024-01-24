/************************************
*@Description: seqDB-19530 指定section设置item的值
*              seqDB-19941 多次修改ini配置文件 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19530/";
   var fileName = "file19530";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var newValue1 = "000c292210af-428df49d-7ad1-11e9-b432";
   var key2 = "port";
   var value2 = "3306";
   var key3 = "log_error";
   var value3 = "/opt/sequoiasql/mysql/myinst.log";

   //指定不存在的section设置item的值
   var iniFile = new IniFile( fileFullPath );
   iniFile.setValue( "auto", key1, value1 );
   iniFile.setValue( "mysqld", key2, value2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( "auto", key1 );
   var checkValue2 = checkFile.getValue( "mysqld", key2 );
   compareValue( value1, checkValue1 );
   compareValue( value2, checkValue2 );

   //指定存在的section设置item的值
   iniFile.setValue( "auto", key1, newValue1 );
   iniFile.setValue( "mysqld", key3, value3 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( "auto", key1 );
   var checkValue3 = checkFile.getValue( "mysqld", key3 );
   compareValue( newValue1, checkValue1 );
   compareValue( value3, checkValue3 );

   deleteIniFile( filePath );
}