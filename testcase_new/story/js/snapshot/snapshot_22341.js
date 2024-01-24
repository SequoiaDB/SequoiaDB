/******************************************************************************
*@Description :   seqDB-22431:生成大BSON查询SDB_SNAP_ACCESSPLANS
*@author:      liyuanyue
*@createdate:  2020.05.09
******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.csName = COMMCSNAME + "cs_snapshot_22431";
testConf.clName = COMMCLNAME + "cl_snapshot_22431";
main( test );

function test ( testPara )
{
   var groupName = testPara.srcGroupName;
   var arr = new Array( 16 * 1024 * 1024 );
   var test = arr.join( 'a' );
   var cs = testPara.testCS;
   var cl = testPara.testCL;
   cl.createIndex( 'a', { a: 1 } );
   cl.find( { a: test } );
   var cur = db.snapshot( SDB_SNAP_ACCESSPLANS, { GroupName: groupName } );
   while( cur.next() )
   {
      errNodes = cur.current().toObj()["ErrNodes"];
      if( errNodes !== undefined )
      {
         throw new Error( "error:errNodes is " + JSON.stringify( errNodes ) );
      }
   }

   cs.createCL( testConf.clName + "_0", { Group: groupName } );

}
