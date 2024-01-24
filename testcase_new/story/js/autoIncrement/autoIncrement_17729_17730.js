/******************************************************************************
@Description :    seqDB-17729:increment为负值，插入值落在当前coord缓存范围，但小于nextValue 
                  seqDB-17730: increment为负值，插入值落在当前coord缓存范围，且大于等于nextValue
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

   var clName = COMMCLNAME + "_17729_17730";
   var increment = -1;
   var acquireSize = 100;

   commDropCL( db, COMMCSNAME, clName );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: "id", Increment: increment, AcquireSize: acquireSize } } );
   commCreateIndex( dbcl, "a", { id: 1 }, { Unique: true } )

   //连接所有coord插入部分记录,coord缓存分别为[-100,-1],[-200,-101],[-300,-201]
   var expRecs = [];
   var cl = new Array();
   var coord = new Array();
   for( var i = 0; i < coordNodes.length; i++ )
   {
      coord[i] = new Sdb( coordNodes[i] );
      cl[i] = coord[i].getCS( COMMCSNAME ).getCL( clName );
      cl[i].insert( { a: i } );
      expRecs.push( { a: i, id: -1 + i * increment * acquireSize } );
   }

   //coordB指定自增字段值id:-150落在当前coord缓存范围，且大于或等于nextValue插入记录
   var insertR1 = { a: 150, id: -150 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //连接所有coord插入部分记录
   for( var i = 0; i < coordNodes.length; i++ )
   {
      cl[i].insert( { a: i } );
      if( i == 1 )
      {
         expRecs.push( { a: 1, id: -151 } );
      } else
      {
         expRecs.push( { a: i, id: -2 + i * increment * acquireSize } );
      }
   }

   //coordB指定自增字段值id:-105落在当前coord缓存范围，但小于nextValue插入记录
   var insertR2 = { a: 105, id: -105 };
   cl[1].insert( insertR2 );
   expRecs.push( insertR2 );

   //连接所有coord插入部分记录
   for( var i = 0; i < coordNodes.length; i++ )
   {
      cl[i].insert( { a: i } );
      if( i == 1 )
      {
         expRecs.push( { a: 1, id: -152 } );
      } else
      {
         expRecs.push( { a: i, id: -3 + i * increment * acquireSize } );
      }
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
   checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
