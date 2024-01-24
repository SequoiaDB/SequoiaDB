/******************************************************************************
 * @Description   : seqDB-23852:组上有回收站，删除组
 * @Author        : liuli
 * @CreateTime    : 2022.03.02
 * @LastEditTime  : 2022.08.30
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23852";
   var clName = "cl_23852";
   var groupsArray = commGetGroups( db );
   var hostName = groupsArray[0][1].HostName;
   var groupNames = ["group_23852_0", "group_23852_1", "group_23852_2"];
   try
   {
      //新创建3个组
      createDataGroups( db, hostName, groupNames );
      commCheckBusinessStatus( db );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );

      var dbcs = commCreateCS( db, csName );
      var dbcl = dbcs.createCL( clName, { Group: groupNames[0] } );
      var docs = []
      for( var i = 0; i < 1000; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );
      db.dropCS( csName );

      // 删除数据组
      for( var i in groupNames )
      {
         db.removeRG( groupNames[i] );
      }

      // 恢复dropCS项目
      var recycleName = getOneRecycleName( db, csName, "Drop" );
      assert.tryThrow( SDB_CLS_GRP_NOT_EXIST, function()
      {
         db.getRecycleBin().returnItem( recycleName );
      } );

      // 重建与groupNames[0]同名的数据组
      createDataGroups( db, hostName, [groupNames[0]] );

      // 恢复dropCS项目
      assert.tryThrow( SDB_CLS_GRP_NOT_EXIST, function()
      {
         db.getRecycleBin().returnItem( recycleName );
      } );

      // 清理dropCS项目
      db.getRecycleBin().dropItem( recycleName );
   }
   finally
   {
      commDropCS( db, csName );
      cleanRecycleBin( db, csName );
      removeDataRG( db, groupNames );
   }
}

function createDataGroups ( db, hostName, groupNames )
{
   for( var i in groupNames )
   {
      var port = parseInt( RSRVPORTBEGIN ) + ( i * 10 );
      var rgName = groupNames[i]
      var dataRG = db.createRG( rgName );
      dataRG.createNode( hostName, port, RSRVNODEDIR + "/" + port, { diaglevel: 5 } );
      dataRG.start();
   }
}

function removeDataRG ( db, groupNames )
{
   for( var i in groupNames )
   {
      try
      {
         db.removeRG( groupNames[i] );
      }
      catch( e )
      {
         if( e != SDB_CLS_GRP_NOT_EXIST )
         {
            throw new Error( e );
         }
      }
   }
}