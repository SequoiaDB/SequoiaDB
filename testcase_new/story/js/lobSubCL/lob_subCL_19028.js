/************************************
*@Description: seqDB-19028 创建主表，指定LobShardingKeyFirmat和多个分区键
*@author:      luweikang
*@createDate:  2019.8.12
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var mainCSName = COMMCSNAME;
   var mainCLName = "mainCL_19028";

   var cs = commCreateCS( db, mainCSName, true );
   var options = { "IsMainCL": true, "ShardingKey": { "date": 1, "a": 1, "b": 2 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( mainCLName, options );
   } );

}
