/******************************************************************************
@Description :    seqDB-17742: increment为负值，插入值比序列的MinValue稍大，MinValue为负边界值，再次插入自增字段未超MinValue 
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

   var clName = COMMCLNAME + "_17742";
   var increment = -2;
   var acquireSize = 100;
   var sortField = 0;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: {
         Field: "id", Increment: increment,
         AcquireSize: acquireSize, Cycled: true
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
      cl[i].insert( { a: sortField } );
      expRecs.push( { a: sortField, id: -1 + i * acquireSize * increment } );
      sortField++;
   }

   //coordB指定自增字段值比MinValue稍大插入记录,此时coord上的所有缓存被丢弃
   var insertR1 = { a: sortField, id: { "$numberLong": "-9223372036854775805" } };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );
   sortField++;

   //coordA不指定自增字段插入
   for( var i = 0; i < 50; i++ )
   {
      cl[0].insert( { a: sortField } );
      expRecs.push( { a: sortField, id: -3 + i * increment } );
      sortField++;
   }

   //coordB插入记录，插入成功，消耗完最后一个自增字段值
   cl[1].insert( { a: sortField } );
   expRecs.push( { a: sortField, id: { "$numberLong": "-9223372036854775807" } } );
   sortField++;

   //coordB再次插入生成自增字段值大于minValue因此翻转，重新取缓存范围[-200,-1],此时需要生成id值为-1,唯一索引冲突，
   //再次取缓存范围[-400,-201],此时生成id值为-201，再次唯一索引冲突后报错-38
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl[1].insert( { a: sortField } );
   } );

   //coordC不指定自增字段插入,消耗完本coord缓存
   for( var i = 0; i < 99; i++ )
   {
      cl[2].insert( { a: sortField } );
      expRecs.push( { a: sortField, id: -403 + i * increment } );
      sortField++;
   }

   //coordC不指定自增字段插入记录，获取新缓存[-600,-401],生成id:-401,首次唯一索引冲突不报错，重新获取缓存[-800,-601]
   for( var i = 0; i < 50; i++ )
   {
      cl[2].insert( { a: sortField } );
      expRecs.push( { a: sortField, id: -601 + i * increment } );
      sortField++;
   }

   var rc = dbcl.find().sort( { a: 1 } );
   expRecs.sort( compare( "a" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
