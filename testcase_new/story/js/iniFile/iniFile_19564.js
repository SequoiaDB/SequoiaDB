/************************************
*@Description: seqDB-19564 删除section的注释 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19564/";
   var fileName = "file19564";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";
   var section3 = "conf";
   var section4 = "config";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";

   var oldComment = "old test comment";
   var comment1 = "section annotation 1";
   var comment2 = "";

   var content = key1 + "=" + value1 + "\n" +
      "; " + oldComment + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; " + comment1 + "\n" +
      "; " + comment2 + "\n" +
      "[" + section2 + "]\n" +
      key2 + "=" + value2 + "\n" +
      ";\n" +
      "[" + section3 + "]\n" +
      key2 + "=" + value2 + "\n" +
      "[" + section4 + "]\n" +
      key2 + "=" + value2 + "\n" +
      "; test last comment";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.delSectionComment( section1 );
   iniFile.delSectionComment( section2 );
   iniFile.delSectionComment( section3 );
   iniFile.delSectionComment( section4 );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getSectionComment( section1 );
   compareValue( "", checkComment1 );
   var checkComment2 = checkFile.getSectionComment( section2 );
   compareValue( "", checkComment2 );
   var checkComment3 = checkFile.getSectionComment( section3 );
   compareValue( "", checkComment3 );
   var checkComment4 = checkFile.getSectionComment( section4 );
   compareValue( "", checkComment4 );

   deleteIniFile( filePath );
}