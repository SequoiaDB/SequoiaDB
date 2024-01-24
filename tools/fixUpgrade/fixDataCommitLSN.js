// fixDataCommitLSN
//
// usage
// sdb -e "var db = new Sdb( 'localhost', 50000 ) ;" -f fixDataCommitLSN.js
//
// testMode: only fix one collection on one group
// sdb -e "var db = new Sdb( 'localhost, 50000 ) ; var testMode = true ;" -f fixDataCommitLSN.js
//
// onErrContinue: skip errors
// sdb -e "var db = new Sdb( 'localhost', 50000 ) ; var onErrContinue = true ;" -f fixDataCommitLSN.js
//
// dummyKey: change dummy _id to generate temporary record
// sdb -e "var db = new Sdb( 'localhost', 50000 ) ; var dummyKey = \"ANOTHER_DUMMY_KEY\"" -f fixDataCommitLSN.js
//
// verbose: print log in verbose
// sdb -e "var db = new Sdb( 'localhost', 50000 ) ; var verbose = true ;" -f fixDataCommitLSN.js

// check if test mode
function _isTestMode()
{
   if ( "undefined" == typeof( testMode ) )
   {
      return false ;
   }
   return testMode ;
}

// check if on error continue
function _isOnErrContinue()
{
   if ( "undefined" == typeof( onErrContinue ) )
   {
      return false ;
   }
   return onErrContinue ;
}

// get dummy key for temporary record
function _getDummyKey()
{
   if ( "undefined" == typeof( dummyKey ) )
   {
      return "SYSDUMMY" ;
   }
   return dummyKey ;
}

// check if print log in verbose
function _isVerbose()
{
   if ( "undefined" == typeof( verbose ) )
   {
      return false ;
   }
   return verbose ;
}

// get catalog info for given collection
function _getCataInfo( collFullName )
{
   var cataCursor = db.snapshot( SDB_SNAP_CATALOG, { "Name" : collFullName } ) ;
   if ( cataCursor.next() )
   {
      return cataCursor.current().toObj().ShardingKey ;
   }
   throw "collection [" + collFullName + "] is not found in catalog" ;
}

// get temporary object for hash sharding empty collection
function _getTmpObjFromHash( collFullName, groupName, cataObject )
{
   var partition = cataObject.Partition ;
   var shardingKey = cataObject.ShardingKey ;
   var csName = collFullName.split( '.' )[ 0 ] ;
   var clName = collFullName.split( '.' ).slice( 1 ).join( '.' ) ;
   // try to iterate partitions for sharding key and explain query to check if
   // query key is on given group
   var keyCount = 0 ;
   var query = {} ;
   var lastKey = "" ;
   for ( let key  in shardingKey )
   {
      query[ key ] = 0 ;
      lastKey = key ;
      ++ keyCount ;
   }
   // retry for partition * key count times
   for ( i = 0 ; i < partition * keyCount ; i ++ )
   {
      query[ lastKey ] = i ;
      var explainCursor = db.getCS( csName ).getCL( clName ).find( query ).explain() ;
      while ( explainCursor.next() )
      {
         var queryGroup = explainCursor.current().toObj().GroupName ;
         if ( queryGroup == groupName )
         {
            return query ;
         }
      }
   }
   throw "sharding key for collection [" + collFullName + "] on group [" + groupName + "] is not found" ;
}

// get temporary object for range sharding empty collection
function _getTmpObjFromRange( collFullName, groupName, cataObject )
{
   // use the low bound of given group for temporary object
   var cataInfo = cataObject.CataInfo ;
   for ( i = 0 ; i < cataInfo.length ; i ++ )
   {
      if ( cataInfo[ i ].GroupName == groupName )
      {
         return cataInfo[ i ].LowBound ;
      }
   }
   throw "sharding key for collection [" + collFullName + "] on group [" + groupName + "] is not found" ;
}

// try to get temporary object from catalog info for empty collection
function _getTmpObjFromCata( collFullName, groupName )
{
   var cataCursor = db.snapshot( SDB_SNAP_CATALOG, { "Name" : collFullName } ) ;
   if ( cataCursor.next() )
   {
      cataObject = cataCursor.current().toObj() ;
      // check sharding type
      var shardingType = cataObject.ShardingType ;
      if ( shardingType == "hash" )
      {
         if ( _isVerbose() )
         {
            println( "collection [" + collFullName + "] is hash sharding" ) ;
         }
         return _getTmpObjFromHash( collFullName, groupName, cataObject ) ;
      }
      else if ( shardingType = "range" )
      {
         if ( _isVerbose() )
         {
            println( "collection [" + collFullName + "] is range sharding" ) ;
         }
         return _getTmpObjFromRange( collFullName, groupName, cataObject ) ;
      }
      else
      {
         if ( _isVerbose() )
         {
            println( "collection [" + collFullName + "] is not sharding" ) ;
         }
         return {} ;
      }
   }
   throw "collection [" + collFullName + "] on group [" + groupName + "] is not found in catalog" ;
}

