/******************************************************************************
@Description : seqDB-17896:Increment为负值，自增字段为嵌套字段，插入值小于所有coord缓存值
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

   var clName = COMMCLNAME + "_17896";
   var increment = -1;
   var acquireSize = 100;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "x.y.z", Increment: increment, AcquireSize: acquireSize } } );
   commCreateIndex( dbcl, "index", { "x.y.z": 1 }, { Unique: true } );

   //连接所有coord插入部分记录,coord缓存分别为[-100,-1],[-200,-101],[-300,-201]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
      cl[i].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -1 - i * acquireSize } } } );
   }

   //coordB指定自增字段值id:-301小于所有coord缓存的最小值插入记录，丢弃本coord缓存，获取新的缓存[-401,-302]
   var insertR1 = { a: 301, x: { y: { z: -301 } } };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA不指定自增字段插入记录，耗尽本coord缓存[-100,-1]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -2 + i * increment } } } );
   }

   //coordA不指定自增字段插入记录，获取新的缓存[-501,-402]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -402 + i * increment } } } );
   }

   //coordB不指定自增字段插入记录，耗尽本coord缓存[-401,-302]
   for( var i = 0; i < 100; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -302 + i * increment } } } );
   }

   //coordB不指定自增字段插入记录，获取新的缓存[-601,-502]
   for( var i = 0; i < 100; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -502 + i * increment } } } );
   }

   //coordC不指定自增字段插入记录，耗尽本coord缓存[-300,-201]
   for( var i = 0; i < 99; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -202 + i * increment } } } );
   }

   //coordC不指定自增字段插入记录，由于冲突，获取新的缓存[-701,-602]
   for( var i = 0; i < 100; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, x: { y: { z: -602 + i * increment } } } );
   }

   var rc = dbcl.find().sort( { _id: 1 } );
   expRecs.sort( compare( "_id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
