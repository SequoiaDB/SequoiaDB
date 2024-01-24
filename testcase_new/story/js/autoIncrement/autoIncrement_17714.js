/***************************************************************************
@Description :seqDB-17714:increment为正值，插入值大于所有缓存值
@Modify list :
              2019-1-24  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   if( commIsStandalone( db ) || coordNum !== 3 )
   {
      return;
   }
   var sortField = 0;
   var increment = 1;
   var currentValue = 1
   var cacheSize = 1000;
   var acquireSize = 11;
   var clName = COMMCLNAME + "_17714";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", AcquireSize: acquireSize, CacheSize: cacheSize } } );
   commCreateIndex( dbcl, "id", { id: 1 }, { Unique: true }, true );

   var expR = [];
   var cl = new Array();
   var coord = new Array();
   for( var k = 0; k < coordNum; k++ )
   {
      coord[k] = new Sdb( coordNodes[k] );
      cl[k] = coord[k].getCS( COMMCSNAME ).getCL( clName );
      //连接所有coord插入部分记录,coord缓存分别为[1,11],[12,22],[23,33]
      var doc = [];
      for( var i = 0; i < 3; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id: currentValue + Math.ceil( 3 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl[k].insert( doc );
   }

   //coordB指定自增字段值插入记录，插入值大于所有缓存值,此时丢弃本coord上自增字段缓存，获取新缓存，[37,47]
   var insertValue = 36;
   var record = { id: insertValue, a: sortField };
   cl[1].insert( record );
   expR.push( record );
   sortField++;

   //coordA再次插入，消耗本coord缓存，不指定自增字段,[1,11]
   for( var i = 0; i < 8; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 4 + i } );
      sortField++;
   }

   //coordA再次插入，从catalog重新获取缓存，不指定自增字段,[48,58]
   for( var i = 0; i < 2; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 48 + i } );
      sortField++;
   }

   //coordB再次插入记录，使用新缓存
   cl[1].insert( { a: sortField } );
   expR.push( { a: sortField, id: 37 } );
   sortField++;

   //coordC再次插入，消耗本coord缓存，不指定自增字段,[23,33]
   for( var i = 0; i < 8; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 26 + i } );
      sortField++;
   }

   //coordC再次插入，从catalog重新获取缓存，不指定自增字段,[59,69]
   for( var i = 0; i < 2; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 59 + i } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
