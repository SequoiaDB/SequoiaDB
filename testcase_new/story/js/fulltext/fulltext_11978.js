/***************************************************************************
@Description :seqDB-11978 :集合空间属性不影响固定集合  
@Modify list :
              2018-10-26  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_11978";
   var csName = "testCS_ES_11978";
   dropCS( db, csName );

   //指定集合空间的PageSize、LobPageSize指定为非默认值
   commCreateCS( db, csName, false, "", { PageSize: 4096, LobPageSize: 4096 } );
   var dbcl = commCreateCL( db, csName, clName );

   var textIndexName = "a_11978";
   commCreateIndex( dbcl, textIndexName, { content: "text" } );

   //固定集合属性为默认值(与原集合属性无关)
   var dbOperator = new DBOperator();
   var cappedCLName = dbOperator.getCappedCLName( dbcl, textIndexName );
   var group = commGetCLGroups( db, csName + "." + clName );

   var cappedDB = db.getRG( group[0] ).getMaster().connect();
   var cappedAttr = cappedDB.snapshot( 4, { Name: cappedCLName + "." + cappedCLName } );
   var cappedAttr = cappedAttr.next().toObj();
   if( cappedAttr["Details"][0]["PageSize"] != 65536 || cappedAttr["Details"][0]["LobPageSize"] != 262144 )
   {
      throw new Error( "expect PageSize: " + 65536 + ",actual PageSize: " + cappedAttr["Details"][0]["PageSize"] + ",expect LobPageSize: " + 262144 + ",actual LobPageSize: " + cappedAttr["Details"][0]["LobPageSize"] );
   }

   var esIndexNames = dbOperator.getESIndexNames( csName, clName, textIndexName );
   dropCS( db, csName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
