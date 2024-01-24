/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/
// Global Constants
const SDB_PAGESIZE_4K              = 4096 ;
const SDB_PAGESIZE_8K              = 8192 ;
const SDB_PAGESIZE_16K             = 16384 ;
const SDB_PAGESIZE_32K             = 32768 ;
const SDB_PAGESIZE_64K             = 65536 ;
const SDB_PAGESIZE_DEFAULT         = SDB_PAGESIZE_64K ;

const SDB_SNAP_CONTEXTS            = 0 ;
const SDB_SNAP_CONTEXTS_CURRENT    = 1 ;
const SDB_SNAP_SESSIONS            = 2 ;
const SDB_SNAP_SESSIONS_CURRENT    = 3 ;
const SDB_SNAP_COLLECTIONS         = 4 ;
const SDB_SNAP_COLLECTIONSPACES    = 5 ;
const SDB_SNAP_DATABASE            = 6 ;
const SDB_SNAP_SYSTEM              = 7 ;
const SDB_SNAP_CATALOG             = 8 ;
const SDB_SNAP_TRANSACTIONS        = 9 ;
const SDB_SNAP_TRANSACTIONS_CURRENT= 10 ;
const SDB_SNAP_ACCESSPLANS         = 11 ;
const SDB_SNAP_HEALTH              = 12 ;
const SDB_SNAP_CONFIGS             = 13 ;
const SDB_SNAP_SVCTASKS            = 14 ;
const SDB_SNAP_SEQUENCES           = 15 ;
const SDB_SNAP_QUERIES             = 18 ;
const SDB_SNAP_LATCHWAITS          = 19 ;
const SDB_SNAP_LOCKWAITS           = 20 ;
const SDB_SNAP_INDEXSTATS          = 21 ;
const SDB_SNAP_TASKS               = 23 ;
// const SDB_SNAP_INDEXES = 24, for internal use only
const SDB_SNAP_TRANSWAITS          = 25 ;
const SDB_SNAP_TRANSDEADLOCK       = 26 ;
const SDB_SNAP_RECYCLEBIN          = 27 ;

const SDB_LIST_CONTEXTS            = 0 ;
const SDB_LIST_CONTEXTS_CURRENT    = 1 ;
const SDB_LIST_SESSIONS            = 2 ;
const SDB_LIST_SESSIONS_CURRENT    = 3 ;
const SDB_LIST_COLLECTIONS         = 4 ;
const SDB_LIST_COLLECTIONSPACES    = 5 ;
const SDB_LIST_STORAGEUNITS        = 6 ;
const SDB_LIST_GROUPS              = 7 ;
const SDB_LIST_STOREPROCEDURES     = 8 ;
const SDB_LIST_DOMAINS             = 9 ;
const SDB_LIST_TASKS               = 10 ;
const SDB_LIST_TRANSACTIONS        = 11 ;
const SDB_LIST_TRANSACTIONS_CURRENT = 12 ;
const SDB_LIST_SVCTASKS            = 14 ;
const SDB_LIST_SEQUENCES           = 15 ;
const SDB_LIST_USERS               = 16 ;
const SDB_LIST_BACKUPS             = 17 ;
const SDB_LIST_DATASOURCES         = 22 ;
// const SDB_LIST_INDEXES = 24, for internal use only
const SDB_LIST_RECYCLEBIN          = 27 ;
const SDB_LIST_GROUPMODES          = 28 ;

const SDB_INSERT_CONTONDUP         = 1 ;
const SDB_INSERT_RETURN_ID         = 0x10000000 ;
const SDB_INSERT_REPLACEONDUP      = 4 ;
const SDB_INSERT_UPDATEONDUP       = 0x00000008 ;
// const SDB_INSERT_HAS_ID_FIELD = 0x00000010, for internal use only
const SDB_INSERT_CONTONDUP_ID      = 0x00000020 ;
const SDB_INSERT_REPLACEONDUP_ID   = 0x00000040 ;

const SDB_TRACE_FLW                = 0 ;
const SDB_TRACE_FMT                = 1 ;

const SDB_COORD_GROUP_NAME         = "SYSCoord" ;
const SDB_CATALOG_GROUP_NAME       = "SYSCatalogGroup" ;
const SDB_SPARE_GROUP_NAME         = "SYSSpare" ;

const SDB_JSON_PARSE               = JSON.parse ;

const CM_PORT        = "CM_PORT" ;
const TMP_PATH       = "TMP_PATH" ;
const TRACE_HOSTNAME = "TRACE_HOSTNAME"  ;

// SdbQuery flags
const SDB_FLG_QUERY_FORCE_HINT        = 0x00000080 ;
const SDB_FLG_QUERY_PARALLED          = 0x00000100 ;
const SDB_FLG_QUERY_WITH_RETURNDATA   = 0x00000200 ;
const SDB_FLG_QUERY_PREPARE_MORE      = 0x00004000 ;
const SDB_FLG_QUERY_FOR_UPDATE        = 0x00010000 ;
const SDB_FLG_QUERY_FOR_SHARE         = 0x00040000 ;

// end Global Constants

// Global functions

