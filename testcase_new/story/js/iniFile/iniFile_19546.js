/************************************
*@Description: seqDB-19546 item添加多个注释 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19546/";
   var fileName = "file19546";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";

   var oldComment = "test item comment";
   var comment1 = "front annotation 1";
   var comment2 = "front annotation 2";
   var comment3 = "post annotation 1";
   var comment4 = "post annotation 2";

   var content = "; " + oldComment + "\n" +
      key1 + "=" + value1 + " ; " + oldComment + "\n" +
      key2 + "=" + value2 + "\n" +
      "; test section comment\n" +
      "[" + section + "]\n" +
      key1 + "=" + value1 + "\n" +
      "; test last comment";

   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );

   //添加注释
   iniFile.addComment( key1, comment1, true );
   iniFile.addComment( key1, comment2, true );
   iniFile.addComment( key1, comment3, false );
   iniFile.addComment( key1, comment4, false );
   iniFile.addComment( key2, comment1, true );
   iniFile.addComment( key2, comment2, true );
   iniFile.addComment( key2, comment3, false );
   iniFile.addComment( key2, comment4, false );
   iniFile.save();

   var checkFile = new IniFile( fileFullPath );

   var checkComment1 = checkFile.getComment( key1, true );
   compareValue( oldComment + "\n" + comment1 + "\n" + comment2, checkComment1 );
   var checkComment2 = checkFile.getComment( key1, false );
   compareValue( oldComment + " " + comment3 + " " + comment4, checkComment2 );

   var checkComment3 = checkFile.getComment( key2, true );
   compareValue( comment1 + "\n" + comment2, checkComment3 );
   var checkComment4 = checkFile.getComment( key2, false );
   compareValue( comment3 + " " + comment4, checkComment4 );

   deleteIniFile( filePath );
}