function _tryFixLSN( csName, clName, shardingObj, isEmpty )
{
   if ( undefined === shardingObj[ "_id" ] )
   {
      // no _id field, add _id field to temporary object
      // insert temporary object, then remove
      shardingObj[ "_id" ] = _getDummyKey() ;
      _insertAndRemoveTmpObj( csName, clName, shardingObj ) ;
   }
   else if ( isEmpty )
   {
      // has _id field in sharding key, but collection is empty
      // insert temporary object, then remove
      _insertAndRemoveTmpObj( csName, clName, shardingObj ) ;
   }
   else
   {
      // has _id field, update record with temporary field then remove
      tmpField = "__TEMP_FIELD__" + _getDummyKey() ;
      // test if origin data contains temporary field
      var dataCursor = db.getCS( csName ).getCL( clName ).find( shardingObj ).limit( 1 ).hint( { "" : "$id" } ) ;
      if ( dataCursor.next() )
      {
         curObj = dataCursor.current().toObj() ;
         while ( undefined !== curObj[ tmpField ] )
         {
            tmpField = tmpField + "_DUMMY_" ;
         }
      }
      dataCursor.close() ;
      _updateTmpField( csName, clName, shardingObj, tmpField ) ;
   }
}

// insert and remove temporary object
function _insertAndRemoveTmpObj( csName, clName, shardingObj )
{
   // check if contains _id
   if ( undefined === shardingObj[ "_id" ] )
   {
      throw( "failed to insert temporary object " + JSON.stringify( shardingObj ) + ", no _id field" ) ;
   }
   // insert temporary object
   try
   {
      db.getCS( csName ).getCL( clName ).insert( shardingObj ) ;
   }
   catch ( e )
   {
      throw( "failed to insert temporary object " + JSON.stringify( shardingObj ) + ", rc: " + e ) ;
   }
   if ( _isVerbose() )
   {
      println( "INSERT collecton [" + csName + "." + clName + "] object " + JSON.stringify( shardingObj ) ) ;
   }
   // remove temporary object
   try
   {
      db.getCS( csName ).getCL( clName ).remove( shardingObj, { "" : "$id" } ) ;
   }
   catch ( e )
   {
      throw( "failed to remove temporary object " + JSON.stringify( shardingObj ) + ", rc: " + e ) ;
   }
   if ( _isVerbose() )
   {
      println( "REMOVE collecton [" + csName + "." + clName + "] object " + JSON.stringify( shardingObj ) ) ;
   }
}

// update object with temporary field
function _updateTmpField( csName, clName, shardingObj, tmpField )
{
   // check if contains _id
   if ( undefined === shardingObj[ "_id" ] )
   {
      throw( "failed to update temporary object " + JSON.stringify( shardingObj ) + ", no _id field" ) ;
   }
   // update to add temporary field
   try
   {
      db.getCS( csName ).getCL( clName ).update( { "$set" : { tmpField : 1 } }, shardingObj, { "" : "$id" } )
   }
   catch ( e )
   {
      throw( "failed to update object " + JSON.stringify( shardingObj ) + " to add field " + tmpField + ", rc: " + e ) ;
   }
   if ( _isVerbose() )
   {
      println( "UPDATE collection [" + csName + "." + clName + "] object " + JSON.stringify( shardingObj ) + " to add field " + tmpField ) ;
   }
   // update to remove temporary field
   try
   {
      db.getCS( csName ).getCL( clName ).update( { "$unset" : { tmpField : 1 } }, shardingObj, { "" : "$id" } )
   }
   catch ( e )
   {
      throw( "failed to update object " + JSON.stringify( shardingObj ) + " to remove field " + tmpField + ", rc: " + e ) ;
   }
   if ( _isVerbose() )
   {
      println( "UPDATE collection [" + csName + "." + clName + "] object " + JSON.stringify( shardingObj ) + " to remove field " + tmpField ) ;
   }
}

