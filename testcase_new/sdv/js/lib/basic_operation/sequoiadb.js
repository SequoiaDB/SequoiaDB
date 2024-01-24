import( "./cLCountObj.js" );
import( "./collection.js" );
import( "./collectionSpace.js" );
import( "./command.js" );
import( "./cursor.js" );
import( "./fileObj.js" );
import( "./node.js" );
import( "./numberDecimalObj.js" );
import( "./objectIdObj.js" );
import( "./query.js" );
import( "./replicaGroup.js" );

if( COORDHOSTNAME == undefined ) { var COORDHOSTNAME = "localhost"; }
if( COORDSVCNAME == undefined ) { var COORDSVCNAME = 11810; }

var db = new Sequoiadb( COORDHOSTNAME, COORDSVCNAME );

/* *****************************************************************************
@discription: return new Sdb -> new Sequoiadb
@author: yinzhen
@parameter
   hostname: string
   svcname: string | int
   username: string
   password: string
@e.g.
   var db = new Sequoiadb("localhost", 11810, "user", "passwd");
   var db = new Sequoiadb("localhost", 11810);
   var db = new Sequoiadb("localhost:11810");
***************************************************************************** */
function Sequoiadb ( hostname, svcname, username, password )
{
   try
   {
      if( username === undefined && password === undefined )
      {
         if( svcname === undefined )
         {
            var db = new Sdb( hostname );
         }
         else
         {
            var db = new Sdb( hostname, svcname );
         }
      }
      else
      {
         var db = new Sdb( hostname, svcname, username, password );
      }
   }
   catch( e )
   {
      throw new Error( e );
   }

   this.createCS =
      function( name, options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            var cs = db.createCS( name, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new CollectionSpace( cs );
      }

   this.dropCS =
      function( name )
      {
         try
         {
            db.dropCS( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.getCS =
      function( name )
      {
         try
         {
            var cs = db.getCS( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new CollectionSpace( cs );
      }

   this.transBegin =
      function()
      {
         try
         {
            db.transBegin();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.transCommit =
      function()
      {
         try
         {
            db.transCommit();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.transRollback =
      function()
      {
         try
         {
            db.transRollback();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.snapshot =
      function( snapType, cond, sel, sort )
      {
         if( cond === undefined ) { cond = {}; }
         if( sel === undefined ) { sel = {}; }
         if( sort === undefined ) { sort = {}; }
         try
         {
            var cursor = db.snapshot( snapType, cond, sel, sort );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.listReplicaGroups =
      function()
      {
         try
         {
            var cursor = db.listReplicaGroups();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.renameCS =
      function( oldname, newname )
      {
         try
         {
            db.renameCS( oldname, newname );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.close =
      function()
      {
         try
         {
            db.close();
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.listTasks =
      function( cond, sel, sort, hint )
      {
         if( cond === undefined ) { cond = {}; }
         if( sel === undefined ) { sel = {}; }
         if( sort === undefined ) { sort = {}; }
         if( hint === undefined ) { hint = {}; }
         try
         {
            var cursor = db.listTasks( cond, sel, sort, hint );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.removeProcedure =
      function( functionName )
      {
         try
         {
            db.removeProcedure( functionName );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.createProcedure =
      function( code )
      {
         try
         {
            db.createProcedure( code );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.eval =
      function( code )
      {
         try
         {
            var obj = db.eval( code );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return obj;
      }

   this.listProcedures =
      function( cond )
      {
         if( cond === undefined ) { cond = {}; }
         try
         {
            var cursor = db.listProcedures( cond );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.getDomain =
      function( name )
      {
         try
         {
            var domain = db.getDomain( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return domain;
      }

   this.createDomain =
      function( name, groups, options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            var domain = db.createDomain( name, groups, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return domain;
      }

   this.dropDomain =
      function( name )
      {
         try
         {
            db.dropDomain( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.getRG =
      function( name )
      {
         try
         {
            var rg = db.getRG( name );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new ReplicaGroup( rg );
      }

   this.listDomains =
      function( cond, sel, sort )
      {
         if( cond === undefined ) { cond = {}; }
         if( sel === undefined ) { sel = {}; }
         if( sort === undefined ) { sort = {}; }
         try
         {
            var cursor = db.listDomains( cond, sel, sort );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.toString =
      function()
      {
         try
         {
            var str = db.toString();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return str;
      }

   this.analyze =
      function( options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            db.analyze( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.getCatalogRG =
      function()
      {
         try
         {
            var rg = db.getCatalogRG();
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new ReplicaGroup( rg );
      }

   this.exec =
      function( selectSql )
      {
         try
         {
            var cursor = db.exec( selectSql );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }

   this.setSessionAttr =
      function( options )
      {
         try
         {
            db.setSessionAttr( options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.updateConf =
      function( config, options )
      {
         if( options === undefined ) { options = {}; }
         try
         {
            db.updateConf( config, options );
         }
         catch( e )
         {
            throw new Error( e );
         }
      }

   this.list =
      function( listType, cond, sel, sort )
      {
         if( cond === undefined ) { cond = {}; }
         if( sel === undefined ) { sel = {}; }
         if( sort === undefined ) { sort = {}; }
         try
         {
            var cursor = db.list( listType, cond, sel, sort );
         }
         catch( e )
         {
            throw new Error( e );
         }
         return new Cursor( cursor );
      }
}
