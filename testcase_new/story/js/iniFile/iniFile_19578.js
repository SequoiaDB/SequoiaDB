/************************************
*@Description: seqDB-19578 注释所有的item
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19578/";
   var fileName = "file19578";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";
   var key3 = "log_error";
   var value3 = "/opt/sequoiasql/mysql/myinst.log";

   var content = key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      key2 + "=" + value2 + "\n" +
      "[" + section2 + "]\n" +
      key2 + "=" + value2 + "\n" + 
      key3 + "=" + value3 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.disableAllItem();
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkItemValue1 = checkFile.getValue( key1 );
   compareValue( undefined, checkItemValue1 );
   var checkItemValue2 = checkFile.getValue( section1, key1 );
   compareValue( undefined, checkItemValue2 );

   var checkItemValue3 = checkFile.getValue( section1, key2 );
   compareValue( undefined, checkItemValue3 );
   var checkItemValue4 = checkFile.getValue( section2, key2 );
   compareValue( undefined, checkItemValue4 );

   var checkItemValue5 = checkFile.getValue( section2, key3 );
   compareValue( undefined, checkItemValue5 );

   deleteIniFile( filePath );
}
