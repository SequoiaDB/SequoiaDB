/*******************************************************************************
*@Description : Aggregate testcase common functions and varialb
*@Modify list :
*               2014-10-10  xiaojun Hu  Change
*               2016-03-18  wenjing wang modify
*******************************************************************************/

import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" );

function compareObj ( lobj, robj, ignoreId )
{
   if( typeof ( lobj ) === "object" &&
      typeof ( robj ) === "object" )
   {
      if( lobj == null && robj == null ) return true;
      if( lobj == null || robj == null || lobj.constructor !== robj.constructor ) return false;
      var _idNum = 0;
      for( key in lobj )
      {
         if( ignoreId && key === "_id" ) { _idNum = 1; continue };
         if( undefined === robj[key] ) return false;
         if( !compareObj( lobj[key], robj[key] ) ) return false;
      }

      var lkeys = Object.getOwnPropertyNames( lobj );
      var rkeys = Object.getOwnPropertyNames( robj );
      if( lkeys.length !== rkeys.length + _idNum ) return false;
      return true;
   }
   else if( lobj === robj )
   {
      return true;
   }
   else
   {
      return false;
   }
}
function collection ( db, csName, clName )
{
   if( db === undefined ||
      typeof ( csName ) !== "string" ||
      typeof ( clName ) !== "string" )
   {
      throw new Error( "collection" + "construct failed: error parameters" );
   }

   this.db = db;
   this.csName = csName;
   this.clName = clName;
}

collection.prototype.create =
   function( options )
   {
      // Drop collection in the beginning
      this.drop();
      // Create Collection and auto specify CollectionSpaces
      this.cl = commCreateCL( this.db, this.csName, this.clName, options );
   }

collection.prototype.drop =
   function()
   {
      // Clear environment in the end
      commDropCL( this.db, this.csName, this.clName, true, true );
   }

collection.prototype.execAggregate =
   function()
   {
      if( arguments.length === 0 )
      {
         throw new Error( "execAggregate" + "parameters error" );
      }

      var parameters = "";
      for( var i = 1; i < arguments.length; ++i )
      {
         if( arguments[i].constructor !== Object )
         {
            throw new Error( "execAggregate" + "parameters error" );
         }
         parameters += arguments[0];
         if( i !== arguments.length - 1 )
         {
            parameters += ", ";
         }
      }

      try
      {
         var cursor = this.cl.aggregate.apply( this.cl, arguments );
      }
      catch( e )
      {
         throw new Error( "collection.execAggregate" + "cl.aggregate( " + parameters + " )" + e );
      }

      return cursor;
   }

collection.prototype.bulkInsert =
   function( docs )
   {
      if( docs !== undefined && docs.constructor !== Array )
      {
         throw new Error( "bulkInsert parameters error" );
      }

      if( this.cl === undefined )
      {
         throw new Error( "bulkInsert must need call create" );
      }

      if( undefined === docs )
      {
         var docs = [{ no: 1000, score: 80, interest: ["basketball", "football"], major: "计算机科学与技术", dep: "计算机学院", info: { name: "Tom", age: 25, sex: "男" } },
         { no: 1001, score: 82, major: "计算机科学与技术", dep: "计算机学院", info: { name: "Json", age: 20, sex: "男" } },
         { no: 1002, score: 85, interest: ["movie", "photo"], major: "计算机软件与理论", dep: "计算机学院", info: { name: "Holiday", age: 22, sex: "女" } },
         { no: 1003, score: 90, major: "计算机软件与理论", dep: "计算机学院", info: { name: "Sam", age: 30, sex: "男" } },
         { no: 1004, score: 69, interest: ["basketball", "football", "movie"], major: "计算机工程", dep: "计算机学院", info: { name: "Coll", age: 26, sex: "男" } },
         { no: 1005, score: 70, major: "计算机工程", dep: "计算机学院", info: { name: "Jim", age: 24, sex: "女" } },
         { no: 1006, score: 84, interest: ["basketball", "football", "movie", "photo"], major: "物理学", dep: "物电学院", info: { name: "Lily", age: 28, sex: "女" } },
         { no: 1007, score: 73, interest: ["basketball", "football", "photo"], major: "物理学", dep: "物电学院", info: { name: "Kiki", age: 18, sex: "女" } },
         { no: 1008, score: 72, interest: ["basketball", "football", "movie"], major: "物理学", dep: "物电学院", info: { name: "Appie", age: 20, sex: "女" } },
         { no: 1009, score: 80, major: "物理学", dep: "物电学院", info: { name: "Lucy", age: 36, sex: "女" } },
         { no: 1010, score: 93, major: "光学", dep: "物电学院", info: { name: "Coco", age: 27, sex: "女" } },
         { no: 1011, score: 75, major: "光学", dep: "物电学院", info: { name: "Jack", age: 30, sex: "男" } },
         { no: 1012, score: 78, interest: ["basketball", "movie"], major: "光学", dep: "物电学院", info: { name: "Mike", age: 28, sex: "男" } },
         { no: 1013, score: 86, interest: ["basketball", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Jaden", age: 20, sex: "男" } },
         { no: 1014, score: 74, interest: ["football", "movie", "photo"], major: "电学", dep: "物电学院", info: { name: "Iccra", age: 19, sex: "男" } },
         { no: 1015, score: 81, major: "电学", dep: "物电学院", info: { name: "Jay", age: 15, sex: "男" } },
         { no: 1016, score: 92, major: "电学", dep: "物电学院", info: { name: "Kate", age: 20, sex: "男" } }
         ];
      }
      this.cl.insert( docs );

      return docs.length;
   }

function checkResult ( cursor, expectResult, parameter )
{
   var ret = [];
   if( cursor.constructor !== SdbCursor ||
      expectResult.constructor !== Array )
   {
      throw new Error( "checkResult parameter error" );
   }

   var i = 0;
   while( cursor.next() )
   {
      retObj = cursor.current().toObj();
      if( i >= expectResult.length )
      {
         ret.push( false );
         ret.push( {} )
         ret.push( retObj );
         return ret;
      }

      if( !compareObj( retObj, expectResult[i], true ) )
      {
         ret.push( false );
         ret.push( expectResult[i] )
         ret.push( retObj );
         return ret;
      }
      i++;
   }

   if( i === 0 )
   {
      ret.push( false );
   }
   else
   {
      ret.push( true );
   }
   ret.push( {} );
   if( !ret[0] )
   {
      throw new Error( "main cl.aggregate( " + parameter + " )" + JSON.stringify( ret[1] ) + JSON.stringify( ret[2] ) );
   }
}

function getRetNumber ( cursor )
{
   if( cursor.constructor !== SdbCursor )
   {
      throw new Error( "checkResult parameter error" );
   }

   var number = 0;
   while( cursor.next() )
   {
      number++;
   }

   return number;
}