// register json function
//JSON.parse JSON.stringify
function SDB_INIT(){

   function isArray( object ){
      return ( object &&
               typeof( object ) === 'object' &&
               typeof( object.length ) === 'number' &&
               typeof( object.splice ) === 'function' &&
               !( object.propertyIsEnumerable( 'length' ) ) ) ;
   }

   function filterInviChart(str) {
      var i = 0, len = str.length;
      var newStr = '';
      var chars, code;
      while (i < len) {
         chars = str.charAt(i);
         code = chars.charCodeAt();
         if (code < 0x20 || code == 0x7F) {
            chars = '?';
         }
         newStr += chars;
         ++i;
      }
      return newStr;
   }

   JSON.parse = function(str, func) {
      var json;
      try {
         json = SDB_JSON_PARSE(str, func);
      } catch (e) {
         try {
            var newStr = filterInviChart(str);
            json = SDB_JSON_PARSE(newStr, func);
         } catch (e) {
            json = eval('(' + str + ')');
         }
      }
      return json;
   } ;

   //var rx_escapable = /[\\\"\u0000-\u001f\u007f-\u009f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;

   function f(n) {
      return n < 10
          ? "0" + n
          : n;
   }

   function this_value() {
      return this.valueOf();
   }

   if (typeof Date.prototype.toJSON !== "function") {

      Date.prototype.toJSON = function () {

         return isFinite(this.valueOf())
             ? this.getUTCFullYear() + "-" +
                     f(this.getUTCMonth() + 1) + "-" +
                     f(this.getUTCDate()) + "T" +
                     f(this.getUTCHours()) + ":" +
                     f(this.getUTCMinutes()) + ":" +
                     f(this.getUTCSeconds()) + "Z"
             : null;
      };

      Boolean.prototype.toJSON = this_value;
      Number.prototype.toJSON = this_value;
      String.prototype.toJSON = this_value;
   }

   var gap;
   var indent;
   var meta;
   var rep;


   function quote(str) {
      var newString = "" ;
      var length = str.length ;
      for( var i = 0; i < length; ++i )
      {
         switch( str.charAt( i ) )
         {
         case "\"":
            newString += "\\\"" ;
            break ;
         case "\\":
            newString += "\\\\" ;
            break ;
         case "\b":
            newString += "\\b" ;
            break ;
         case "\f":
            newString += "\\f" ;
            break ;
         case "\n":
            newString += "\\n" ;
            break ;
         case "\r":
            newString += "\\r" ;
            break ;
         case "\t":
            newString += "\\t" ;
            break ;
         default:
            newString += str.charAt( i ) ;
         }
      }
      return "\"" + newString + "\"" ;
   }


   function str(key, holder) {

      var i;
      var k;
      var v;
      var length;
      var mind = gap;
      var partial;
      var value = holder[key];

      if (value && typeof value === "object" &&
              typeof value.toJSON === "function") {
         value = value.toJSON(key);
      }

      if (typeof rep === "function") {
         value = rep.call(holder, key, value);
      }

      switch (typeof value) {
         case "string":
            return quote(value);

         case "number":
            if (value === Number.POSITIVE_INFINITY) {
               return 'Infinity';
            }
            else if (value === Number.NEGATIVE_INFINITY) {
               return '-Infinity';
            }
            else if (value === Number.NaN) {
               return '0';
            }
            else {
               return String(value);
            }

         case "boolean":
         case "null":
            return String(value);

         case "object":
            if (!value) {
               return "null";
            }
            gap += indent;
            partial = [];

            if( isArray( value ) ) {
               length = value.length;
               for (i = 0; i < length; i += 1) {
                  partial[i] = str(i, value) || "null";
               }

               v = partial.length === 0
                   ? "[]"
                   : gap
                       ? "[\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "]"
                       : "[" + partial.join(",") + "]";
               gap = mind;
               return v;
            }

            if (rep && typeof rep === "object") {
               length = rep.length;
               for (i = 0; i < length; i += 1) {
                  if (typeof rep[i] === "string") {
                     k = rep[i];
                     v = str(k, value);
                     if (v) {
                        partial.push(quote(k) + (
                            gap
                                ? ": "
                                : ":"
                        ) + v);
                     }
                  }
               }
            } else {

               for (k in value) {
                  //if (Object.prototype.hasOwnProperty.call(value, k)) {
                     v = str(k, value);
                     if (v) {
                        partial.push(quote(k) + (
                            gap
                                ? ": "
                                : ":"
                        ) + v);
                     }
                  //}
               }
            }

            v = partial.length === 0
                ? "{}"
                : gap
                    ? "{\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "}"
                    : "{" + partial.join(",") + "}";
            gap = mind;
            return v;
      }
   }

   meta = {
      "\b": "\\b",
      "\t": "\\t",
      "\n": "\\n",
      "\f": "\\f",
      "\r": "\\r",
      "\"": "\\\"",
      "\\": "\\\\"
   };

   JSON.stringify = function (value, replacer, space) {

      var i;
      gap = "";
      indent = "";

      if (typeof space === "number") {
         for (i = 0; i < space; i += 1) {
            indent += " ";
         }

      } else if (typeof space === "string") {
         indent = space;
      }

      rep = replacer;
      if (replacer && typeof replacer !== "function" &&
              (typeof replacer !== "object" ||
              typeof replacer.length !== "number")) {
         throw new Error("JSON.stringify");
      }

      return str("", { "": value });
   };

}
SDB_INIT() ;

function println ( val ) {
   if ( arguments.length > 0 )
      print ( val ) ;
   print ( '\n' ) ;
}
// return a double number between 0 and 1
function rand () {
   return Math.random() ;
}

// return merged json object
function mergeJsonObject(obj1, obj2) {
   var result = {};
   for (var attr in obj1) {
      result[attr] = obj1[attr];
   }
   for (var attr in obj2) {
      result[attr] = obj2[attr];
   }

   return result;
}

function isEmptyObject(obj) {
   for (var name in obj) {
      return false;
   }

   return true;
}

// end Global functions

Object.defineProperty(Object.prototype,'_rawValueOf',{
   enumerable:false,
   value: Object.prototype.valueOf
});

Object.defineProperty(Object.prototype,'_rawToString',{
   enumerable:false,
   value: Object.prototype.toString
});

Object.defineProperty(Object.prototype,'_equality',{
   enumerable:false,
   value: function(rval) {
      if ( this.$numberLong != undefined ) {
         if ( rval.$numberLong != undefined ) {
            return this.valueOf() == rval.valueOf() ;
         }
         return rval == this.valueOf() ;
      }
      if ( rval.$numberLong != undefined ) {
         return this == rval.valueOf() ;
      }

      throw "condition not suitable for the function" ;
   }
});

Object.prototype.valueOf = function() {
   if (this.$numberLong != undefined) {
      if ( typeof(this.$numberLong ) == "string" )
      {
         return parseInt(this.$numberLong) ;
      }
      else if ( typeof(this.$numberLong ) == "number" )
      {
         return this.$numberLong ;
      }
      else
      {
         throw "invalid $numberLong" ;
      }
   }

   return this._rawValueOf() ;
}

