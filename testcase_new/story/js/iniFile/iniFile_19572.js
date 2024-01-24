/************************************
*@Description: seqDB-19572 注释item，item key和section名相同 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19572/";
   var fileName = "file19572";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var value2 = "3306";

   var content = section1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      section1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      section2 + "=" + value2 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.disableItem( section1 );
   iniFile.disableItem( section2, section2 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkItemValue1 = checkFile.getValue( section1 );
   compareValue( undefined, checkItemValue1 );
   var checkItemValue2 = checkFile.getValue( section1, section1 );
   compareValue( value1, checkItemValue2 );
   var checkItemValue3 = checkFile.getValue( section2, section2 );
   compareValue( undefined, checkItemValue3 );

   deleteIniFile( filePath );
}