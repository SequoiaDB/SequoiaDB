/************************************
*@Description: seqDB-19566 添加结尾的注释
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19566/";
   var fileName = "file19566";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";

   var lastComment1 = "last annotation 1";
   var lastComment2 = "last annotation 2";
   var lastComment3 = "last annotation 3";
   var lastComment4 = "last annotation 4";
   var lastComment5 = "last annotation 5";
   var lastComment6 = "last annotation 6";

   var content = key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "[" + section2 + "]\n" +
      key2 + "=" + value2 + "\n";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.addLastComment( lastComment1 );
   iniFile.addLastComment( lastComment2 );
   iniFile.addLastComment( lastComment3 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkLastComment1 = checkFile.getLastComment();
   compareValue( lastComment1 + "\n" + lastComment2 + "\n" + lastComment3, checkLastComment1 );

   var iniFile = new IniFile( fileFullPath );
   iniFile.addLastComment( lastComment4 );
   iniFile.addLastComment( lastComment5 );
   iniFile.addLastComment( lastComment6 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkLastComment2 = checkFile.getLastComment();
   compareValue( lastComment1 + "\n" + lastComment2 + "\n" + lastComment3 + "\n" +
      lastComment4 + "\n" + lastComment5 + "\n" + lastComment6, checkLastComment2 );

   deleteIniFile( filePath );
}