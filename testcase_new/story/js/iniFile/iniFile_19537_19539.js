/************************************
*@Description: seqDB-19537 不指定section获取item的值 
*              seqDB-19539 获取不存在的item值
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19537/";
   var fileName = "file19537";
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

   var content = key1 + "=" + value1 + "\n" +
      "; " + key2 + "=" + value2 + "\n" +
      "[" + section + "]\n" +
      key3 + "=" + value3 + "\n" +
      "; " + key4 + "=" + value4;
   initFile( fileFullPath, content );

   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1 );
   compareValue( value1, checkValue1 );
   var checkValue1_section = checkFile.getValue( section, key1 );
   compareValue( undefined, checkValue1_section );

   var checkValue2 = checkFile.getValue( key2 );
   compareValue( undefined, checkValue2 );

   var checkValue3 = checkFile.getValue( key3 );
   compareValue( undefined, checkValue3 );

   var checkValue4 = checkFile.getValue( key4 );
   compareValue( undefined, checkValue4 );
   var checkValue4_section = checkFile.getValue( section, key4 );
   compareValue( undefined, checkValue4_section );

   deleteIniFile( filePath );
}