Object.prototype.toString = function() {
   if (this.$numberLong != undefined) {
      try
      {
         return JSON.stringify ( this, undefined, 2 ) ;
      }
      catch ( e )
      {
      }
   }
   return this._rawToString() ;
}

// SdbCursor
SdbCursor.prototype.toArray = function() {
   if ( this._arr )
      return this._arr;

   var a = [];
   while ( true ) {
      var bs = this.next();
      if ( ! bs ) break ;
      var json = bs.toJson () ;
      try
      {
         var stf = JSON.stringify(JSON.parse(json), undefined, 2) ;
         a.push ( stf ) ;
      }
      catch ( e )
      {
         a.push ( json ) ;
      }
   }
   this._arr = a ;
   return this._arr ;
}

SdbCursor.prototype.arrayAccess = function( idx ) {
   return this.toArray()[idx] ;
}

SdbCursor.prototype.size = function() {
   //return this.toArray().length ;
   var cursor = this ;
   var size = 0 ;
   var record = undefined ;
   while( ( record = cursor.next() ) != undefined )
   {
      size++ ;
   }
   return size ;
}

SdbCursor.prototype.toString = function() {
   //return this.toArray().join('\n') ;
   var csr = this ;
   var record = undefined ;
   var returnRecordNum = 0 ;
   while ( ( record = csr.next() ) != undefined )
   {
      returnRecordNum++ ;
      try
      {
         println ( record ) ;
      }
      catch ( e )
      {
         var json = record.toJson () ;
         println ( json ) ;
      }
   }
   println("Return "+returnRecordNum+" row(s).") ;
   return "" ;
}
// end SdbCursor


// CLCount
CLCount.prototype.toString = function() {
   this._exec() ;
   return this._count ;
}
CLCount.prototype.valueOf = function() {
   this._exec() ;
   return this._count ;
}
CLCount.prototype.hint = function( hint ) {
   this._hint = hint ;
   return this ;
}
CLCount.prototype._exec = function() {
   this._count = this._collection._count ( this._condition,
                                           this._hint ) ;
}
// end CLCount

// SdbCollection

SdbCollection.prototype.count = function( condition ) {
   var count = new CLCount() ;
   count._condition = {} ;
   if( undefined != condition )
   {
      count._condition = condition ;
   }
   count._collection = this ;
   count._hint = {} ;
   return count ;
}

SdbCollection.prototype.find = function( query, select ) {

   if ( query instanceof SdbQueryOption )
   {
      return this.rawFind( query );
   }

   var queryObj = new SdbQuery();
   queryObj._query = {};
   queryObj._select = {} ;
   if( undefined != query )
   {
      queryObj._query = query ;
   }
   if( undefined != select )
   {
      queryObj._select = select ;
   }
   queryObj._sort = {} ;
   queryObj._hint = {} ;
   queryObj._options = {} ;
   queryObj._collection = this ;
   return queryObj ;
}

SdbCollection.prototype.findOne = function( query, select ) {
   if ( query instanceof SdbQueryOption )
   {
      return this.rawFind( query.limit(1) );
   }

   var queryObj = new SdbQuery() ;
   queryObj._query = {};
   queryObj._select = {} ;
   if( undefined != query )
   {
      queryObj._query = query ;
   }
   if( undefined != select )
   {
      queryObj._select = select ;
   }
   queryObj._sort = {} ;
   queryObj._hint = {} ;
   queryObj._options = {} ;
   queryObj._collection = this ;
   queryObj.limit( 1 ) ;
   return queryObj ;
}

SdbCollection.prototype.getIndex = function( name ) {
   if ( ! name )
   {
      setLastErrMsg( "SdbCollection.getIndex(): the 1st param should be "
                     + "valid string" ) ;
      throw SDB_INVALIDARG ;
   }

   var obj = this._getIndexes(name).next();
   if (undefined == obj)
   {
      setLastError( SDB_IXM_NOTEXIST ) ;
      setLastErrMsg( getErr( SDB_IXM_NOTEXIST ) ) ;
      throw SDB_IXM_NOTEXIST ;
   }

   return obj ;
}

SdbCollection.prototype.listIndexes = function() {
   return this._getIndexes() ;
}

SdbCollection.prototype.toString = function() {
   return this._cs.toString() + "." + this._name;
}

