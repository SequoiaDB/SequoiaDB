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

const SDB_INSERT_CONTONDUP         = 1 ;
const SDB_INSERT_RETURN_ID         = 2 ; // only available when inserting only one document

const SDB_TRACE_FLW                = 0 ;
const SDB_TRACE_FMT                = 1 ;

const SDB_COORD_GROUP_NAME         = "SYSCoord" ;
const SDB_CATALOG_GROUP_NAME       = "SYSCatalogGroup" ;
const SDB_SPARE_GROUP_NAME         = "SYSSpare" ;

var SDB_PRINT_JSON_FORMAT          = true ;

const SDB_JSON_PARSE               = JSON.parse ;
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

function jsonFormat(pretty) {
   if (pretty == undefined){
      pretty = true;
   }
   SDB_PRINT_JSON_FORMAT = pretty;
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

// Bson
Bson.prototype.toObj = function() {
   return JSON.parse( this.toJson() ) ;
}

Bson.prototype.toString = function() {
   if ( typeof(SDB_PRINT_JSON_FORMAT) == "undefined" ||
        SDB_PRINT_JSON_FORMAT )
   {
      try
      {
         var obj = this.toObj();
         var str = JSON.stringify ( obj, undefined, 2 ) ;
         return str ;
      }
      catch ( e )
      {
         return this.toJson() ;
      }
   }
   else
   {
      return this.toJson() ;
   }
}
// end Bson

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
   return new CLCount( this, condition ) ;
}

SdbCollection.prototype.find = function( query, select ) {
   return new SdbQuery( this , query, select );
}

SdbCollection.prototype.findOne = function( query, select ) {
   return new SdbQuery( this , query, select ).limit( 1 ) ;
}

SdbCollection.prototype.getIndex = function( name ) {
   if ( ! name )
      throw "SdbCollection.getIndex(): 1st parameter should be valid string" ;
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

SdbCollection.prototype.insert = function ( data , flags )
{
   if ( (typeof data) != "object" )
      throw "SdbCollection.insert(): the 1st param should be obj or array of objs";
   var newFlags = 0 ;
   if ( flags != undefined )
   {
      if ( (typeof flags) != "number" ||
            ( flags != 0 &&
              flags != SDB_INSERT_RETURN_ID &&
              flags != SDB_INSERT_CONTONDUP ) )
         throw "SdbCollection.insert(): the 2nd param if existed should be 0 or SDB_INSERT_RETURN_ID or SDB_INSERT_CONTONDUP only";
      newFlags = flags ;
   }

   if ( data instanceof Array )
   {
      if ( 0 == data.length ) return ;
      if ( newFlags != 0 && newFlags != SDB_INSERT_CONTONDUP )
         throw "SdbCollection.insert(): when insert more than 1 records, the 2nd param if existed should be 0 or SDB_INSERT_CONTONDUP only";
      return this._bulkInsert ( data , newFlags ) ;
   }
   else
   {
      if ( newFlags != 0 && newFlags != SDB_INSERT_RETURN_ID )
         throw "SdbCollection.insert(): when insert 1 record, the 2nd param if existed should be 0 or SDB_INSERT_RETURN_ID only";
      return this._insert ( data , SDB_INSERT_RETURN_ID == flags ) ;
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
      throw "SdbQuery.update(): the 1st param should be non-empty object";
   }
   if (undefined != returnNew && (typeof returnNew) != "boolean") {
      throw "SdbQuery.update(): the 2nd param should be boolean";
   }
   if (undefined != options && (typeof options) != "object") {
      throw "SdbQuery.update(): the 3rd param should be object";
   }

   this._checkExecuted();

   if (undefined == this._hint) {
      this._hint = {};
   } else if (undefined != this._hint.$Modify) {
      throw "SdbQuery.update(): duplicate modification";
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
      throw "SdbQuery.remove(): duplicate modification";
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
      throw "count() cannot be executed with update() or remove()";
   }
   var countObj = this._collection.count( this._query ) ;
   if ( undefined != this._hint ) {
      countObj.hint( this._hint ) ;
   }
   return countObj ;
}

SdbQuery.prototype.explain = function( options ) {
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
// end SdbNode

// SdbReplicaGroup
SdbReplicaGroup.prototype.toString = function() {
   return this._name;
}

SdbReplicaGroup.prototype.getDetail = function() {
   return this._conn.list( SDB_LIST_GROUPS,
                           {GroupName: this._name } ) ;
}
// end SdbReplicaGroup

// SdbCS
SdbCS.prototype.toString = function() {
   return this._conn.toString() + "." + this._name;
}

SdbCS.prototype._resolveCL = function(clName) {
   this.getCL(clName) ;
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

Sdb.prototype.listReplicaGroups = function() {
   return this.list( SDB_LIST_GROUPS ) ;
}

Sdb.prototype._resolveCS = function(csName) {
   this.getCS( csName ) ;
}

Sdb.prototype.getCatalogRG = function() {
   return this.getRG( SDB_CATALOG_GROUP_NAME ) ;
}

Sdb.prototype.removeCatalogRG = function() {
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

// SdbDate
if ( !SdbDate.prototype )
   SdbDate.prototype = {}

SdbDate.prototype.toString = function() {
   return "SdbDate(\"" + this._d + "\")" ;
}
// end SdbDate
