/************************************
*@Description: seqDB-19532 设置item的值，item key和section名相同
*@author:      yinzhen
*@createDate:  2019.10.09
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19532/";
   var fileName = "file19532";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var value2 = "b432-000c292210af-428df49d-7ad1-11e9";
   var value3 = "000c292210af-428df49d-b432-7ad1-11e9";
   var iniFile = new IniFile( fileFullPath );

   // 已有item的key和section名相同
   iniFile.setValue( key1, key1, value1 );
   iniFile.save();

   // 指定section进行设置
   iniFile.setValue( key1, key1, value2 );
   iniFile.save();
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( key1, key1 );
   compareValue( value2, checkValue1 );

   // 不指定section进行设置
   iniFile.setValue( key1, value3 );
   iniFile.save();
   var checkFile = new IniFile( fileFullPath );
   var checkValue2 = checkFile.getValue( key1 );
   compareValue( value3, checkValue2 );

   deleteIniFile( filePath );
}
