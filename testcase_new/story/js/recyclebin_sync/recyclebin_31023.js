/******************************************************************************
 * @Description   : seqDB-31023:获取回收站项目快照，指定Role为all/data/coord/catalog
 * @Author        : Bi Qin
 * @CreateTime    : 2023.04.04
 * @LastEditTime  : 2023.09.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_31023";
   var clName = "cl_31023";
   var fullName = csName + "." + clName;

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   commCreateCL( db, csName, clName, { ReplSize: 0 } );
   dbcs.dropCL( clName );

   var rawdata = [true, false];
   var nodeSelect = ["all", "master", "primary", "any", "secondary"];
   var role = ["all", "data", "catalog", "coord"];

   for( var i in role )
   {
      if( ["catalog", "coord"].indexOf( role[i] ) > -1 ) 
      {
         var cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { Role: role[i], OriginName: fullName } );
         checkNoResults( cursor )
      } else
      {
         var cursor = db.getRecycleBin().snapshot( { Role: role[i], OriginName: fullName } );
         checkCursorData( cursor, fullName );
      }
   }

   for( var i in rawdata )
   {
      var cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { RawData: rawdata[i], OriginName: fullName } );
      checkCursorData( cursor, fullName );

      cursor = db.getRecycleBin().snapshot( { RawData: rawdata[i], OriginName: fullName } );
      checkCursorData( cursor, fullName );
   }

   for( var i in nodeSelect )
   {
      var cursor = db.snapshot( SDB_SNAP_RECYCLEBIN, { NodeSelect: nodeSelect[i], OriginName: fullName } );
      checkCursorData( cursor, fullName );

      cursor = db.getRecycleBin().snapshot( { NodeSelect: nodeSelect[i], OriginName: fullName } );
      checkCursorData( cursor, fullName );
   }

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function checkNoResults ( cursor )
{
   while( cursor.next() )
   {
      throw new Error( JSON.stringify( cursor.current().toObj() ) );
   }
}

function checkCursorData ( cursor, fullName )
{
   assert.equal( cursor.current().toObj().OriginName, fullName );
   cursor.close();
}