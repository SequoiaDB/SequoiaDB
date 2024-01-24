/************************************
*@Description: seqDB-19538 获取item的值，item key和section名相同
*@author:      yinzhen
*@createDate:  2019.10.10
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19538/";
   var fileName = "file19538";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   // 已有item的key和section名相同
   var section1 = "section1";
   var key1 = "section1";
   var value1 = "value1";
   var value2 = "value2";
   var initContent = key1 + "=" + value2 + "\n[" + section1 + "]\n" + key1 + "=" + value1;
   initFile( fileFullPath, initContent );

   // 获取item的值
   // 指定section进行获取
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( value1, checkValue1 );

   // 不指定section进行获取
   var checkValue2 = checkFile.getValue( key1 );
   compareValue( value2, checkValue2 );

   deleteIniFile( filePath );
}
