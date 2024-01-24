/************************************
*@Description: seqDB-19926 IniFile类SDB_INIFILE_NOTCASE测试
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19526/";
   var fileName = "file19526";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "aUtO";

   var key1 = "sErVeR-UUID";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var newValue1 = "b432-7ad1-428df49d-000c292210af-11e9";
   var key2 = "PoRt";
   var value2 = "3306";
   var newValue2 = "6603";

   var content = key1 + "=" + value1 + "\n" +
      "[" + section + "]\n" +
      key2 + "=" + value2 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath, SDB_INIFILE_NOTCASE );

   //设置无section的item
   iniFile.setValue( key1.toLowerCase(), newValue1 );

   //设置有section的item
   iniFile.setValue( section.toLowerCase(), key2.toUpperCase(), newValue2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath, SDB_INIFILE_NOTCASE );
   var checkValue1 = checkFile.getValue( key1.toUpperCase() );
   compareValue( newValue1, checkValue1 );

   var checkValue2 = checkFile.getValue( section.toUpperCase(), key2.toLowerCase() );
   compareValue( newValue2, checkValue2 );

   deleteIniFile( filePath );
}