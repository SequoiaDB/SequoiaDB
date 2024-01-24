import( "../lib/index_commlib.js" );

// select primary node with a collection
function selectPrimaryNode( db, csName, clName )
{
   var groups = commGetCLGroups( db, csName + "." + clName ) ;
   var index = parseInt( Math.random() * groups.length ) ;
   println( "selected group " + groups[ index ] ) ;
   var node = db.getRG( groups[ index ] ).getMaster() ;
   println( "selected node " + node ) ;
   return node.connect() ;
}

function selectGroup( db, csName, clName )
{
   var groups = commGetCLGroups( db, csName + "." + clName ) ;
   return groups[ 0 ] ;
}

function getOneSample( db, csName, clName, ixName )
{
   var cl = db.getCS( "SYSSTAT" ).getCL( "SYSINDEXSTAT" ) ;
   var stat = cl.find( { CollectionSpace: csName, Collection: clName, Index: ixName } ).current().toObj() ;
   return stat.MCV.Values[ 1 ] ;
}

function randomString( maxLength )
{
   var arr = new Array( Math.round( Math.random() * maxLength ) )
   return arr.join( "a" )
}
