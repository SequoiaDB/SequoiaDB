/***************************************************************************
@Description :seqDB-17892:Increment为正值，自增字段为嵌套字段，插入值小于所有coord缓存值
@Modify list :
              2019-2-21  Zhao Xiaoni  Create
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
   var clName = COMMCLNAME + "_17892";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "x.y.z", AcquireSize: acquireSize, CacheSize: cacheSize } } );
   commCreateIndex( dbcl, "index", { "x.y.z": 1 }, { Unique: true }, true );

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
         expR.push( { a: sortField, x: { y: { z: currentValue + Math.ceil( 3 / acquireSize ) * acquireSize * increment * k + increment * i } } } );
         sortField++;
      }
      cl[k].insert( doc );
   }

   //coordB插入记录，指定自增字段，插入值小于所有缓存值
   var insertValue = -100;
   var record = { x: { y: { z: insertValue } }, a: sortField };
   cl[1].insert( record );
   expR.push( record );
   sortField++;

   //coordA再次插入，消耗本coord缓存，不指定自增字段,[1,11]
   for( var i = 0; i < 8; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, x: { y: { z: 4 + i } } } );
      sortField++;
   }

   //coordA再次插入，从catalog重新获取缓存，不指定自增字段,[34,44]
   for( var i = 0; i < 2; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, x: { y: { z: 34 + i } } } );
      sortField++;
   }

   //coordB再次插入记录，消耗本coord缓存，不指定自增字段,[12,22]
   for( var i = 0; i < 8; i++ )
   {
      cl[1].insert( { a: sortField } );
      expR.push( { a: sortField, x: { y: { z: 15 + i } } } );
      sortField++;
   }

   //coordB再次插入，从catalog重新获取缓存，不指定自增字段,[45,55]
   for( var i = 0; i < 2; i++ )
   {
      cl[1].insert( { a: sortField } );
      expR.push( { a: sortField, x: { y: { z: 45 + i } } } );
      sortField++;
   }

   //coordC再次插入，消耗本coord缓存，不指定自增字段,[23,33]
   for( var i = 0; i < 8; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, x: { y: { z: 26 + i } } } );
      sortField++;
   }

   //coordC再次插入，从catalog重新获取缓存，不指定自增字段,[56,66]
   for( var i = 0; i < 2; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, x: { y: { z: 56 + i } } } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
