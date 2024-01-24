/************************************
*@Description: seqDB-19570 指定section注释item
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19570/";
   var fileName = "file19570";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";

   var content = key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      key2 + "=" + value2 + "\n" +
      "[" + section2 + "]\n" +
      key2 + "=" + value2 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.disableItem( section1, key1 );
   iniFile.disableItem( section1, key2 );
   iniFile.save();

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.disableItem( "notsection", key1 );
      iniFile.save();
   } );

   var checkFile = new IniFile( fileFullPath );
   var checkItemValue1 = checkFile.getValue( section1, key1 );
   compareValue( undefined, checkItemValue1 );
   var checkItemValue2 = checkFile.getValue( section1, key2 );
   compareValue( undefined, checkItemValue2 );

   var checkItemValue3 = checkFile.getValue( key1 );
   compareValue( value1, checkItemValue3 );
   var checkItemValue4 = checkFile.getValue( section2, key2 );
   compareValue( value2, checkItemValue4 );

   deleteIniFile( filePath );
}