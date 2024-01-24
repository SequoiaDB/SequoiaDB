/************************************
*@Description: seqDB-19932 IniFile类SDB_INIFILE_EQUALSIGN测试
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19532/";
   var fileName = "file19532";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var newValue1 = "b432-7ad1-428df49d-000c292210af-11e9";
   var key2 = "port";
   var value2 = "3306";
   var newValue2 = "6603";
   var key3 = "log_error";
   var value3 = "/opt/sequoiasql/mysql/myinst.log";
   var key4 = "pid-file";
   var value4 = "/opt/sequoiasql/mysql/database/3306//mysqld.pid";

   var content = key1 + "=" + value1 + "\n" +
      "[" + section + "]\n" +
      key2 + "=" + value2 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_EQUALSIGN );

   //设置无section的item
   iniFile.setValue( key1, newValue1 );
   iniFile.setValue( key3, value3 );

   //设置有section的item
   iniFile.setValue( section, key2, newValue2 );
   iniFile.setValue( section, key4, value4 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_EQUALSIGN );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( newValue1, checkValue1 );

   var checkValue2 = checkFile.getValue( section, key2 );
   compareValue( newValue2, checkValue2 );

   var checkValue3 = checkFile.getValue( key3 );
   compareValue( value3, checkValue3 );

   var checkValue4 = checkFile.getValue( section, key4 );
   compareValue( value4, checkValue4 );

   var expContent = key1 + "=" + newValue1 + "\n" +
      key3 + "=" + value3 + "\n" +
      "[" + section + "]\n" +
      key2 + "=" + newValue2 + "\n" +
      key4 + "=" + value4;
   var checkContent = checkFile.toString();
   compareValue( expContent, checkContent );

   deleteIniFile( filePath );
}