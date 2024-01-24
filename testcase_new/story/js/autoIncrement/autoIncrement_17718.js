/***************************************************************************
@Description :seqDB-17718：increment为正值，插入值是序列的currentValue
@Modify list :
              2019-1-25  zhaoyu  Create
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
   var clName = COMMCLNAME + "_17718";
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

   //coordB指定自增字段插入记录，插入值是序列的currentValue:1001，catalog重新生成新的缓存[1002,2002],coordB重新获取新缓存
   cl[1].insert( { a: sortField, id: 1001 } );
   expR.push( { a: sortField, id: 1001 } );

   //coordA插入记录，消耗完本coord的缓存，[1,11]
   for( var i = 0; i < 8; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 4 + i } );
      sortField++;
   }

   //coordA插入记录，不指定自增字段，重新从catalog或者新缓存,[1013,1023]
   for( var i = 0; i < 2; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 1013 + i } );
      sortField++;
   }

   //coordB插入记录，不指定自增字段，重新从catalog或者新缓存,[1002,1012]
   for( var i = 0; i < 2; i++ )
   {
      cl[1].insert( { a: sortField } );
      expR.push( { a: sortField, id: 1002 + i } );
      sortField++;
   }

   //coordC插入记录，消耗完本coord的缓存，[23,33]
   for( var i = 0; i < 8; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 26 + i } );
      sortField++;
   }

   //coordC插入记录，不指定自增字段，重新从catalog或者新缓存,[1024,1034]
   for( var i = 0; i < 2; i++ )
   {
      cl[2].insert( { a: sortField } );
      expR.push( { a: sortField, id: 1024 + i } );
      sortField++;
   }

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
