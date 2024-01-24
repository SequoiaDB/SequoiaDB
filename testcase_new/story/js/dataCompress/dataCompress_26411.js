/******************************************************************************
 * @Description   : seqDB-26411:构建字典多个数据块，truncate后插入数据，访问无效的数据块
 * @Author        : liuli
 * @CreateTime    : 2022.04.24
 * @LastEditTime  : 2022.04.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.csName = COMMCLNAME + "_26411";
testConf.clName = COMMCLNAME + "_26411";

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;
   dbcl.insert( { a: 1 } );
   // 插入大量数据，达到多个数据块
   for( var i = 500; i < 1000; ++i )
   {
      var doc = [];
      for( var j = i * 10000; j < ( i + 1 ) * 10000; ++j )
      {
         doc.push( { a: "asdfaxxxxxxxxxxxxxxxxxxxxlkjasldkjfasaasdfasdfa;ldkjf;lkasdjf;lajsd;lkfja;lksdjf;klajsdf;lkajsdf;lkjask;ldfja;lksdf" + j } )
      }
      dbcl.insert( doc );
   }

   // 获取数据块信息
   var cursor = dbcl.find().getQueryMeta()
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var datablocks1 = obj["Datablocks"];
   }
   cursor.close();

   // 执行truncate
   dbcl.truncate();
   dbcl.insert( { a: 1 } );
   // 再次插入数据，达到构建字典，数据块数量少于前一次
   for( var i = 0; i < 200; ++i )
   {
      var doc = [];
      for( var j = i * 10000; j < ( i + 1 ) * 10000; ++j )
      {
         doc.push( { a: "yiqopipiojklouqewpoirzcfvasdjpoijqwerfkljasldkjfasaasdfasdfa;ldkjf;lkasdjf;lajsd;lkfja;lksdjf;klajsdf;lkajsdf;lkjask;ldfja;lksdf" + j } )
      }
      dbcl.insert( doc );
   }

   // 获取数据块信息
   var cursor = dbcl.find().getQueryMeta();
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var datablocks2 = obj["Datablocks"];
   }
   cursor.close();

   var diff = getDiff( datablocks1, datablocks2 );
   var equal = getEqual( datablocks1, datablocks2 );

   var cur = dbcl.find().hint( { $Meta: { "ScanType": "tbscan", "Datablocks": equal } } );
   cur.next();
   cur.close();

   var cur = dbcl.find().hint( { $Meta: { "ScanType": "tbscan", "Datablocks": diff } } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cur.next();
      cur.close();
   } );

   var cur = dbcl.find().hint( { $Meta: { "ScanType": "tbscan", "Datablocks": [equal[0], diff[0]] } } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cur.next();
      cur.close();
   } );
}

function getDiff ( array1, array2 )
{
   var diff = [];
   for( var i in array1 )
   {
      if( array2.indexOf( array1[i] ) < 0 )
      {
         diff.push( array1[i] );
      }
   }
   for( var i in array2 )
   {
      if( array1.indexOf( array2[i] ) < 0 )
      {
         diff.push( array2[i] );
      }
   }
   return diff;
}

function getEqual ( array1, array2 )
{
   var equal = [];
   for( var i in array1 )
   {
      if( array2.indexOf( array1[i] ) != -1 )
      {
         equal.push( array1[i] );
      }
   }
   return equal;
}
