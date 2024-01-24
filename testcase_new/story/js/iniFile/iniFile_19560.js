/************************************
*@Description: seqDB-19560 设置section的注释
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19560/";
   var fileName = "file19560";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";

   var oldComment = "old test comment";
   var comment1 = "new annotation 1";
   var comment2 = "";

   var content = key1 + "=" + value1 + "\n" +
      "[" + section1 + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; " + oldComment + "\n" +
      "[" + section2 + "]\n" +
      key2 + "=" + value2 + "\n" +
      "; test last comment";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.setSectionComment( section1, comment1 );
   iniFile.setSectionComment( section2, comment2 );
   iniFile.save();

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.setSectionComment( "notsection", comment1 );
      iniFile.save();
   } );

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getSectionComment( section1 );
   compareValue( comment1, checkComment1 );
   var checkComment2 = checkFile.getSectionComment( section2 );
   compareValue( comment2, checkComment2 );

   deleteIniFile( filePath );
}