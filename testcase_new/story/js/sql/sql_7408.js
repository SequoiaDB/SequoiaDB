/******************************************************************************
@Description seqDB-7408:create/drop collectionspace/collection/index，name参数校验
                        1. drop/create collectionspace/collection/index
                           1. create/drop,name为空，error:create/drop cs:SDB_SQL_SYNTAX_ERROR,create/drop cl:SDB_INVALIDARG,
                           2. create/drop,name包含特殊非法字符，如"$"、"."、"a.",error:SDB_INVALIDARG,drop cs还会有SDB_DMS_CS_NOTEXIST
                           3. create/drop,name长度128B，error:SDB_INVALIDARG
                           4. create/drop,name长度127B，成功 
                           5. create，name已存在，error:create cs:SDB_DMS_CS_EXIST,create cl:SDB_DMS_EXIST
                           5. drop，name不存在，error:drop cl:SDB_DMS_NOTEXIST,drop cs:SDB_DMS_CS_NOTEXIST
                        2. drop/create index
                           1. create/drop,name为空，error:SDB_SQL_SYNTAX_ERROR
                           2. create/drop,name包含特殊非法字符，如"$"、"."、"a."，error:SDB_INVALIDARG(create)、SDB_IXM_NOTEXIST(drop) 
                           3. create/drop,name长度1024B，error：SDB_INVALIDARG(create)、SDB_IXM_NOTEXIST(drop)
                           4. create/drop,name长度1023B，成功
                           5. create，name已存在，error:SDB_IXM_REDEF
                           5. drop，name不存在，error:SDB_IXM_NOTEXIST
                           6. create，索引不包含字段，error:SDB_SQL_SYNTAX_ERROR
@author liyuanyue
@date 2020-4-7
******************************************************************************/
//  单号：5767
main( test )

