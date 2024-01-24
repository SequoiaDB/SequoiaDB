/******************************************************************************
@Description :    seqDB-17735:    increment为负值，插入值是当前coord的nextValue 
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

   var clName = COMMCLNAME + "_17735";
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

   //coordB指定自增字段值为本coord缓存的nextValue插入记录
   var insertR1 = { a: 102, id: -102 };
   cl[1].insert( insertR1 );
   expRecs.push( insertR1 );

   //连接所有coord插入记录,其中coordB从userValue开始
   for( var i = 0; i < coordNodes.length; i++ )
   {
      for( var j = 0; j < 50; j++ )
      {
         cl[i].insert( { a: j } );
         if( i == 1 )
         {
            expRecs.push( { a: j, id: -103 + j * increment } );
         } else
         {
            expRecs.push( { a: j, id: - 2 + j * increment + i * acquireSize * increment } );
         }
      }
   }

   var rc = dbcl.find().sort( { id: 1 } );
   expRecs.sort( compare( "id" ) );
checkRec( rc, expRecs );

   commDropCL( db, COMMCSNAME, clName );
}