SdbCollection.prototype.insert = function ( data , arg )
{
   if ( (typeof data) != "object" )
   {
      setLastErrMsg( "SdbCollection.insert(): the 1st param should be "
                     + "obj or array of objs" ) ;
      throw SDB_INVALIDARG ;
   }

   var flag = 0 ;
   if ( arg == undefined )
   {
      flag = 0 ;
   }
   else if ( ( typeof arg ) == "number" ||
             ( ( typeof arg ) == "object" && !( arg instanceof Array ) ) )
   {
      flag = arg ;
   }
   else
   {
      setLastErrMsg( "SdbCollection.insert(): the 2nd param if existed "
                     + "should be a insert flag or insert options" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( data instanceof Array )
   {
      if ( 0 == data.length )
      {
         return ;
      }

      return this._bulkInsert( data, flag ) ;
   }
   else
   {
      return this._insert( data, flag ) ;
   }
}

/*
SdbCollection.prototype.rename = function ( newName ) {
   this._rename ( newName ) ;
   this._name = newName ;
}
*/
// end SdbCollection

// SdbQuery
SdbQuery.prototype._checkExecuted = function() {
   if ( this._cursor )
      throw "query already executed";
}

SdbQuery.prototype._exec = function() {
   if ( ! this._cursor ) {
      this._cursor = this._collection.rawFind( this._query,
                                               this._select,
                                               this._sort,
                                               this._hint,
                                               this._skip,
                                               this._limit,
                                               this._flags,
                                               this._options );
   }
   return this._cursor;
}

SdbQuery.prototype.sort = function( sort ) {
   this._checkExecuted();
   this._sort = sort;
   return this;
}

SdbQuery.prototype.hint = function( hint ) {
   this._checkExecuted();
   if (undefined == this._hint) {
      this._hint = hint;
   } else {
      this._hint = mergeJsonObject(hint, this._hint);
   }
   return this;
}

SdbQuery.prototype.skip = function( skip ) {
   this._checkExecuted();
   this._skip = skip;
   return this;
}

SdbQuery.prototype.limit = function( limit ) {
   this._checkExecuted();
   this._limit = limit;
   return this;
}

SdbQuery.prototype.flags = function( flags ) {
   this._checkExecuted();
   this._flags = flags;
   return this;
}

SdbQuery.prototype.next = function() {
   this._exec();
   return this._cursor.next();
}

SdbQuery.prototype.current = function() {
   this._exec();
   return this._cursor.current();
}

SdbQuery.prototype.close = function() {
   this._exec();
   return this._cursor.close();
}

SdbQuery.prototype.update = function( rule, returnNew, options ) {
   if ((typeof rule) != "object" || isEmptyObject(rule)) {
      setLastErrMsg( "SdbQuery.update(): the 1st param should be "
                     + "non-empty object" ) ;
      throw SDB_INVALIDARG ;
   }
   if (undefined != returnNew && (typeof returnNew) != "boolean") {
      setLastErrMsg( "SdbQuery.update(): the 2nd param should be boolean" ) ;
      throw SDB_INVALIDARG ;
   }
   if (undefined != options && (typeof options) != "object") {
      setLastErrMsg( "SdbQuery.update(): the 3rd param should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   this._checkExecuted();

   if (undefined == this._hint) {
      this._hint = {};
   } else if (undefined != this._hint.$Modify) {
      setLastErrMsg( "SdbQuery.update(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "update";
   modify.Update = rule;
   modify.ReturnNew = (returnNew != undefined) ? returnNew : false;
   this._hint.$Modify = modify;

   if (undefined != options) {
      this._options = options;
   }

   return this;
}

SdbQuery.prototype.remove = function() {
   this._checkExecuted();
   if (undefined == this._hint) {
      this._hint = {};
   } else if (undefined != this._hint.$Modify) {
      setLastErrMsg( "SdbQuery.remove(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "remove";
   modify.Remove = true;
   this._hint.$Modify = modify;

   return this;
}

/*
SdbQuery.prototype.updateCurrent = function ( rule ) {
   this._exec();
   return this._cursor.updateCurrent( rule ) ;
}

SdbQuery.prototype.deleteCurrent = function () {
   this._exec();
   return this._cursor.deleteCurrent();
}
*/
SdbQuery.prototype.toArray = function() {
   this._exec();
   return this._cursor.toArray();
}

SdbQuery.prototype.arrayAccess = function( idx ) {
   return this.toArray()[idx];
}

SdbQuery.prototype.count = function() {
   if (undefined != this._hint && undefined != this._hint.$Modify) {
      setLastErrMsg( "count() cannot be executed with update() or remove()" ) ;
      throw SDB_INVALIDARG ;
   }
   var countObj = this._collection.count( this._query ) ;
   if ( undefined != this._hint ) {
      countObj.hint( this._hint ) ;
   }
   return countObj ;
}

SdbQuery.prototype.explain = function( options ) {
   if( undefined == options )
   {
      options = {} ;
   }
   return this._collection.explain( this._query,
                                    this._select,
                                    this._sort,
                                    this._hint,
                                    this._skip,
                                    this._limit,
                                    this._flags,
                                    options ) ;
}

SdbQuery.prototype.getQueryMeta = function() {
   return this._collection.getQueryMeta( this._query,
                                         this._sort,
                                         this._hint,
                                         this._skip,
                                         this._limit ) ;
}

SdbQuery.prototype.size = function() {
//   return this.toArray().length;
   this._exec();
   return this._cursor.size() ;
}

SdbQuery.prototype.toString = function() {
   this._exec();
   var csr = this._cursor ;
   var record = undefined ;
   var returnRecordNum = 0 ;
   while ( ( record = csr.next() ) != undefined )
   {
      returnRecordNum++ ;
      try
      {
         println ( record ) ;
      }
      catch ( e )
      {
         var json = record.toJson () ;
         println ( json ) ;
      }
   }
   println("Return "+returnRecordNum+" row(s).") ;
   return "" ;
   //return this._cursor.toString();
}
// end SdbQuery

// SdbNode
SdbNode.prototype.toString = function() {
   return this._hostname + ":" +
          this._servicename ;
}

SdbNode.prototype.getHostName = function() {
   return this._hostname ;
}

SdbNode.prototype.getServiceName = function() {
   return this._servicename ;
}

SdbNode.prototype.getNodeDetail = function() {
   return this._nodeid + ":" + this._hostname + ":" +
          this._servicename + "(" +
          this._rg.toString() + ")" ;
}

SdbNode.prototype.getDetailObj = function() {
    var rgInfo = this._rg.getDetailObj().toObj();
    var groupInfo = rgInfo.Group;
    for (var i in groupInfo) {
        if (groupInfo[i].NodeID == this._nodeid) {
            var node = groupInfo[i];
            node.GroupName = rgInfo.GroupName;
            node.GroupID = rgInfo.GroupID;
            return new BSONObj(node);
        }
    }
    setLastErrMsg(getErr(SDB_CLS_NODE_NOT_EXIST));
    throw SDB_CLS_NODE_NOT_EXIST;
}
// end SdbNode

// SdbReplicaGroup
SdbReplicaGroup.prototype.toString = function() {
   return this._name;
}

// getDetail will be remove, suggest using getDetailObj
SdbReplicaGroup.prototype.getDetail = function() {
   return this._conn.list( SDB_LIST_GROUPS,
                           {GroupName: this._name } ) ;
}

SdbReplicaGroup.prototype.getDetailObj = function() {
   var cursor = this._conn.list( SDB_LIST_GROUPS,
                              {GroupName: this._name } ) ;
   if ( undefined == cursor )
   {
      setLastErrMsg( getErr( SDB_CLS_GRP_NOT_EXIST ) ) ;
      throw SDB_CLS_GRP_NOT_EXIST ;
   }
   var obj = cursor.next() ;
   cursor.close() ;
   return obj ;
}
// end SdbReplicaGroup

// SdbCS
SdbCS.prototype.toString = function() {
   return this._conn.toString() + "." + this._name;
}

SdbCS.prototype._resolveCL = function(clName) {
   if( !this.hasOwnProperty( clName) )
   {
      this.getCL( clName ) ;
   }
}

// end SdbCS


// SdbDomain
SdbDomain.prototype.toString = function() {
   return this._domainname ;
}
// end SdbDomain

// SdbDc
SdbDC.prototype.toString = function() {
   return this._name ;
}
// end SdbDc

// SdbSequence
SdbSequence.prototype.toString = function() {
   return this._conn.toString() + "." + this._name;
}
// end SdbSequence

// Sdb
Sdb.prototype.toString = function() {
   return this._host + ":" + this._port;
}

Sdb.prototype.listCollectionSpaces = function() {
   return this.list( SDB_LIST_COLLECTIONSPACES ) ;
}

Sdb.prototype.listCollections = function() {
   return this.list( SDB_LIST_COLLECTIONS ) ;
}

Sdb.prototype.listSequences = function() {
   return this.list( SDB_LIST_SEQUENCES ) ;
}

Sdb.prototype.listReplicaGroups = function() {
   return this.list( SDB_LIST_GROUPS ) ;
}

Sdb.prototype.getTask = function( id ) {
   if ( typeof( id ) == 'undefined' )
   {
      setLastErrMsg( "Task id must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( typeof( id ) != 'number' )
   {
      setLastErrMsg( "Task id must be number" ) ;
      throw SDB_INVALIDARG ;
   }

   var obj = this.listTasks( { TaskID: id } ).next() ;
   if (undefined == obj)
   {
      setLastError( SDB_CAT_TASK_NOTFOUND ) ;
      setLastErrMsg( getErr( SDB_CAT_TASK_NOTFOUND ) ) ;
      throw SDB_CAT_TASK_NOTFOUND ;
   }
   return obj ;
}

Sdb.prototype._resolveCS = function(csName) {
   if( !this.hasOwnProperty( csName ) )
   {
      return this.getCS( csName ) ;
   }
}

// getCatalogRG will be remove, suggest using getCataRG
Sdb.prototype.getCatalogRG = function() {
   return this.getCataRG() ;
}

Sdb.prototype.getCataRG = function() {
   return this.getRG( SDB_CATALOG_GROUP_NAME ) ;
}

// removeCatalogRG will be remove, suggest using removeCataRG
Sdb.prototype.removeCatalogRG = function() {
   return this.removeCataRG() ;
}

Sdb.prototype.removeCataRG = function() {
   return this.removeRG( SDB_CATALOG_GROUP_NAME ) ;
}

Sdb.prototype.createCoordRG = function() {
   return this.createRG( SDB_COORD_GROUP_NAME ) ;
}

Sdb.prototype.removeCoordRG = function() {
   return this.removeRG( SDB_COORD_GROUP_NAME ) ;
}

Sdb.prototype.getCoordRG = function() {
   return this.getRG( SDB_COORD_GROUP_NAME ) ;
}

Sdb.prototype.createSpareRG = function() {
   return this.createRG(SDB_SPARE_GROUP_NAME) ;
}

Sdb.prototype.getSpareRG = function() {
   return this.getRG(SDB_SPARE_GROUP_NAME) ;
}

Sdb.prototype.removeSpareRG  = function() {
   return this.removeRG( SDB_SPARE_GROUP_NAME ) ;
}

Sdb.prototype.stopRG = function() {
   for( var index in arguments )
   {
      var rgName = arguments[ index ] ;
      if ( "string" != typeof( rgName ) )
      {
         setLastErrMsg( "Sdb.stopRG(): wrong arguments" ) ;
         throw SDB_INVALIDARG ;
      }
      try
      {
         this.getRG( rgName ).stop() ;
      }
      catch( e )
      {
         setLastErrMsg( rgName + ": " + getLastErrMsg() ) ;
         throw e ;
      }
   }
}

Sdb.prototype._getTraceInfo = function()
{
   var path          = "" ;
   var cmPort        = "" ;
   var localIP       = "" ;
   var traceInfo     = {} ;
   var traceHostname = "" ;
   var localHostname = "" ;

   try
   {
      var retObj = this.snapshot( SDB_SNAP_CONFIGS, { "Global": false },
                                  { "NodeName": 1,
                                    "tmppath": 1 } ).next().toObj() ;
   }
   catch( e )
   {
      setLastErrMsg( getLastErrMsg() + " Failed to get trace info" ) ;
      throw e ;
   }

   traceHostname = retObj.NodeName.split( ":" )[0] ;
   localHostname = System.getHostName() ;
   cmPort        = Oma.getAOmaSvcName( traceHostname ) ;
   localIP       = System.getAHostMap( localHostname ) ;

   // The format of the NodeName:
   // 1. u1604-fngjiabin:50000
   // 2. 192.168.20.71:50000
   // 3. 127.0.0.1:50000
   // 4. localhost:50000
   if( traceHostname != localHostname && traceHostname != localIP &&
       traceHostname != "127.0.0.1" && traceHostname != "localhost" )
   {
      path = retObj.tmppath + "tmp.dump" ;
   }

   traceInfo[CM_PORT]        = cmPort ;
   traceInfo[TMP_PATH]       = path ;
   traceInfo[TRACE_HOSTNAME] = traceHostname ;

   return traceInfo ;
}

Sdb.prototype.traceOff = function()
{
   var path          = "" ;
   var cmPort        = "" ;
   var traceHostname = "" ;
   var argumentsSize = arguments.length ;
   var traceInfo ;

   if( 1 == argumentsSize )
   {
      if( "string" != typeof( arguments[0] ) )
      {
         setLastErrMsg( "FileName must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   if( 2 == argumentsSize )
   {
      if( "boolean" != typeof( arguments[1] ) )
      {
         setLastErrMsg( "The second parameter must be bool" ) ;
         throw SDB_INVALIDARG ;
      }

      if( arguments[1] )
      {
         traceInfo     = this._getTraceInfo() ;
         path          = traceInfo.TMP_PATH ;
         cmPort        = traceInfo.CM_PORT ;
         traceHostname = traceInfo.TRACE_HOSTNAME ;
      }
   }

   if( "" != path )
   {
      try
      {
         var remote = new Remote( traceHostname, cmPort ) ;
      }
      catch( e )
      {
         setLastErrMsg( getLastErrMsg() +
                        ". You can check if there is a cm process " +
                        "on the host[" + traceHostname + ":" + cmPort + "]. "
                        + "\n" + "Or check whether the network is normal" ) ;
         throw e ;
      }

      if( File.exist( arguments[0] ) )
      {
         if( File.getSize( arguments[0] ) < 2 )
         {
            setLastErrMsg( "The file[" + arguments[0] +
                           "] exists. But it isn't trace file" ) ;
            throw SDB_FE ;
         }

         var file = new File( arguments[0] ) ;
         var eyeCatcher = file.read( 2 ) ;
         if( "TB" != eyeCatcher )
         {
            setLastErrMsg( "The file[" + arguments[0] +
                           "] is exist. But it isn't trace file" ) ;
            throw SDB_PD_TRACE_FILE_INVALID ;
         }
      }

      this._traceOff( path ) ;

      try
      {
         var src = traceHostname + ":" + cmPort + "@" + path ;
         var des = arguments[0] ;
         File.scp( src, des, true, 0640 ) ;
      }
      catch( e )
      {
         setLastErrMsg( getLastErrMsg() +
                        " Failed to scp. The tmp trace file is in " +
                        traceHostname + ":" + path ) ;
         throw e ;
      }

      try
      {
         var remote = new Remote( traceHostname, cmPort ) ;
         var remoteFile = remote.getFile() ;
         remoteFile.remove( path ) ;
      }
      catch( e )
      {
         setLastErrMsg( getLastErrMsg() +
                        " Failed to remove tmp trace file" ) ;
         throw e ;
      }
   }
   else
   {
      this._traceOff( arguments[0] ) ;
   }
}

SecureSdb.prototype._resolveCS = function(csName) {
   if( !this.hasOwnProperty( csName ) )
   {
      return this.getCS( csName ) ;
   }
}
// end Sdb

function printCallStack()
{
   try
   {
      throw new Error( "print ErrStack" ) ;
   }
   catch ( e )
   {
      print( e.stack ) ;
   }
}

function assert( condition )
{
   if ( !condition )
   {
      printCallStack() ;
   }
}

// ObjectId
if ( !ObjectId.prototype )
   ObjectId.prototype = {}

ObjectId.prototype.toString = function() {
   return "ObjectId(\"" + this._str + "\")" ;
}
// end ObjectId

// BinData
if ( !BinData.prototype )
   BinData.prototype = {}

BinData.prototype.toString = function() {
   return "BinData(\"" + this._data + "\", \"" + this._type + "\")"  ;
}


// end BinData

// Timestamp
if ( !Timestamp.prototype )
   Timestamp.prototype = {}

Timestamp.prototype.toString = function() {
   return "Timestamp(\"" + this._t + "\")" ;
}
// end Timestamp

// Regex
if ( !Regex.prototype )
   Regex.prototype = {}

Regex.prototype.toString = function () {
   return "Regex(\"" + this._regex + "\", \"" + this._option + "\")" ;
}
// end Regex

// MinKey
if ( !MinKey.prototype )
   MinKey.prototype = {}

MinKey.prototype.toString = function() {
   return "MinKey()" ;
}
// end MinKey

// MaxKey
if ( !MaxKey.prototype )
   MaxKey.prototype = {}

MaxKey.prototype.toString = function() {
   return "MaxKey()" ;
}
// end MaxKey

// NumberLong
if ( !NumberLong.prototype )
   NumberLong.prototype = {}

NumberLong.prototype.toString = function() {
   if ( typeof(this._v ) == "string" )
   {
      return "NumberLong(\"" + this._v + "\")" ;
   }
   return "NumberLong(" + this._v + ")" ;
}

NumberLong.prototype.valueOf = function() {
   if ( typeof(this._v ) == "string" )
   {
      return parseInt(this._v) ;
   }
   return this._v ;
}

// end NumberLong

// NumberDecimal
if ( !NumberDecimal.prototype )
   NumberDecimal.prototype = {}

NumberDecimal.prototype.valueOf = function() {
   var decimalNumber = this._decimal ;

   if ( typeof( this._decimal ) == "string" )
   {
      if( isNaN( this._decimal) )
      {
         throw "Decimal data must be number or numeric string" ;
      }
      decimalNumber = parseInt(this._decimal) ;
   }

   return decimalNumber ;
}

NumberDecimal.prototype.toString = function() {
   var decimalNumber = this._decimal ;
   var precision = "";

   if( typeof( this._precision ) != "undefined" )
   {
      if( 0 == this._precision.length )
      {
         precision = "" ;
      }
      else
      {
         precision = ", [" + this._precision +"]" ;
      }
   }

   return "NumberDecimal( " + decimalNumber + precision + " )" ;
}

// end NumberDecimal

// SdbDate
if ( !SdbDate.prototype )
   SdbDate.prototype = {}

SdbDate.prototype.toString = function() {
   return "SdbDate(\"" + this._d + "\")" ;
}
// end SdbDate

// SdbOptionBase

SdbOptionBase.prototype.cond = function(cond) {
   this._cond = BSONObj(cond) ;
   return this ;
}

SdbOptionBase.prototype.sel = function(sel) {
   this._sel = BSONObj(sel) ;
   return this ;
}

SdbOptionBase.prototype.sort = function(sort) {
   this._sort = BSONObj(sort) ;
   return this ;
}

SdbOptionBase.prototype.hint = function(hint) {
   this._hint = BSONObj(hint) ;
   return this ;
}

SdbOptionBase.prototype.skip = function(skip) {
   if ( typeof( skip ) == "number" ) {
      this._skip = skip ;
   } else {
      setLastErrMsg( "SdbOptionBase.skip() param must be Number" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

SdbOptionBase.prototype.limit = function(limit) {
   if ( typeof( limit ) == "number" ) {
      this._limit = limit ;
   } else {
      setLastErrMsg( "SdbOptionBase.limit() param must be Number" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

SdbOptionBase.prototype.flags = function(flags) {
   if ( typeof ( flags ) == "number" ) {
      this._flags = flags ;
   } else {
      setLastErrMsg( "SdbOptionBase.flags() param must be Number" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

SdbOptionBase.prototype.toString = function() {
   return this.__className__ + "(" + "\"cond\": " + this._cond.toJson() +
          ", \"sel\": " + this._sel.toJson() +
          ", \"sort\": " + this._sort.toJson() +
          ", \"hint\": " + this._hint.toJson() +
          ", \"skip\": " + this._skip +
          ", \"limit\": " + this._limit +
          ", \"flags\": " + this._flags + ")" ;
}

// end SdbOptionBase

// SdbSnapshotOption

SdbSnapshotOption.prototype.options = function(options) {
   if (undefined != options && (typeof options) != "object") {
      setLastErrMsg( "SdbSnapshotOption.options(): param should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   this._hint = BSONObj({$Options:BSONObj(options)}) ;
   return this ;
}

// end SdbSnapshotOption

// SdbQueryOption

SdbQueryOption.prototype.update = function( rule, returnNew, options ) {
   if ((typeof rule) != "object" || isEmptyObject(rule)) {
      setLastErrMsg( "SdbQueryOption.update(): the 1st param should be "
                     + "non-empty object" ) ;
      throw SDB_INVALIDARG ;
   }
   if (undefined != returnNew && (typeof returnNew) != "boolean") {
      setLastErrMsg( "SdbQueryOption.update(): the 2nd param "
                     + "should be boolean" ) ;
      throw SDB_INVALIDARG;
   }
   if (undefined != options && (typeof options) != "object") {
      setLastErrMsg( "SdbQueryOption.update(): the 3rd param "
                     + "should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   var hintObj = eval('(' + this._hint.toString() + ')');

   if (undefined == this._hint) {
      this._hint = {};
   } else if ( undefined != hintObj.$Modify ) {
      setLastErrMsg( "SdbQueryOption.update(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "update";
   modify.Update = rule;
   modify.ReturnNew = (returnNew != undefined) ? returnNew : false ;
   hintObj["$Modify"] = modify ;
   this._hint = BSONObj( hintObj );

   if (undefined != options) {
      this._options = BSONObj( options ) ;
   }

   return this;
}

SdbQueryOption.prototype.remove = function() {

   var hintObj = eval('(' + this._hint.toString() + ')');

   if (undefined == this._hint) {
      this._hint = {};
   } else if ( undefined != hintObj.$Modify ) {
      setLastErrMsg( "SdbQueryOption.remove(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "remove";
   modify.Remove = true;
   hintObj["$Modify"] = modify ;
   this._hint = BSONObj( hintObj );

   return this;
}

// end SdbQueryOption

// SdbTraceOption

SdbTraceOption.prototype.components = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      // the format that user specifies component parameter is like
      // .components( [ "dms", "rtn" ] )
      if( arguments[0] instanceof Array )
      {
         // if the type of the first parameter is array,
         // the method only needs one parameter
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid components' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         // the format that user specifies component parameter is like
         // .components( [] )
         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "Components can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            // the format that user specifies component parameter is like
            // .components( [ "", "" ] )
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "Component can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            // the format that user specifies component parameter is like
            // .components( [ 123, 456 ] )
            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "Component must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._components ) == "undefined" )
         {
            this._components = arguments[0] ;
         }
         // After we call the component method,
         // we can call the component method again
         // eg:
         // > var option = new SdbTraceOption().component( [ "rtn", "dms" ] )
         // now the component is [ "rtn", "dms" ]
         // > option.component( [ "oss", "mth" ] )
         // now the component is [ "rtn", "dms", "oss", "mth" ]
         else
         {
            this._components = this._components.concat( arguments[0] ) ;
         }
      }

      // the format that user specifies component parameter is like
      // .components( "dms", "rtn" )
      else
      {
         if( typeof( this._components ) == "undefined" )
         {
            this._components = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            // the format that user specifies component parameter is like
            // .components( "", "" )
            if ( "" == arguments[i] )
            {
               setLastErrMsg( "Component can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            // the format that user specifies component parameter is like
            // .components( 123, 456 )
            if ( typeof( arguments[i] ) != "string" )
            {
               setLastErrMsg( "Component must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }

            this._components.push( arguments[i] );
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.breakPoints = function( breakPoints )
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid breakPoints' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "Breakpoints can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "Breakpoint can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "Breakpoint must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._breakPoints ) == "undefined" )
         {
            this._breakPoints = arguments[0] ;
         }
         else
         {
            this._breakPoints = this._breakPoints.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._breakPoints ) == "undefined" )
         {
            this._breakPoints = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if ( "" == arguments[i] )
            {
               setLastErrMsg( "Breakpoint can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[i] ) != "string" )
            {
               setLastErrMsg( "Breakpoint must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }

            this._breakPoints.push( arguments[i] );
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.tids = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid tids' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "Tids can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( typeof( arguments[0][i] ) != "number" )
            {
               setLastErrMsg( "Tid must be int or int array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._tids ) == "undefined" )
         {
            this._tids = arguments[0] ;
         }
         else
         {
            this._tids = this._tids.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._tids ) == "undefined" )
         {
            this._tids = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if ( typeof( arguments[i] ) != "number" )
            {
               setLastErrMsg( "Tid must be int or int array" ) ;
               throw SDB_INVALIDARG ;
            }

            this._tids.push( arguments[i] );
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.functionNames = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid functionNames' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "FunctionNames can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "FunctionName can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "FunctionName must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._functionNames ) == "undefined" )
         {
            this._functionNames = arguments[0] ;
         }
         else
         {
            this._functionNames = this._functionNames.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._functionNames ) == "undefined" )
         {
            this._functionNames = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if( arguments[i] instanceof Array )
            {
               setLastErrMsg( "Invalid functionNames' parameters" ) ;
               throw SDB_INVALIDARG ;
            }
            else
            {
               if ( "" == arguments[i] )
               {
                  setLastErrMsg( "FunctionName can't be empty" ) ;
                  throw SDB_INVALIDARG ;
               }

               if ( typeof( arguments[i] ) != "string" )
               {
                  setLastErrMsg( "FunctionName must be string or string array" ) ;
                  throw SDB_INVALIDARG ;
               }

               this._functionNames.push( arguments[i] );
            }
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.threadTypes = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid threadTypes' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "ThreadTypes can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "ThreadType can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "ThreadType must be string or string array" ) ;
               throw SDB_INVALIDARG
            }
         }

         if( typeof( this._threadTypes ) == "undefined" )
         {
            this._threadTypes = arguments[0] ;
         }
         else
         {
            this._threadTypes = this._threadTypes.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._threadTypes ) == "undefined" )
         {
            this._threadTypes = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if( arguments[i] instanceof Array )
            {
               setLastErrMsg( "Invalid threadTypes' parameters" ) ;
               throw SDB_INVALIDARG ;
            }
            else
            {
               if ( "" == arguments[i] )
               {
                  setLastErrMsg( "ThreadType can't be empty" ) ;
                  throw SDB_INVALIDARG ;
               }

               if ( typeof( arguments[i] ) != "string" )
               {
                  setLastErrMsg( "ThreadType must be string or string array" ) ;
                  throw SDB_INVALIDARG ;
               }

               this._threadTypes.push( arguments[i] );
            }
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.toString = function()
{
   var componentsStr ;
   var breakPointsStr ;
   var tidsStr ;
   var funcNamesStr ;
   var threadTypesStr ;

   // User doesn't specify component parameter
   if( typeof( this._components ) == "undefined" )
   {
      componentsStr = "[]" ;
   }
   else
   {
      // the value that shell or engine returns
      if( this._components[0] != "[" )
      {
         componentsStr = "[ "  + "\"" + this._components.join("\", \"") +
                         "\"" + " ]" ;
      }
      // the value that fmp returns
      else
      {
         componentsStr = this._components ;
      }
   }

   if( typeof( this._breakPoints ) == "undefined" )
   {
      breakPointsStr = "[]" ;
   }
   else
   {
      if( this._breakPoints[0] != "[" )
      {
         breakPointsStr = "[ "  + "\"" + this._breakPoints.join("\", \"") +
                          "\"" + " ]" ;
      }
      else
      {
         breakPointsStr = this._breakPoints ;
      }
   }

   if( typeof( this._tids ) == "undefined" )
   {
      tidsStr = "[]" ;
   }
   else
   {
      if( this._tids[0] != "[" )
      {
         tidsStr = "[ "  + this._tids.join(", ") + " ]" ;
      }
      else
      {
         tidsStr = this._tids ;
      }
   }

   if( typeof( this._functionNames ) == "undefined" )
   {
      funcNamesStr = "[]" ;
   }
   else
   {
      if( this._functionNames[0] != "[" )
      {
         funcNamesStr = "[ "  + "\"" + this._functionNames.join("\", \"") +
                        "\"" + " ]" ;
      }
      else
      {
         funcNamesStr = this._functionNames ;
      }
   }

   if( typeof( this._threadTypes ) == "undefined" )
   {
      threadTypesStr = "[]" ;
   }
   else
   {
      if( this._threadTypes[0] != "[" )
      {
         threadTypesStr = "[ "  + "\"" + this._threadTypes.join("\", \"") +
                          "\"" + " ]" ;
      }
      else
      {
         threadTypesStr = this._threadTypes ;
      }
   }

   return this.__className__ +
          "( " +
          "\"components\": " + componentsStr +
          ", \"breakPoints\": " + breakPointsStr +
          ", \"tids\": " + tidsStr +
          ", \"functionNames\": " + funcNamesStr +
          ", \"threadTypes\": " + threadTypesStr +
          " )" ;
}

// end SdbTraceOption

// User
User.prototype.promptPassword = function()
{
   if ( "function" != typeof ( this._promptPassword ) )
   {
      throw "Fmp can't use promptPassword function" ;
   }
   this._promptPassword() ;
   return this ;
}

User.prototype.getUsername = function() {
   return this._user ;
}

User.prototype.toString = function() {
   return this._user ;
}

// end User

// CipherUser
CipherUser.prototype.token = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if ( "string" == typeof( arguments[0] ) )
      {
         this._setToken( arguments[0] ) ;
      }
      else
      {
         setLastErrMsg( "Token must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else
   {
      setLastErrMsg( "You must input token" ) ;
      throw SDB_INVALIDARG ;
   }

   return this ;
}

CipherUser.prototype.clusterName = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if ( "string" == typeof( arguments[0] ) )
      {
         this._clusterName = arguments[0] ;
      }
      else
      {
         setLastErrMsg( "Cluster name must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else
   {
      setLastErrMsg( "You must input cluster name" ) ;
      throw SDB_INVALIDARG ;
   }

   return this ;
}

CipherUser.prototype.cipherFile = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if ( "string" == typeof( arguments[0] ) )
      {
         this._cipherFile = arguments[0] ;
      }
      else
      {
         setLastErrMsg( "Cipher file must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else
   {
      setLastErrMsg( "You must input cipher file" ) ;
      throw SDB_INVALIDARG ;
   }

   return this ;
}

CipherUser.prototype.getUsername = function()
{
   return this._user ;
}

CipherUser.prototype.getClusterName = function()
{
   return this._clusterName ;
}

CipherUser.prototype.getCipherFile = function()
{
   return this._cipherFile ;
}

CipherUser.prototype.toString = function()
{
   var output = this._user ;
   if ( "undefined" != typeof ( this._clusterName ) && "" != this._clusterName )
   {
      output = ( output + "@" + this._clusterName ) ;
   }
   if ( "undefined" != typeof ( this._cipherFile ) && "" != this._cipherFile )
   {
      output = ( output + ":" + this._cipherFile ) ;
   }
   return output ;
}

// end CipherUser

SdbDataSource.prototype.help = function()
{
    println() ;
    println( '   --Instance methods for class "SdbDataSource"' ) ;
    println( '   alter(<options>)         - Alter data source options' ) ;
}