function test ()
{
   var csName = COMMCSNAME + "_7408";
   var clName = COMMCLNAME + "_7408";
   var idxName = CHANGEDPREFIX + "_7408_idx";
   var speChars = ["$", ".", "a."];

   // create collectionspace
   var sql = "create collectionspace " + " ";
   compareError( sql, "create collectionspace success when name = ' ', Except errorno: SDB_SQL_SYNTAX_ERROR", SDB_SQL_SYNTAX_ERROR );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpcsName = speChars[i] + csName;
      var sql = "create collectionspace " + tmpcsName;
      compareError( sql, "create collectionspace success when '" + speChars[i] + "' at the beginning of the name, Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );
   }

   var tmpcsName = csName;
   for( var i = 0; i < 128 - csName.length; i++ )
   {
      tmpcsName += "a";
   }
   var sql = "create collectionspace " + tmpcsName;
   compareError( sql, "create collectionspace success when the length of the name is 128B (invalid length), Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );

   var succsName = tmpcsName.substring( 0, 127 );
   commDropCS( db, succsName );
   var sql = "create collectionspace " + succsName;
   db.execUpdate( sql );

   compareError( sql, "create collectionspace success when cs existed, Except errorno: SDB_DMS_CS_EXIST", SDB_DMS_CS_EXIST );

   // create collection
   var sql = "create collection " + succsName + "." + " ";
   compareError( sql, "create collection success when name = ' ', Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpclName = speChars[i] + clName;
      var sql = "create collection " + succsName + "." + tmpclName;
      compareError( sql, "create collection success when '" + speChars[i] + "' at the beginning of the name, Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );
   }

   var tmpclName = clName;
   for( var i = 0; i < 128 - clName.length; i++ )
   {
      tmpclName += "a";
   }
   var sql = "create collection " + succsName + "." + tmpclName;
   compareError( sql, "create collection success when the length of the name is 128B (invalid length), Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );

   var succlName = tmpclName.substring( 0, 127 );
   commDropCL( db, succsName, succlName );
   var sql = "create collection " + succsName + "." + succlName;
   db.execUpdate( sql );

   compareError( sql, "create collection success when cl existed, Except errorno: SDB_DMS_EXIST", SDB_DMS_EXIST );

   // create index
   var sql = "create index " + " " + " on " + succsName + "." + succlName + " (name)";
   compareError( sql, "create index success when name = ' ', Except errorno: SDB_SQL_SYNTAX_ERROR", SDB_SQL_SYNTAX_ERROR );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpidxName = speChars[i] + idxName;
      var sql = "create index " + tmpidxName + " on " + succsName + "." + succlName + " (name)";
      compareError( sql, "create index success when '" + speChars[i] + "' at the beginning of the name, Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );
   }

   var tmpidxName = idxName;
   for( var i = 0; i < 1024 - idxName.length; i++ )
   {
      tmpidxName += "a";
   }
   var sql = "create index " + tmpidxName + " on " + succsName + "." + succlName + " (name)";
   compareError( sql, "create index success when the length of the name is 1024B (invalid length), Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );

   var sucidxName = tmpidxName.substring( 0, 1023 );
   var sql = "create index " + sucidxName + " on " + succsName + "." + succlName + " (name)";
   db.execUpdate( sql );

   compareError( sql, "create index success when index existed, Except errorno: SDB_IXM_REDEF", SDB_IXM_REDEF );

   var sql = "create index " + idxName + " on " + succsName + "." + succlName + " ";
   compareError( sql, "create index success when field empty,Except errorno:SDB_SQL_SYNTAX_ERROR", SDB_SQL_SYNTAX_ERROR );

   // drop index
   var sql = "drop index " + " " + " on " + succsName + "." + succlName;
   compareError( sql, "drop index success when name = ' ', Except errorno: SDB_SQL_SYNTAX_ERROR", SDB_SQL_SYNTAX_ERROR );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpidxName = speChars[i] + idxName;
      var sql = "drop index " + tmpidxName + " on " + succsName + "." + succlName;
      compareError( sql, "drop index success when '" + speChars[i] + "' at the beginning of the name, Except errorno: SDB_IXM_NOTEXIST", SDB_IXM_NOTEXIST );
   }

   var tmpidxName = sucidxName + "a";
   var sql = "drop index " + tmpidxName + " on " + succsName + "." + succlName;
   compareError( sql, "drop index success when the length of the name is 1024B (invalid length), Except errorno: SDB_IXM_NOTEXIST", SDB_IXM_NOTEXIST );

   var sql = "drop index " + sucidxName + " on " + succsName + "." + succlName;
   db.execUpdate( sql );

   compareError( sql, "drop index success when index not exist, Except errorno: SDB_IXM_NOTEXIST", SDB_IXM_NOTEXIST );

   // drop collection
   var sql = "drop collection " + succsName + "." + " ";
   compareError( sql, "drop collection success when name = ' ', Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpclName = speChars[i] + clName;
      var sql = "drop collection " + succsName + "." + tmpclName;
      compareError( sql, "drop collection success when '" + speChars[i] + "' at the beginning of the name, Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );
   }

   var tmpclName = succlName + "a";
   var sql = "drop collection " + succsName + "." + tmpclName;
   // 单号：5767
   compareError( sql, "drop collection success when the length of the name is 128B (invalid length), Except errorno: SDB_INVALIDARG", SDB_INVALIDARG );
   compareError( sql, "drop collection success when the length of the name is 128B (invalid length), Except errorno: SDB_DMS_NOTEXIST or SDB_INVALIDARG", SDB_DMS_NOTEXIST, SDB_INVALIDARG );
   var sql = "drop collection " + succsName + "." + succlName;
   db.execUpdate( sql );

   compareError( sql, "drop collection success when cl not exist, Except errorno: SDB_DMS_NOTEXIST", SDB_DMS_NOTEXIST );

   // drop collectionspace
   var sql = "drop collectionspace " + " ";
   compareError( sql, "drop collectionspace success when name = ' ', Except errorno: SDB_SQL_SYNTAX_ERROR", SDB_SQL_SYNTAX_ERROR );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpcsName = speChars[i] + csName;
      var sql = "drop collectionspace " + tmpcsName;
      compareError( sql, "drop collectionspace success when '" + speChars[i] + "' at the beginning of the name, Except errorno: SDB_INVALIDARG or SDB_DMS_CS_NOTEXIST", SDB_INVALIDARG, SDB_DMS_CS_NOTEXIST );
   }

   var tmpcsName = succsName + "a";
   var sql = "drop collectionspace " + tmpcsName;
   // 单号：5767
   compareError( sql, "drop collectionspace success when the length of the name is 128B (invalid length), Except errorno:SDB_INVALIDARG", SDB_INVALIDARG );
   compareError( sql, "drop collectionspace success when the length of the name is 128B (invalid length), Except errorno: SDB_DMS_CS_NOTEXIST or SDB_INVALIDARG", SDB_DMS_CS_NOTEXIST, SDB_INVALIDARG );

   var sql = "drop collectionspace " + succsName;
   db.execUpdate( sql );

   compareError( sql, "drop collectionspace success when cs not exist, Except errorno: SDB_DMS_CS_NOTEXIST", SDB_DMS_CS_NOTEXIST );
}
function compareError ( sql, message, code, otherCode )
{
   var codes = [code];
   if( otherCode !== undefined ) { codes.push( otherCode ); }
   assert.tryThrow( codes, function()
   {
      db.execUpdate( sql );
   } );
}
