/* *****************************************************************************
@discretion: detachNode( )options参数为空
@author：2018-12-12 wangkexin
***************************************************************************** */
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var groupList = getGroup( db );
   var groupName = groupList[0];

   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      db.getRG( groupName ).detachNode( COORDHOSTNAME, RSRVPORTBEGIN );
   } );
}

