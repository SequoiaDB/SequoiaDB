import( "../lib/main.js" );

/************************************
*@Description: 检查静态表返回信息
*@author:      huanghaimei
*@createDate:  2022.12.20
**************************************/
function checkCollectionStat ( cl, isDefault, isExpired, avgNumFields, sampleRecords, totalRecords, totalDataPages, totalDataSize )
{
   var actResult = cl.getCollectionStat().toObj();
   if( totalDataSize != undefined )
   {
      assert.equal( actResult.TotalDataSize, totalDataSize, "实际结果为:" + JSON.stringify( actResult ) );
   }

   var parts = cl.toString().split( ":" );
   var infoArr = parts[1].split( "." );
   var csName = infoArr[1];
   var clName = infoArr[2];
   assert.equal( actResult.TotalDataPages, totalDataPages, "实际结果为:" + JSON.stringify( actResult ) );
   assert.equal( actResult.Collection, csName + "." + clName, "实际结果为:" + JSON.stringify( actResult ) );
   assert.equal( actResult.IsDefault, isDefault, "实际结果为:" + JSON.stringify( actResult ) );
   assert.equal( actResult.IsExpired, isExpired, "实际结果为:" + JSON.stringify( actResult ) );
   assert.equal( actResult.AvgNumFields, avgNumFields, "实际结果为:" + JSON.stringify( actResult ) );
   assert.equal( actResult.SampleRecords, sampleRecords, "实际结果为:" + JSON.stringify( actResult ) );
   assert.equal( actResult.TotalRecords, totalRecords, "实际结果为:" + JSON.stringify( actResult ) );
}

/************************************
*@Description: 插入数据
*@author:      huanghaimei
*@createDate:  2023.1.04
**************************************/
function insertData ( cl, recsNum )
{
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i } );
   }
   cl.insert( docs );
}