/******************************************************************************
@Description seqDB-18634:内置SQL语句查询$SNAPSHOT_SEQUENCES
@author yinzhen
@date 2019-7-5
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_18634";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.createAutoIncrement( [{ "Field": "studentID1" }, { "Field": "studentID2" }, { "Field": "studentID3" }, { "Field": "studentID4" }] );

   // 使用内置SQL语句查询 $SNAPSHOT_SEQUENCES
   var cur = db.exec( "select * from $SNAPSHOT_SEQUENCES" );
   var sqlAutoIncreNames = [];
   while( cur.next() )
   {
      var tmpName = cur.current().toObj()["Name"];
      sqlAutoIncreNames.push( tmpName );
   }

   // 查看快照中自增字段信息
   cur = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var snapsqlAutoIncres = cur.current().toObj()["AutoIncrement"];
   var clAutoIncreNames = [];
   for( var i in snapsqlAutoIncres )
   {
      var tmpName = snapsqlAutoIncres[i]["SequenceName"];
      clAutoIncreNames.push( tmpName );
   }

   // 校验结果，校验内置SQL语句查询是否和snapshot一致
   for( var i in clAutoIncreNames )
   {
      var tmpName = clAutoIncreNames[i];
      if( sqlAutoIncreNames.indexOf( tmpName ) == -1 )
      {
         throw new Error( "Expect indexOf(tempName) is -1,but is" + sqlAutoIncreNames.indexOf( tmpName ) );
      }
   }

   commDropCL( db, COMMCSNAME, clName );
}