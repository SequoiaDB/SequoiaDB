/******************************************************************************
@Description :    seqDB-17738:      increment为负值，允许翻转，插入值是序列的MinValue 
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

   var clName = COMMCLNAME + "_17738";
   var increment = -1;
   var cacheSize = 500;
   var acquireSize = 100;
   var minValue = -500;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: {
         Field: "id", Increment: increment,
         CacheSize: 500, AcquireSize: acquireSize, MinValue: minValue, Cycled: true
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

   //coordB指定自增字段值为序列的Minvalue插入记录,此时翻转，coordB丢弃本coord缓存，获取新缓存[-400,-301]
   var insertR1 = { a: 500, id: minValue };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA不指定自增字段插入记录,耗尽本coord缓存[-100,-1]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -2 + i * increment } );
   }

   //coordA不指定自增字段插入记录，获取新的缓存[-200,-101]，此时生成id值为-101，唯一索引首次冲突不报错，
   //再次重新获取新的缓存[-300,-201],此时生成id值为-201再次唯一索引冲突才会报错SDB_IXM_DUP_KEY;
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl[0].insert( { a: 0 } );
   } );

   //coordB不指定自增字段插入记录 
   for( var i = 0; i < 10; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -301 + i * increment } );
   }

   //coordC不指定自增字段插入记录，耗尽本缓存[-300,-201]        
   for( var i = 0; i < 99; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -202 + i * increment } );
   }

   //coordC不指定自增字段插入记录，获取新缓存[-500,-401]
   for( var i = 0; i < 10; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -401 + i * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
