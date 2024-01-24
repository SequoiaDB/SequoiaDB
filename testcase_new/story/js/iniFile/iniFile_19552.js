/************************************
*@Description: seqDB-19552 不指定section获取item注释
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19552/";
   var fileName = "file19552";
   var fileFullPath = filePath + fileName;
   makeIniFile( filePath, fileName );

   var section1 = "auto";
   var section2 = "mysqld";

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
      key1 + "=" + value1 + "; " + comment3 + " " + comment4 + "\n" +
      "; test section comment\n" +
      "[" + section1 + "]\n" +
      key2 + "=" + value2 + "\n" +
   "[" + section2 + "]\n" +
      "; " + comment1 + "\n" +
      key2 + "=" + value2 + " ; " + comment3 + "\n" +
      "; test last comment";
   initFile( fileFullPath, content );

   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( key1, true );
   compareValue( comment1 + "\n" + comment2, checkComment1 );
   var checkComment2 = checkFile.getComment( key1, false );
   compareValue( comment3 + " " + comment4, checkComment2 );

   var checkComment3 = checkFile.getComment( key2, true );
   compareValue( undefined, checkComment3 );
   var checkComment4 = checkFile.getComment( key2, false );
   compareValue( undefined, checkComment4 );

   deleteIniFile( filePath );
}
