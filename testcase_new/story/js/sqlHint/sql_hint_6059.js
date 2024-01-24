/************************************************************************
*@Description:   seqDB-6059:select指定某个集合使用索引扫描，集合不存在_st.sql.hint.011
                 seqDB-6060:hint格式错误_st.sql.hint.012
*@Author:  2016/7/13  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6059";

   dropCL( csName, clName, true, "Failed to drop cl in the begin." );
   createCL( csName, clName, true, true, "Failed to create cl." );

   //verifyParam1( csName );
   verifyParam2( csName, clName );
   verifyParam3( csName, clName );

   dropCL( csName, clName, false, "Failed to drop cl in the end." );
}

function verifyParam1 ( csName )
{

   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db.exec( "select * from 123.atest /*+use_index(idx)*/" );
   } )
}

function verifyParam2 ( csName, clName )
{

   assert.tryThrow( SDB_SQL_SYNTAX_ERROR, function()
   {
      db.exec( "select * from " + csName + "." + clName + " /+use_index(idx)*/" );
   } )
}

function verifyParam3 ( csName, clName )
{
   db.exec( "select * from " + csName + "." + clName + " /*+use_index()*/" );
}