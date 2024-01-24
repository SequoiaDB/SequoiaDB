/***************************************************************************
@Description :seqDB-17882:Increment为正值，自增字段为嵌套字段，插入记录
@Modify list :
              2019-2-19  Zhao Xiaoni  Create
****************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   if( coordNodes.length < 3 || commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_17882";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 1;
   var acquireSize = 100;

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "a.b.c", AcquireSize: acquireSize } } );
   dbcl.createIndex( "index", { "a.b.c": 1 }, true );

   //连接所有coord插入部分记录,coord缓存分别为[1,100],[101,200],[201,300]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
      cl[i].insert( { h: i } );
      expRecs.push( { h: i, a: { b: { c: 1 + i * acquireSize * increment } } } );
   }

   //coordB指定自增字段值id:5小于当前coord缓存范围，但在其它coord缓存范围内插入记录
   var insertR1 = { h: 5, a: { b: { c: 5 } } };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA插入记录，当自增字段值与coordB插入的记录冲突时，coordA丢弃缓存，重新获取新的缓存[301,401]
   for( var i = 0; i < 100; i++ )
   {
      cl[0].insert( { h: i } );
      if( i < 3 )
      {
         expRecs.push( { h: i, a: { b: { c: 2 + i * increment } } } );
      } else
      {
         expRecs.push( { h: i, a: { b: { c: 301 + ( i - 3 ) * increment } } } );
      }
   }

   //coordB、coordC不指定自增字段插入
   for( var i = 1; i < coordNodes.length; i++ )
   {
      cl[i].insert( { h: i } );
      expRecs.push( { h: i, a: { b: { c: 2 + i * acquireSize * increment } } } );
   }

   var rc = dbcl.find().sort( { "_id": 1 } );
   expRecs.sort( compare( "_id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