// fix one collection for one group
function _fixCollGroup( curColl, curGroup, curNode )
{
   var csName = curColl.split( '.' )[ 0 ] ;
   var clName = curColl.split( '.' ).slice( 1 ).join( '.' ) ;

    // get sharding
   var shardingKey = {}
   try
   {
      shardingKey = _getCataInfo( curColl ) ;
   }
   catch ( e )
   {
      println( "ERROR: failed to get catalog for [" + curColl + "], error: " + e ) ;
      throw e ;
   }

   if ( undefined === shardingKey ||
        Object.keys( shardingKey ).length === 0 )
   {
      // non sharding collection, just insert a empty record
      if ( _isVerbose() )
      {
         println( "collection [" + curColl + "] empty sharding key" ) ;
      }
      try
      {
         _tryFixLSN( csName, clName, {}, false )
      }
      catch ( e )
      {
         println( "ERROR: failed to fix collection [" + curColl + "] on group [" + curGroup + "], error: " + e ) ;
         throw e ;
      }
   }
   else
   {
      // sharding collection
      // try to fetch one record from current group
      var found = false ;
      var succeed = true ;
      if ( _isVerbose() )
      {
         println( "query from node [" + curNode + "], sharding key " + JSON.stringify( shardingKey ) ) ;
      }
      var data = new Sdb( curNode ) ;
      var dataCursor = data.getCS( csName ).getCL( clName ).find( {}, shardingKey ).limit( 1 ) ;
      if ( dataCursor.next() )
      {
         // found record, use the same sharding key to insert temporary record
         found = true ;
         var shardingObj = dataCursor.current().toObj() ;
         try
         {
            _tryFixLSN( csName, clName, shardingObj, false )
         }
         catch ( e )
         {
            println( "ERROR: failed to fix collection [" + curColl + "] on group [" + curGroup + "], error: " + e ) ;
            data.close() ;
            throw e ;
         }
      }
      data.close() ;
      if ( !found )
      {
         // no record is found, try get sharding key from catalog
         var shardingObj = {} ;
         if ( _isVerbose() )
         {
            println( "collection [" + curColl + "] empty on group [" + curGroup + "]" ) ;
         }
         try
         {
            shardingObj = _getTmpObjFromCata( curColl, curGroup ) ;
         }
         catch ( e )
         {
            println( "ERROR: failed to get catalog for [" + curColl + "], error: " + e ) ;
            throw e ;
         }
         try
         {
            _tryFixLSN( csName, clName, shardingObj, true )
         }
         catch ( e )
         {
            println( "ERROR: failed to fix collection [" + curColl + "] on group [" + curGroup + "], error: " + e ) ;
            throw e ;
         }
      }
   }
}

// fix DataCommitLSN with -1
// insert and remove one temporary record to collection on each group to make a valid DataCommitLSN
function fixDataCommitLSN()
{
   var collCursor = db.snapshot( SDB_SNAP_COLLECTIONS,
                                 { "RawData" : true, "Details.DataCommitLSN" : { $et : -1 } },
                                 { "Name" : 1, "Details.NodeName" : 1, "Details.GroupName" : 1 },
                                 { "Name" : 1, "Details.GroupName" : 1 } ) ;

   var curColl = "" ;
   var curGroup = "" ;
   var fixCount = 0 ;
   // iterate each collection on each group
   while ( collCursor.next() )
   {
      var coll = collCursor.current().toObj() ;

      if ( coll.Name != curColl )
      {
         // new collection
         curColl = coll.Name ;
         curGroup = coll.Details[ 0 ].GroupName ;
      }
      else if ( coll.Details[ 0 ].GroupName == curGroup )
      {
         // same group skip
         continue ;
      }
      curGroup = coll.Details[ 0 ].GroupName ;
      var curNode = coll.Details[ 0 ].NodeName ;
      println( "=======================" ) ;
      println( "start to fix collection [" + curColl + "] on group [" + curGroup + "]" ) ;
      try
      {
         _fixCollGroup( curColl, curGroup, curNode ) ;
      }
      catch ( e )
      {
         println( "ERROR: failed to fix collection [" + curColl + "] on group [" + curGroup + "], error: " + e ) ;
         // error happened, check if we could continue
         if ( !_isOnErrContinue() )
         {
            println( "ERROR: on error stop" ) ;
            break ;
         }
         else
         {
            println( "WARNING: on error skip and continue" ) ;
            continue ;
         }
      }

      println( "finish to fix collection [" + curColl + "] on group [" + curGroup + "]" ) ;
      ++ fixCount ;
      if ( _isTestMode() )
      {
         break ;
      }
   }

   // flush commit LSN
   db.sync() ;

   println( "fix " + fixCount + " " + ( fixCount > 1 ? "collections" : "collection" ) ) ;
}

fixDataCommitLSN() ;
