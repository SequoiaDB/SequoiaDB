/* *****************************************************************************
@discretion: attachNode( )options参数格式非法
@author：2018-12-12 wangkexin
***************************************************************************** */
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var groupList = getGroup( db );
   var groupName = groupList[0];

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRG( groupName ).attachNode( COORDHOSTNAME, RSRVPORTBEGIN, "test" );
   } );
}

