/****************************************************
@description: seqDB-6079:创建{a.b:1}索引_ST.index.001 
@author:
              2015-11-9 TingYU
****************************************************/


main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME;
   var cl = readyCL( csName, clName );

   cl.insert( { a: [{ b: 0 }, { c: 1 }] } );
   cl.insert( { a: [{ b: 2 }, { c: 3 }] } );
   cl.insert( { a: [5, 6, 7, 8, 9] } );

   var idxName = 'aIdx';
   commCreateIndex( cl, idxName, { "a.b": -1 } );

   var rc = cl.find( { "a.$1.b": 2 } );
   var expRecs = [{ a: [{ b: 2 }, { c: 3 }] }];
   checkRec( rc, expRecs );
   checkExplain( rc, idxName );

   clean( csName, clName );
}

function readyCL ( csName, clName, option )
{

   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   cl = commCreateCL( db, csName, clName, option, true, false, "create cl in begin" );

   return cl;
}

function checkExplain ( rc, idxName )
{
   var plan = rc.explain().current().toObj();
   if( ( plan.ScanType === "ixscan" ) && ( plan.IndexName === idxName ) )
   {	//ok
   }
   else
   {
      throw new Error( "checkExplain() fail,query.explain()" +
         "ScanType:ixscan, IndexName:" + idxName +
         "ScanType:" + plan.ScanType + ", IndexName:" + plan.IndexName );
   }
}


function clean ( csName, clName )
{

   commDropCL( db, csName, clName, true, true, "drop cl in clean" );
}


