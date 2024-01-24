/************************************
*@Description: 数据切分后创建索引，按指定的索引查询
*@author:     Qiangzhong Deng 2015/10/20
**************************************/

var clName = CHANGEDPREFIX + "_cl";

main( db );
function main ( db )
{
   if( commIsStandalone( db ) ) return;

   commDropCL( db, COMMCSNAME, clName );

   var objCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ShardingType: "range" } );

   //prepare 2 groups:the first one is source gorup,the other is target group
   var tarGroupNum = 1
   var testGroups = getSplitGroups( COMMCSNAME, clName, tarGroupNum );
   if( 1 === testGroups.length )
   {
      println( "--least two groups" );
      return;
   }

   var insertNum = 1000;
   insertData( db, COMMCSNAME, clName, insertNum );

   //split with first ShardingKey:no
   splitCL( objCL, testGroups[0].GroupName, testGroups[1].GroupName, { no: 500 } );
   checkRangeClSplitResult( clName, testGroups );

   //create index on no2
   var indexName = "no2Index";
   objCL.createIndex( indexName, { no2: 1 } );
   checkIndex( objCL, indexName );

   commDropCL( db, COMMCSNAME, clName );
}
function checkIndex ( objectCL, idxName )
{
   var indexInfo = objectCL.find( { no2: 100 } ).explain().toArray();
   for( var i = 0; i < indexInfo.length; i++ )
   {
      try
      {
         var tmpIndexInfo = eval( "(" + indexInfo[i] + ")" );
         if( tmpIndexInfo.IndexName === idxName
            && tmpIndexInfo.ScanType === "ixscan" )
         {
            //right situation, so do nothing
         }
         else
         {
            throw buildException( "SD.split.01.020", -1, "checkIndex", "IndexName:" + idxName + " ScanType: ixscan",
               "IndexName:" + tmpIndexInfo.IndexName + " ScanType: ixscan" + tmpIndexInfo.ScanType )
         }
      }
      catch( e )
      {
         throw e;
      }
   }
   println( "--create index success and check the query by index" );
}