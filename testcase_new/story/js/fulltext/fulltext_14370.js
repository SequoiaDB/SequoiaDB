/***************************************************************************
@Description :seqDB-14370 :查询全文索引    
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

   var clName = COMMCLNAME + "_ES_14370";
   dropCL( db, COMMCSNAME, clName, true, true );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.createIndex( "fullIndex_14370", { content: "text" } );

   var indexes = dbcl.listIndexes();
   var actIndexes = new Array();
   while( indexes.next() )
   {
      var index = indexes.current().toObj();
      actIndexes.push( index );
   }
   actIndexes = getActualIndexes( actIndexes );

   //获取预期的索引结果
   var dbOperator = new DBOperator();
   var cappedCLName = dbOperator.getCappedCLName( dbcl, "fullIndex_14370" );
   var expIndexes = getExpectIndexes( cappedCLName );

   expIndexes.sort( compare( "Type" ) );
   actIndexes.sort( compare( "Type" ) );
   checkResult( expIndexes, actIndexes );

   //get全文索引
   var index = dbcl.getIndex( "fullIndex_14370" );
   var actIndexes = new Array();
   actIndexes.push( index.toObj() );
   actIndexes = getActualIndexes( actIndexes );
   var expIndexes = getExpectIndexes( cappedCLName );
   expIndexes.pop();

   checkResult( expIndexes, actIndexes );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_14370" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function getExpectIndexes ( cappedCLName )
{
   var arrayIndexes = new Array();
   var index = { "IndexDef": { "name": "fullIndex_14370", "key": { "content": "text" }, "v": 0, "unique": false, "dropDups": false, "enforced": false }, "IndexFlag": "Normal", "Type": "Text", "ExtDataName": cappedCLName };
   arrayIndexes.push( index );
   var index = { "IndexDef": { "name": "$id", "key": { "_id": 1 }, "v": 0, "unique": true, "dropDups": false, "enforced": true }, "IndexFlag": "Normal", "Type": "Positive" };
   arrayIndexes.push( index );
   return arrayIndexes;
}

function getActualIndexes ( actIndexes )
{
   var actualIndexes = new Array();
   for( var i in actIndexes )
   {
      var obj = new Object();
      obj["IndexDef"] = new Object();
      obj["IndexDef"]["name"] = actIndexes[i]["IndexDef"]["name"];
      obj["IndexDef"]["key"] = actIndexes[i]["IndexDef"]["key"];
      obj["IndexDef"]["v"] = actIndexes[i]["IndexDef"]["v"];
      obj["IndexDef"]["unique"] = actIndexes[i]["IndexDef"]["unique"];
      obj["IndexDef"]["dropDups"] = actIndexes[i]["IndexDef"]["dropDups"];
      obj["IndexDef"]["enforced"] = actIndexes[i]["IndexDef"]["enforced"];
      obj["IndexFlag"] = actIndexes[i]["IndexFlag"];
      obj["Type"] = actIndexes[i]["Type"];
      if( actIndexes[i]["ExtDataName"] )
      {
         obj["ExtDataName"] = actIndexes[i]["ExtDataName"];
      }
      actualIndexes.push( obj );
   }
   return actualIndexes;
}
