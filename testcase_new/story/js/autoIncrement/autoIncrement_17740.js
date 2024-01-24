/******************************************************************************
@Description :    seqDB-17740: increment为负值，插入值比序列的MinValue稍大，再次插入自增字段未超MinValue  
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

   var clName = COMMCLNAME + "_17740";
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

   //coordB指定自增字段值比MinValue稍大插入记录,此时本coord上缓存被丢弃
   var insertR1 = { a: 490, id: -490 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA不指定自增字段插入,耗尽本coord缓存[-100,-1]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -2 + i * increment } );
   }

   //coordA不指定自增字段插入,获取新的缓存[-100，-1]，此时生成id:-1,首次唯一索引冲突不报错，
   //再次取缓存范围[-200，-101]，生成id:-101唯一索引冲突报错-38，插入失败
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl[0].insert( { a: i } );
   } );

   //coordB不指定自增字段插入,使自增字段值达到minValue
   for( var i = 0; i < 10; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -491 + i * increment } );
   }

   //coordB再次插入时翻转，获取缓存范围[-300,-201],生成id:-201导致首次唯一索引冲突不报错，重新取缓存范围[-400,-301]
   for( var i = 0; i < 10; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -301 + i * increment } );
   }

   //coordC不指定自增字段插入
   for( var i = 0; i < 50; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -202 + i * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
