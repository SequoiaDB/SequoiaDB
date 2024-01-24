/******************************************************************************
 * @Description   : seqDB-26843:开启回收机制，删除组后再删除集合空间
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.08.24
 * @LastEditTime  : 2022.09.05
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test )
function test ()
{
   var csName = "cs_26843";
   var clName = "cl_26843";
   var groupsArray = testPara.groups;
   var hostName = groupsArray[0][1].HostName;
   var groupNames = ["group_26843_0"];
   try
   {
      createDataGroups( db, hostName, groupNames );
      commCheckBusinessStatus( db );

      commDropCS( db, csName );
      cleanRecycleBin( db, csName );

      var dbcs = commCreateCS( db, csName );
      dbcs.createCL( clName + "_1", { Group: groupNames[0] } );
      dbcs.createCL( clName + "_2", { Group: groupsArray[0][0]["GroupName"] } );
      dbcs.dropCL( clName + "_1" );
      db.removeRG( groupNames[0] );
      db.dropCS( csName );
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