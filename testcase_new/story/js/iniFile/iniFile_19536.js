/************************************
*@Description: seqDB-19536 指定section获取item的值
*@author:      yinzhen
*@createDate:  2019.10.10
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19536/";
   var fileName = "file19536";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "section1";
   var key1 = "key1";
   var value1 = "value1";
   var initContent = "[" + section1 + "]\n" + key1 + "=" + value1;
   initFile( fileFullPath, initContent );

   // 指定section获取item的值 
   // section存在
   var checkFile = new IniFile( fileFullPath );
   var checkValue1 = checkFile.getValue( section1, key1 );
   compareValue( value1, checkValue1 );

   // section不存在
   var checkValue2 = checkFile.getValue( "section2", key1 );
   if( checkValue2 !== undefined )
   {
      throw new Error( "get not exist section item return " + checkValue2 );
   }

   deleteIniFile( filePath );
}
