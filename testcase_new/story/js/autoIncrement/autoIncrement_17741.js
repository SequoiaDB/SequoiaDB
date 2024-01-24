/******************************************************************************
@Description :    seqDB-17741:increment为负值，插入值比序列的MinValue稍大，再次插入自增字段超过MinValue  
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

   var clName = COMMCLNAME + "_17741";
   var increment = -2;
   var cacheSize = 500;
   var acquireSize = 100;
   var minValue = -1000;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: {
         Field: "id", Increment: increment,
         CacheSize: 500, AcquireSize: acquireSize, MinValue: minValue, Cycled: true
      }
   } );
   commCreateIndex( dbcl, "a", { id: 1 }, { Unique: true } );

   //连接所有coord插入部分记录,coord缓存分别为[-200,-1],[-400,-201],[-600,-401]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
   }
   cl[1].insert( { a: 1 } );
   expRecs.push( { a: 1, id: -1 } );

   //coordB指定自增字段值比MinValue稍大插入记录,此时coord上的所有缓存被丢弃
   var insertR1 = { a: 999, id: -999 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordB不指定自增字段再次插入记录，翻转获取缓存[-200,-1],生成id:-1导致唯一索引冲突，再次取缓存范围[-400,-201],自增字段生成值由201开始
   for( var i = 0; i < 20; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -201 + i * increment } );
   }

   //coordA不指定自增字段插入
   for( var i = 0; i < 50; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: -401 + i * increment } );
   }

   //coordC不指定自增字段插入
   for( var i = 0; i < 50; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -601 + i * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
