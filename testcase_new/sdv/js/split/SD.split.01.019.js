/************************************
*@Description: 数据切分后插入大量数据，进行upsert操作（匹配条件更新）
*@author:     Qiangzhong Deng 2015/10/26
**************************************/

var clName = CHANGEDPREFIX + "_cl";

main( db );
function main ( db )
{
   if( commIsStandalone( db ) ) return;

   commDropCL( db, COMMCSNAME, clName );

   var objCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ShardingType: "range" } );

   //prepare 2 groups:the first one is source gorup,the other is target groups
   var tarRgNum = 1;
   var testGroups = getSplitGroups( COMMCSNAME, clName, tarRgNum );
   if( 1 === testGroups.length )
   {
      println( "--least two groups" );
      return;
   }

   var insertNum = 1000;
   insertData( db, COMMCSNAME, clName, insertNum );

   //split with first ShardingKey:no
   println( "--begin split" );
   splitCL( objCL, testGroups[0].GroupName, testGroups[1].GroupName, { no: 500 } );
   println( "--end split" );
   checkRangeClSplitResult( clName, testGroups );

   //upsert(will update) 
   objCL.upsert( { $inc: { no2: 1000 } }, { $and: [{ no: { $gte: 500 } }, { no: { $lt: 1000 } }] } );
   checkUpsertResult( objCL, { no2: { $gte: 1000 } }, 500 );

   //upsert(will insert one record)
   objCL.upsert( { $inc: { no2: 6000 } }, { $and: [{ no: { $gt: 1000 } }, { no: { $lt: 1500 } }] } );
   checkUpsertResult( objCL, { no2: { $et: 6000 } }, 1 );

   commDropCL( db, COMMCSNAME, clName );
}
/************************************
*@Description: check upsert result with count()
*@author：Qiangzhong Deng 2015/10/26
*@parameter:
            objectCL: reference to a collection
            condition: condition of count()
            expectRecordNumber: expect result of count()
**************************************/
function checkUpsertResult ( objectCL, condition, expectRecordNumber )
{
   db.setSessionAttr( { PreferedInstance: "M" } );
   var cnt = objectCL.count( condition );
   try
   {
      if( parseInt( cnt ) !== expectRecordNumber )
      {
         throw buildException( "SD.split.01.019", -1, "checkUpsertResult",
            "count( " + condition + " ) returns " + expectRecordNumber,
            "count( " + condition + " ) returns " + parseInt( cnt ) );
      }
   }
   catch( e )
   {
      throw e;
   }
   println( "--the find count after update is " + cnt );
}