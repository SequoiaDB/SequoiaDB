/************************************
*@Description: seqDB-19558 指定section删除item的注释
*@author:      luweikang
*@createDate:  2019.10.08
**************************************/
main( test );

function test ()
{
   var filePath = WORKDIR + "/ini19558/";
   var fileName = "file19558";
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
      key1 + "=" + value1 + "; " + comment3 + " " + comment4 + "\n" +
      "; test section comment\n" +
      "[" + section + "]\n" +
      "; " + comment1 + "\n" +
      "; " + comment2 + "\n" +
      key1 + "=" + value1 + "; " + comment3 + " " + comment4 + "\n" +
      "; test last comment";
   initFile( fileFullPath, content );

   var iniFile = new IniFile( fileFullPath );
   iniFile.delComment( section, key1, true );
   iniFile.delComment( section, key1, false );
   iniFile.save();

   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.delComment( "notsection", key1, true );
      iniFile.save();
   } );


   assert.tryThrow( SDB_FIELD_NOT_EXIST, function()
   {
      iniFile.delComment( "notsection", key1, false );
      iniFile.save();
   } );


   var checkFile = new IniFile( fileFullPath );
   var checkComment1 = checkFile.getComment( section, key1, true );
   compareValue( "", checkComment1 );
   var checkComment2 = checkFile.getComment( section, key1, false );
   compareValue( "", checkComment2 );

   deleteIniFile( filePath );
}