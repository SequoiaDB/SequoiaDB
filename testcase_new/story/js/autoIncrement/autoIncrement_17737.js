/******************************************************************************
@Description :    seqDB-17737:     increment为负值，插入值是序列的cachedValue 
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

   var clName = COMMCLNAME + "_17737";
   var increment = -1;
   var acquireSize = 100;
   var startValue = 350;
   var maxValue = 500;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      AutoIncrement: {
         Field: "id", Increment: increment,
         AcquireSize: acquireSize, StartValue: startValue, MaxValue: maxValue
      }
   } );
   commCreateIndex( dbcl, "a", { id: 1 }, { Unique: true } )

   //连接所有coord插入部分记录,coord缓存分别为[251,350],[151,250],[51,150]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
      cl[i].insert( { a: i } );
      expRecs.push( { a: i, id: 350 + i * increment * acquireSize } );
   }

   //coordB指定自增字段值为序列的cachedValue插入记录
   var insertR1 = { a: 300, id: 300 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA不指定自增字段插入记录
   for( var i = 0; i < 49; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: 349 + i * increment } );
   }

   //coordA不指定自增字段插入记录，由于冲突，获取新的缓存[-51,50]
   for( var i = 0; i < 99; i++ )
   {
      cl[0].insert( { a: i } );
      expRecs.push( { a: i, id: 50 + i * increment } );
   }

   //coordB不指定自增字段插入记录,耗尽本coord缓存
   for( var i = 0; i < 99; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: 249 + i * increment } );
   }

   //coordB不指定自增字段插入记录,获取新的缓存[-151,-50]
   for( var i = 0; i < 99; i++ )
   {
      cl[1].insert( { a: i } );
      expRecs.push( { a: i, id: -50 + i * increment } );
   }

   //coordC不指定自增字段插入记录，耗尽本coord缓存[51,150]
   for( var i = 0; i < 99; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: 149 + i * increment } );
   }

   //coordC不指定自增字段插入记录，获取新的缓存[-251,-150]
   for( var i = 0; i < 100; i++ )
   {
      cl[2].insert( { a: i } );
      expRecs.push( { a: i, id: -150 + i * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
