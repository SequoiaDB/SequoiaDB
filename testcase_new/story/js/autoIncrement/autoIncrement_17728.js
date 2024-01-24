/******************************************************************************
@Description :    seqDB-17728:increment为负值，插入值小于当前coord缓存范围，但在其它coord缓存范围内 
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

   var clName = COMMCLNAME + "_17728";
   var increment = -1;
   var acquireSize = 100;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", Increment: increment, AcquireSize: acquireSize } } );
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
      expRecs.push( { a: i, id: -1 + i * acquireSize * increment } );
   }

   //coordB指定自增字段值id:-5小于当前coord缓存范围，但在其它coord缓存范围内插入记录
   var insertR1 = { a: 5, id: -5 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //coordA插入记录，当自增字段值与coordB插入的记录冲突时，coordA丢弃缓存，重新获取新的缓存[-400，-301]
   for( var i = 0; i < 100; i++ )
   {
      cl[0].insert( { a: i } );
      if( i < 3 )
      {
         expRecs.push( { a: i, id: -2 + i * increment } );
      } else
      {
         expRecs.push( { a: i, id: -301 + ( i - 3 ) * increment } );
      }
   }

   //coordB、coordC不指定自增字段插入
   for( var i = 1; i < coordNodes.length; i++ )
   {
      cl[i].insert( { a: i } );
      expRecs.push( { a: i, id: - 2 + i * acquireSize * increment } );
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
