/************************************
*@Description: seqDB-19554 指定不存在的item获取注释 
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19554/";
   var fileName = "file19554";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section = "auto";

   var key1 = "server-uuid";
   var value1 = "428df49d-7ad1-11e9-b432-000c292210af";
   var key2 = "port";
   var value2 = "3306";

   var comment1 = "front annotation 1";
   var comment2 = "front annotation 2";
   var comment3 = "post annotation 1";
   var comment4 = "post annotation 2";

   var content = "; " + comment1 + "\n" +
      "; " + comment2 + "\n" +
      "; " + key1 + "=" + value1 + "; " + comment3 + " " + comment4 + "\n" +
      "; test section comment\n" +
      "[" + section + "]\n" +
      "; " + comment1 + "\n" +
      "; " + key2 + "=" + value2 + " ; " + comment3 + "\n" +
      "; test last comment";
   initFile( fileFullPath, content );

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( key1, true );
   compareValue( undefined, checkComment1 );
   var checkComment2 = checkFile.getComment( section, key2, false );
   compareValue( undefined, checkComment2 );

   var checkComment3 = checkFile.getComment( "notkey", false );
   compareValue( undefined, checkComment3 );
   var checkComment4 = checkFile.getComment( section, "notkey", true );
   compareValue( undefined, checkComment4 );

   deleteIniFile( filePath );
}