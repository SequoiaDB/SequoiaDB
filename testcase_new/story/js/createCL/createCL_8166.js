/* *****************************************************************************
@discretion: 测试建表时的AutoIndexId选项 
@modify list:
   2014-2-24 wenjing wang  Init
***************************************************************************** */
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var TESTCSNAME = CHANGEDPREFIX + "_8166";
   var TESTCLNAME = CHANGEDPREFIX + "_8166";
   db.setSessionAttr( { "PreferedInstance": 'M' } );

   commDropCS( db, TESTCSNAME );
   var cs = commCreateCS( db, TESTCSNAME, false );
   // 选项字段为全小写，不被允许
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( TESTCLNAME, { autoindexid: false } );
   } );

   // 选项字段值为数字，不被允许
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( TESTCLNAME, { AutoIndexId: 0 } );
   } );

   // 选项字段在主表上使用，不被允许
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( TESTCLNAME, { IsMainCL: true, AutoIndexId: false, ShardingKey: { date: 1 } } );
   } );

   // 不使用AutoIndexId选项的用例没有加，因为原来的测试用例已经覆盖
   createCLwithOrdinaryTable( cs, TESTCLNAME );
   createCLwithHorizontalpartitiontable( db, TESTCSNAME, TESTCLNAME );
   createCLwithVerticalpartitiontable( db, TESTCSNAME, TESTCLNAME );

   commDropCS( db, TESTCSNAME, false );
}


function checkIndexnumber ( cl )
{
   var indexes = cl.listIndexes().toArray();

   assert.equal( indexes.length, 0 );
}

// 普通表上使用该选项
function createCLwithOrdinaryTable ( cs, clname )
{
   var cl = cs.createCL( clname, { AutoIndexId: false } );
   cl.insert( { _id: 1, a: 1 } );
   var num = cl.count();
   assert.equal( num, 1 );

   checkIndexnumber( cl );

   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      cl.update( { $set: { a: 2 } }, { _id: 1 } );
   } );

   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      cl.remove( { _id: 1 } )
   } );

   cs.dropCL( clname );
}

function loaddata ( cl, recordnumber )
{
   for( i = 0; i < recordnumber; ++i )
   {
      cl.insert( { _id: i, a: i, date: 20150000 + 100 * ( i % 2 + 1 ) + i % 29 + 1 } );
   }
}

function checkupdate ( cl, recordnumber )
{
   for( i = 0; i < recordnumber; ++i )
   {
      try
      {
         cl.update( { $inc: { a: 1000 } }, { _id: i } );
      }
      catch( e )
      {
         if( e.message == SDB_RTN_AUTOINDEXID_IS_FALSE )
         {
            continue;
         }
      }
   }

   for( i = 0; i < recordnumber; ++i )
   {
      var cursor = cl.find( { a: i } );
      var cnt = 0;
      while( cursor.next() )
      {
         cnt++;
      }
      assert.equal( cnt, 1 );
   }
}

function checkremove ( cl, recordnumber )
{
   for( i = 0; i < recordnumber; ++i )
   {
      try
      {
         cl.remove( { a: i } )
      }
      catch( e )
      {
         if( e.message == SDB_RTN_AUTOINDEXID_IS_FALSE )
         {
            continue;
         }
      }
   }

   var currecordnum = cl.count();
   assert.equal( currecordnum, recordnumber );
}

// 水平分区表上使用该选项，让其自动切分，所以建在域中
function createCLwithHorizontalpartitiontable ( db, csname, clname )
{
   var domainname = CHANGEDPREFIX + "_8166_tdomain";
   commDropDomain( db, domainname );
   var groupnames = commGetDataGroupNames( db );
   commCreateDomain( db, domainname, groupnames, { AutoSplit: true } );

   commDropCS( db, csname );

   var opt = new Object();
   opt.Domain = domainname;
   var cs = db.createCS( csname, opt );
   var cl = cs.createCL( clname, { ShardingKey: { _id: 1 }, ShardingType: 'hash', AutoSplit: true, AutoIndexId: false, EnsureShardingIndex: false } );

   checkIndexnumber( cl );
   var recordnumber = 1000;
   loaddata( cl, recordnumber );
   checkupdate( cl, recordnumber );
   checkremove( cl, recordnumber );

   cs.dropCL( clname );
   db.dropCS( csname );
   commDropDomain( db, domainname );
}

// 垂直分区表使用该选项
function createCLwithVerticalpartitiontable ( db, csname, clname )
{
   var subopt = new Object();
   subopt.date = 1;

   var opt = new Object();
   opt.IsMainCL = true;
   opt.ShardingKey = subopt;

   var cl = commCreateCL( db, csname, clname, opt, true );
   //var cl = cs.createCL(clname, {IsMainCL:true,ShardingKey:{date:1}});
   var subclname1 = CHANGEDPREFIX + "_8166_t01";
   var subclname2 = CHANGEDPREFIX + "_8166_t02";

   var subcl1 = db.getCS( csname ).createCL( subclname1, { ShardingType: "hash", ShardingKey: { _id: 1 }, AutoIndexId: false, EnsureShardingIndex: false } );
   var subcl2 = db.getCS( csname ).createCL( subclname2, { ShardingType: "hash", ShardingKey: { _id: 1 }, AutoIndexId: false, EnsureShardingIndex: false } );
   cl.attachCL( csname + '.' + subclname1, { LowBound: { date: 20150101 }, UpBound: { date: 20150201 } } );
   cl.attachCL( csname + '.' + subclname2, { LowBound: { date: 20150201 }, UpBound: { date: 20150301 } } );

   checkIndexnumber( cl );
   checkIndexnumber( subcl1 );
   checkIndexnumber( subcl2 );

   var recordnumber = 1000;
   loaddata( cl, recordnumber );
   checkupdate( cl, recordnumber );
   checkremove( cl, recordnumber );

   db.getCS( csname ).dropCL( clname );
}
