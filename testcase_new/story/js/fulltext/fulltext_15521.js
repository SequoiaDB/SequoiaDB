/******************************************************************************
@Description :   seqDB-15521:指定_id插入记录，_id字段类型为timestamp，ES从原始集合/固定集合中同步索引数据
                 seqDB-15522:指定_id插入记录，_id字段类型为obj，ES从原始集合/固定集合中同步索引数据
                 seqDB-15453:指定_id插入记录，_id字段类型为int，ES从原始集合/固定集合中同步索引数据
                 seqDB-15454:指定_id插入记录，_id字段类型为long，ES从原始集合/固定集合中同步索引数据
                 seqDB-15455:指定_id插入记录，_id字段类型为double，ES从原始集合/固定集合中同步索引数据
                 seqDB-15456:指定_id插入记录，_id字段类型为decimal，ES从原始集合/固定集合中同步索引数据
                 seqDB-15457:指定_id插入记录，_id字段类型为string，ES从原始集合/固定集合中同步索引数据
                 seqDB-15458:指定_id插入记录，_id字段类型为oid，ES从原始集合/固定集合中同步索引数据
                 seqDB-15459:指定_id插入记录，_id字段类型为bool，ES从原始集合/固定集合中同步索引数据
                 seqDB-15460:指定_id插入记录，_id字段类型为date，ES从原始集合/固定集合中同步索引数据
@Modify list :   2018-9-26  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_15521";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var textIndexName = "a_15521";
   var selectorCond = { a: "" };

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   insertData( dbcl );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20 );

   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20 );

   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   dbcl.insert( { _id: { $timestamp: "1902-01-01-00.00.00.000000" }, a: "timestamp1" } );
   dbcl.insert( { _id: { $timestamp: "2018-01-01-00.00.00.000000" }, a: "timestamp2" } );
   dbcl.insert( { _id: { $timestamp: "2037-12-31-23.59.59.999999" }, a: "timestamp3" } );

   dbcl.insert( { _id: { a: "b" }, a: "obj" } );

   dbcl.insert( { _id: 1, a: "int1" } );
   dbcl.insert( { _id: -2147483648, a: "int2" } );
   dbcl.insert( { _id: 2147483647, a: "int3" } );

   dbcl.insert( { _id: { $numberLong: "-9223372036854775808" }, a: "numberLong1" } );
   dbcl.insert( { _id: { $numberLong: "9223372036854775807" }, a: "numberLong2" } );
   dbcl.insert( { _id: { $numberLong: "54456456" }, a: "numberLong3" } );

   dbcl.insert( { _id: -1.7E+308, a: "double1" } );
   dbcl.insert( { _id: 123.456, a: "double2" } );
   dbcl.insert( { _id: 1.7E+308, a: "double3" } );

   dbcl.insert( { _id: { $decimal: "1.7E+400" }, a: "decimal1" } );

   dbcl.insert( { _id: "string1", a: "string1" } );

   dbcl.insert( { _id: { "$oid": "55713f7953e6769804000001" }, a: "oid1" } );

   dbcl.insert( { _id: true, a: "bool1" } );

   dbcl.insert( { _id: { "$date": "0000-01-01" }, a: "date1" } );
   dbcl.insert( { _id: { "$date": "2012-01-01" }, a: "date2" } );
   dbcl.insert( { _id: { "$date": "9999-12-31" }, a: "date3" } );
}
