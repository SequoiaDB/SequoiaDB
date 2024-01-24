/*******************************************************************************
*@Description : snapshots do select query.
*               $elemMatch/$elemMatchOne/$slice/$default/$include
*@Example : db.snapshot( <snaptype>, [cond], [sel], [sort] )
*@Modify list :
*               2015-01-29  xiaojun Hu  Change
*******************************************************************************/

main( test );

function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   /*【Test Point 1】 contexts snapshot:*
    * SDB_SNAP_CONTEXTS/SDB_SNAP_CONTEXTS_CURRENT*/
   var condObj = {};
   var selObj = {
      "SessionID": { $default: 123 }, "Contexts": { slice: [0, 1] },
      "Contexts": {
         $elemMatch: {
            "Type": "DUMP",
            "Description": "BufferSize:0"
         }
      },
      "Contexts.ContextID": { $include: 1 },
      "Contexts.DataRead": { $include: 1 },
      "Contexts.IndexRead": { $include: 1 }
   };
   var ret1 = db.snapshot( SDB_SNAP_CONTEXTS, condObj, selObj ).toArray();
   var ret2 = db.snapshot( SDB_SNAP_CONTEXTS_CURRENT, condObj, selObj ).toArray();

   /*【Test Point 2】 sessions snapshot:*
    * SDB_SNAP_SESSIONS/SDB_SNAP_SESSIONS_CURRENT*/
   var condObj = {};
   var selObj = {
      "SessionID": { $default: 12345 }, "TotalDataRead": { $include: 1 },
      "TotalIndexRead": { $include: 1 }, "TotalDataWrite": { $include: 1 }
   };
   var ret1 = db.snapshot( SDB_SNAP_SESSIONS, condObj, selObj ).toArray();
   var ret2 = db.snapshot( SDB_SNAP_SESSIONS_CURRENT, condObj, selObj ).toArray();

   /*【Test Point 3】 collection and collectionspace snapshot:*
    * SDB_SNAP_COLLECTIONS/SDB_SNAP_COLLECTIONSPACES*/
   var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, true );
   var condObj = "{\"Name\": \"" + COMMCSNAME + "." + COMMCLNAME + "\"}";
   condObj = JSON.parse( condObj );
   var selObj = {
      "Name": { $default: 123 }, "Details": { $slice: [0, 1] },
      "Details.Group": { $elemMatch: { "ID": 0 } },
      "Details.Group.TotalRecords": { $include: 1 },
      "Details.Group.NodeName": { $include: 1 }
   };
   var ret = db.snapshot( SDB_SNAP_COLLECTIONS, condObj, selObj ).toArray();
   var condObj = "{\"Name\": \"" + COMMCSNAME + "\"}";
   condObj = JSON.parse( condObj );
   var selObj = {
      "Name": { $default: 1 },
      "Collection": { $elemMatch: { "Name": "selector_query_cl" } },
      "Group": { $slice: [0, 1] }
   };
   var ret = db.snapshot( SDB_SNAP_COLLECTIONS, condObj, selObj ).toArray();

   /*【Test Point 4】 database snapshot: SDB_SNAP_DATABASE*/
   var condObj = {};
   var selObj = {
      "TotalNumConnects": { $default: "no" },
      "TotalDataRead": { $include: 1 },
      "TotalMapped": { $include: 1 }
   };
   var ret = db.snapshot( SDB_SNAP_DATABASE, condObj, selObj ).toArray();

   /*【Test Point 5】 system snapshot: SDB_SNAP_SYSTEM*/
   var condObj = {};
   var selObj = {
      "CPU.User": { $default: 123 }, "Memory.TotalRAM": { $include: 1 },
      "Disk.TotalSpace": { $include: 1 }
   };
   var ret = db.snapshot( SDB_SNAP_SYSTEM, condObj, selObj ).toArray();

   /*【Test Point 6】 system snapshot: SDB_SNAP_CATALOG*/
   if( false == commIsStandalone( db ) )
   {
      var condObj = {};
      var selObj = {
         "Name": { $default: 1 }, "Version": { $include: 1 },
         "CataInfo": { $slice: [0, 1] }
      };
      var ret = db.snapshot( SDB_SNAP_CATALOG, condObj, selObj ).toArray();
   }
}
