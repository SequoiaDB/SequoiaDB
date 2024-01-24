/******************************************************************************
@Description :    seqDB-17736:    increment为负值，插入值是序列的currentValue 
@Modify list :   2018-1-25    Zhao Xiaoni  Init
******************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   if( coordNodes.length < 3 || commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_17736";
   var increment = -1;
   var acquireSize = 100;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", Increment: increment, AcquireSize: acquireSize } } );
   commCreateIndex( dbcl, "a", { id: 1 }, { Unique: true } )

   //连接所有coord插入部分记录,coord缓存分别为[-100,-1],[-200,-101],[-300,-201]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
      cl[i].insert( { a: i } );
      expRecs.push( { a: i, id: -1 + i * increment * acquireSize } );
   }

   //coordB指定自增字段值为本序列的currentValue插入记录，catalog丢弃老的缓存，coordB获取新的缓存[-1101,-1002]
   var insertR1 = { a: 1001, id: -1001 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //检查catalog已丢弃老的缓存，重新生成新的缓存
   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_id_SEQ";
   var cursor = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } );
   if( cursor.current().toObj().CurrentValue !== -2001 )
   {
      throw new Error( "failed!" );
   }

   //coordA不指定自增字段插入记录，耗尽本coord缓存[-100,-1]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -2 + i * increment } );
   }

   //coordA不指定自增字段插入记录，获取新的缓存[-1201,-1102]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -1102 + i * increment } );
   }

   //coordB不指定自增字段插入记录
   for( var i = 0; i < 100; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -1002 + i * increment } );
   }

   //coordC不指定自增字段插入记录，耗尽本coord缓存[-300,-201]
   for( var i = 0; i < 99; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -202 + i * increment } );
   }

   //coordC不指定自增字段插入记录，由于冲突，获取新的缓存[-1301,-1202]
   for( var i = 0; i < 100; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -1202 + i * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
