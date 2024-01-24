/************************************
*@Description: seqDB-19533 指定不存在的item设置值 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19533/";
   var fileName = "file19533";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";
   var key3 = "log_error";
   var value3 = "/opt/sequoiasql/mysql/myinst.log";
   var key4 = "pid-file";
   var value4 = "/opt/sequoiasql/mysql/database/3306//mysqld.pid";

   var content = "; " + key1 + "=" + value1 + "_disable\n" +
      "[" + section + "]\n" +
      "; " + key2 + "=" + value2 + "_disable\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );

   //设置已被注释的的item
   iniFile.setValue( key1, value1 );
   iniFile.setValue( section, key2, value2 );

   //设置不存在的item
   iniFile.setValue( key3, value3 );
   iniFile.setValue( section, key4, value4 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( value1, checkValue1 );

   var checkValue2 = checkFile.getValue( section, key2 );
   compareValue( value2, checkValue2 );

   var checkValue3 = checkFile.getValue( key3 );
   compareValue( value3, checkValue3 );

   var checkValue4 = checkFile.getValue( section, key4 );
   compareValue( value4, checkValue4 );

   deleteIniFile( filePath );
}