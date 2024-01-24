/************************************
*@Description: 多次修改cs名--------//1、用例描述信息和实际不符
*@author:      luweikang
*@createdate:  2018.10.12
*@testlinkCase:seqDB-16098
**************************************/

main( test );

function test ()
{
   var csname1 = CHANGEDPREFIX + "_oldcs_16098_16146";
   var csname2 = CHANGEDPREFIX + "_newcs_16098_16146";
   var notExitName = "notExitName_cs";
   var clName = CHANGEDPREFIX + "_cl_16098_16146";

   commCreateCS( db, csname1, false, "create cs in begine" );
   commCreateCS( db, csname2, false, "create cs in begine" );

   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.renameCS( notExitName, csname1 );
   } );

   assert.tryThrow( SDB_DMS_CS_EXIST, function()
   {
      db.renameCS( csname1, csname2 );
   } );

   commDropCS( db, csname1, true, "clean cs---" );
   commDropCS( db, csname2, true, "clean cs---" );
}