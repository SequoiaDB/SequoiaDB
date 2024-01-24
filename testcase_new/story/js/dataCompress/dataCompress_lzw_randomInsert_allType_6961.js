/******************************************************************************
 * @Description   : seqDB-6961:批量插入覆盖所有支持的数据类型
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2023.02.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6961";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var dtNumber = 200000;
   var checkRecsNum = 3;

   // insert  
   var tmpTypes = ["int", "long", "float", "OID", "bool", "date", "timestamp",
      "binary", "regex", "object", "array", "null", "string"];
   // 构造随机类型和值
   var dataTypes = getRdmType( tmpTypes );
   var dataValues = getRdmValue( dataTypes );
   insertRecs( cl, dtNumber, dataTypes, dataValues );

   waitDictionary( db, csName, clName );

   // 检查结果，检查组内每个节点数据正确性
   checkResult( rgName, csName, clName, dtNumber, dataTypes, dataValues, checkRecsNum );
}

function insertRecs ( cl, dtNumber, dataTypes, dataValues )
{
   var i = 0;
   var h = 0;
   while( i < dataValues.length )
   {
      for( k = 0; k < dtNumber; k += 50000 )
      {
         var doc = [];
         for( j = 0 + k; j < 50000 + k; j++ )
         {
            doc.push( { a: j + h, dataType: dataTypes[i], typeValue: dataValues[i] } )
         };
         cl.insert( doc );
      }
      h = h + dtNumber;
      i++;
   }
}

function checkResult ( rgName, csName, clName, dtNumber, dataTypes, dataValues, checkRecsNum )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );

         // 检查集合属性
         var clInfo = nodeDB.snapshot( 4, { Name: csName + "." + clName } ).toArray();
         var details = JSON.parse( clInfo[0] ).Details[0];
         assert.equal( details.Attribute, "Compressed", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.CompressionType, "lzw", "clInfo = " + JSON.stringify( clInfo ) );

         // 检查数据总数
         var recsCnt = nodeCL.count();
         assert.equal( recsCnt, dtNumber * dataTypes.length );

         // 随机检查n条记录正确性
         var h = 0;
         for( i = 0; i < dataTypes.length; i++ )
         {
            for( j = 0; j < checkRecsNum; j++ )
            {
               var k = Math.random() * dtNumber + h;
               k = parseInt( k, 10 );  //10: decimal system
               if( dataTypes[i] === "regex" )
               {
                  var recsCnt = nodeCL.find( {
                     a: k, dataType: dataTypes[i],
                     typeValue: { $et: dataValues[i] }
                  } ).count();
               }
               else
               {
                  var recsCnt = nodeCL.find( {
                     a: k, dataType: dataTypes[i],
                     typeValue: dataValues[i]
                  } ).count();
               }
               assert.equal( recsCnt, 1 );
            }
            h = h + dtNumber;
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}

function getRdmType ( tmpTypes )
{

   var dataTypes = new Array;
   var num = 10;
   for( k = 0; k < num; k++ )
   {
      var i = Math.random() * tmpTypes.length;
      i = parseInt( i );
      dataTypes.push( tmpTypes[i] );
   }

   return dataTypes;
}

function getRdmValue ( dataTypes )
{

   var rd = new commDataGenerator();

   var dataValues = new Array;
   for( i = 0; i < dataTypes.length; i++ )
   {
      var tmpValues = rd.getValue( dataTypes[i] );

      dataValues.push( tmpValues );
   }

   return dataValues;
}