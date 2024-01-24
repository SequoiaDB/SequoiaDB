/******************************************************************************
@Description seqDB-18625:内置SQL语句查询$LIST_SEQUENCES
@author yinzhen
@date 2019-7-4
******************************************************************************/
main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_18625";
   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.createAutoIncrement( [{ "Field": "studentID1" }, { "Field": "studentID2" }, { "Field": "studentID3" }, { "Field": "studentID4" }] );

   // 使用内置SQL语句查询 $LIST_SEQUENCES
   var cur = db.exec( "select * from $LIST_SEQUENCES" );
   var autoIncreNames = [];
   while( cur.next() )
   {
      var autoIncreName = cur.current().toObj()["Name"];
      autoIncreNames.push( autoIncreName );
   }

   // 查看快照中自增字段信息
   cur = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var autoIncres = cur.current().toObj()["AutoIncrement"];
   var clAutoIncreNames = [];
   for( var i in autoIncres )
   {
      var clAutoName = autoIncres[i]["SequenceName"];
      clAutoIncreNames.push( clAutoName );
   }

   for( var i in clAutoIncreNames )
   {
      var tmpName = clAutoIncreNames[i];
      if( autoIncreNames.indexOf( tmpName ) == -1 )
      {
         throw new Error( "indexOf(tempName) is -1 but is " + autoIncreNames.indexOf( tmpName ) );
      }
   }

   commDropCL( db, COMMCSNAME, clName );
}
