if ( tmpSdbCollection == undefined )
{
   var tmpSdbCollection = {
      _bulkInsert: SdbCollection.prototype._bulkInsert,
      _count: SdbCollection.prototype._count,
      _getIndexes: SdbCollection.prototype._getIndexes,
      _insert: SdbCollection.prototype._insert,
      aggregate: SdbCollection.prototype.aggregate,
      alter: SdbCollection.prototype.alter,
      attachCL: SdbCollection.prototype.attachCL,
      copyIndex: SdbCollection.prototype.copyIndex,
      copyIndexAsync: SdbCollection.prototype.copyIndexAsync,
      count: SdbCollection.prototype.count,
      createAutoIncrement: SdbCollection.prototype.createAutoIncrement,
      createIdIndex: SdbCollection.prototype.createIdIndex,
      createIndex: SdbCollection.prototype.createIndex,
      createIndexAsync: SdbCollection.prototype.createIndexAsync,
      createLobID: SdbCollection.prototype.createLobID,
      deleteLob: SdbCollection.prototype.deleteLob,
      detachCL: SdbCollection.prototype.detachCL,
      disableCompression: SdbCollection.prototype.disableCompression,
      disableSharding: SdbCollection.prototype.disableSharding,
      dropAutoIncrement: SdbCollection.prototype.dropAutoIncrement,
      dropIdIndex: SdbCollection.prototype.dropIdIndex,
      dropIndex: SdbCollection.prototype.dropIndex,
      dropIndexAsync: SdbCollection.prototype.dropIndexAsync,
      enableCompression: SdbCollection.prototype.enableCompression,
      enableSharding: SdbCollection.prototype.enableSharding,
      explain: SdbCollection.prototype.explain,
      find: SdbCollection.prototype.find,
      findOne: SdbCollection.prototype.findOne,
      getCollectionStat: SdbCollection.prototype.getCollectionStat,
      getDetail: SdbCollection.prototype.getDetail,
      getIndex: SdbCollection.prototype.getIndex,
      getIndexStat: SdbCollection.prototype.getIndexStat,
      getLob: SdbCollection.prototype.getLob,
      getLobDetail: SdbCollection.prototype.getLobDetail,
      getQueryMeta: SdbCollection.prototype.getQueryMeta,
      help: SdbCollection.prototype.help,
      insert: SdbCollection.prototype.insert,
      listIndexes: SdbCollection.prototype.listIndexes,
      listLobPieces: SdbCollection.prototype.listLobPieces,
      listLobs: SdbCollection.prototype.listLobs,
      pop: SdbCollection.prototype.pop,
      putLob: SdbCollection.prototype.putLob,
      rawFind: SdbCollection.prototype.rawFind,
      remove: SdbCollection.prototype.remove,
      setAttributes: SdbCollection.prototype.setAttributes,
      setConsistencyStrategy: SdbCollection.prototype.setConsistencyStrategy,
      snapshotIndexes: SdbCollection.prototype.snapshotIndexes,
      split: SdbCollection.prototype.split,
      splitAsync: SdbCollection.prototype.splitAsync,
      toString: SdbCollection.prototype.toString,
      truncate: SdbCollection.prototype.truncate,
      truncateLob: SdbCollection.prototype.truncateLob,
      update: SdbCollection.prototype.update,
      upsert: SdbCollection.prototype.upsert
   };
}
var funcSdbCollection = ( funcSdbCollection == undefined ) ? SdbCollection : funcSdbCollection;
var funcSdbCollectionhelp = funcSdbCollection.help;
SdbCollection=function(){try{return funcSdbCollection.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbCollection.help = function(){try{ return funcSdbCollectionhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
SdbCollection.prototype._bulkInsert=function(){try{return tmpSdbCollection._bulkInsert.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype._count=function(){try{return tmpSdbCollection._count.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype._getIndexes=function(){try{return tmpSdbCollection._getIndexes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype._insert=function(){try{return tmpSdbCollection._insert.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.aggregate=function(){try{return tmpSdbCollection.aggregate.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.alter=function(){try{return tmpSdbCollection.alter.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.attachCL=function(){try{return tmpSdbCollection.attachCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.copyIndex=function(){try{return tmpSdbCollection.copyIndex.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.copyIndexAsync=function(){try{return tmpSdbCollection.copyIndexAsync.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.count=function(){try{return tmpSdbCollection.count.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.createAutoIncrement=function(){try{return tmpSdbCollection.createAutoIncrement.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.createIdIndex=function(){try{return tmpSdbCollection.createIdIndex.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.createIndex=function(){try{return tmpSdbCollection.createIndex.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.createIndexAsync=function(){try{return tmpSdbCollection.createIndexAsync.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.createLobID=function(){try{return tmpSdbCollection.createLobID.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.deleteLob=function(){try{return tmpSdbCollection.deleteLob.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.detachCL=function(){try{return tmpSdbCollection.detachCL.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.disableCompression=function(){try{return tmpSdbCollection.disableCompression.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.disableSharding=function(){try{return tmpSdbCollection.disableSharding.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.dropAutoIncrement=function(){try{return tmpSdbCollection.dropAutoIncrement.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.dropIdIndex=function(){try{return tmpSdbCollection.dropIdIndex.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.dropIndex=function(){try{return tmpSdbCollection.dropIndex.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.dropIndexAsync=function(){try{return tmpSdbCollection.dropIndexAsync.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.enableCompression=function(){try{return tmpSdbCollection.enableCompression.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.enableSharding=function(){try{return tmpSdbCollection.enableSharding.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.explain=function(){try{return tmpSdbCollection.explain.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.find=function(){try{return tmpSdbCollection.find.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.findOne=function(){try{return tmpSdbCollection.findOne.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getCollectionStat=function(){try{return tmpSdbCollection.getCollectionStat.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getDetail=function(){try{return tmpSdbCollection.getDetail.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getIndex=function(){try{return tmpSdbCollection.getIndex.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getIndexStat=function(){try{return tmpSdbCollection.getIndexStat.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getLob=function(){try{return tmpSdbCollection.getLob.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getLobDetail=function(){try{return tmpSdbCollection.getLobDetail.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.getQueryMeta=function(){try{return tmpSdbCollection.getQueryMeta.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.help=function(){try{return tmpSdbCollection.help.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.insert=function(){try{return tmpSdbCollection.insert.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.listIndexes=function(){try{return tmpSdbCollection.listIndexes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.listLobPieces=function(){try{return tmpSdbCollection.listLobPieces.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.listLobs=function(){try{return tmpSdbCollection.listLobs.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.pop=function(){try{return tmpSdbCollection.pop.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.putLob=function(){try{return tmpSdbCollection.putLob.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.rawFind=function(){try{return tmpSdbCollection.rawFind.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.remove=function(){try{return tmpSdbCollection.remove.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.setAttributes=function(){try{return tmpSdbCollection.setAttributes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.setConsistencyStrategy=function(){try{return tmpSdbCollection.setConsistencyStrategy.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.snapshotIndexes=function(){try{return tmpSdbCollection.snapshotIndexes.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.split=function(){try{return tmpSdbCollection.split.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.splitAsync=function(){try{return tmpSdbCollection.splitAsync.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.toString=function(){try{return tmpSdbCollection.toString.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.truncate=function(){try{return tmpSdbCollection.truncate.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.truncateLob=function(){try{return tmpSdbCollection.truncateLob.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.update=function(){try{return tmpSdbCollection.update.apply(this,arguments);}catch(e){throw new Error(e);}};
SdbCollection.prototype.upsert=function(){try{return tmpSdbCollection.upsert.apply(this,arguments);}catch(e){throw new Error(e);}};
