/***************************************************************************
@Description :seqDB-17710:increment为正值，插入值小于当前coord缓存范围，但在其它coord缓存范围内
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
   var clName = COMMCLNAME + "_17710";
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
      cl[k].insert( { a: sortField } );
      expR.push( { a: sortField, id: currentValue + acquireSize * k } );
      sortField++;
   }

   //指定自增字段值插入记录，插入值小于当前coord缓存范围，但在其它coord缓存范围内
   var insertValue = 5;
   var record = { id: insertValue, a: sortField };
   cl[1].insert( record );
   expR.push( record );
   sortField++;

   //coordA再次插入3条记录，不指定自增字段
   for( var i = 0; i < 3; i++ )
   {
      cl[0].insert( { a: sortField } );
      expR.push( { a: sortField, id: 2 + i } );
      sortField++;
   }

   //coordA再次插入1条记录，不指定自增字段，此时由于冲突，coordA丢弃缓存，重新获取新的缓存[34,44]
   cl[0].insert( { a: sortField } );
   expR.push( { a: sortField, id: 34 } );
   sortField++;

   //coordB再次插入，不指定自增字段
   cl[1].insert( { a: sortField } );
   expR.push( { a: sortField, id: 13 } );
   sortField++;

   //coordC再次插入，不指定自增字段
   cl[2].insert( { a: sortField } );
   expR.push( { a: sortField, id: 24 } );
   sortField++;

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
