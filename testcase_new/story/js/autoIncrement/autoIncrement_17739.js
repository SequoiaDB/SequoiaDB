/******************************************************************************
@Description :    seqDB-17739:increment为负值，不允许翻转，插入值是序列的MinValue 
@Modify list :   2018-1-29    Zhao Xiaoni  Init
******************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   if( coordNodes.length < 3 || commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_17739";
   var increment = -1;
   var cacheSize = 500;
   var acquireSize = 100;
   var minValue = -500;

   var coordB = new Sdb( coordNodes[1] );
   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: {
         Field: "id", Increment: increment,
         CacheSize: 500, AcquireSize: acquireSize, MinValue: minValue
      }
   } );
   commCreateIndex( dbcl, "a", { id: 1 }, { Unique: true } );

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

   //coordB指定自增字段值为序列的Minvalue插入记录,此时coord上的所有缓存被丢弃
   var insertR1 = { a: 500, id: minValue };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA不指定自增字段插入记录,耗尽本coord缓存[-100,-1]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -2 + i * increment } );
   }

   //coordA超范围插入失败
   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      cl[0].insert( { a: 0 } );
   } );

   //coordB超范围插入失败
   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      cl[1].insert( { a: 0 } );
   } );

   //coordC不指定自增字段插入记录,耗尽本coord缓存[-300,-201]
   for( var i = 0; i < 99; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -202 + i * increment } );
   }

   //coordC不指定自增字段插入，待coordC缓存耗尽后，超范围插入失败
   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      cl[2].insert( { a: 0 } );
   } );

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
