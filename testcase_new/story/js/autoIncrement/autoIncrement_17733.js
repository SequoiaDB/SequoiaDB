/******************************************************************************
@Description :    seqDB-17733:    increment为负值，在序列未使用时（无缓存状态）插入值 
@Modify list :   2018-1-28    Zhao Xiaoni  Init
******************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   if( coordNodes.length < 3 || commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_17733";
   var increment = -1;
   var acquireSize = 100;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", Increment: increment, AcquireSize: acquireSize } } );
   commCreateIndex( dbcl, "a", { id: 1 }, { Unique: true } )

   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
   }

   //自增字段序列值还未使用，coordB指定自增字段插入记录
   var insertR1 = { a: 20, id: -20 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA不指定自增字段插入记录
   for( var i = 0; i < 50; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -101 + i * increment } );
   }

   //coordB不指定自增字段插入记录
   for( var i = 0; i < 50; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -21 + i * increment } );
   }

   //coordC不指定自增字段插入记录，耗尽本coord缓存[-300,-201]
   for( var i = 0; i < 50; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -201 + i * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
