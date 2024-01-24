/************************************
*@Description: seqDB-13867:切分表使用Search参数展示访问计划
seqDB-13869:切分表使用Evaluate参数展示访问计划
seqDB-13871:切分表使用Estimate参数展示访问计划
seqDB-13873:切分表使用Expand参数展示访问计划
seqDB-13878:切分表使用Filter参数展示访问计划
seqDB-13880:切分表使用Detail参数展示访问计划
seqDB-13882:切分表使用Run参数展示访问计划
seqDB-14011:切分表使用Flatten参数展示访问计划
*@author:      zhaoyu
*@createdate:  2019.7.13
*@testlinkCase: seqDB-13867
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_13867";
testConf.clOpt = { ShardingKey: { a: 1 }, AutoSplit: true };
main( test );
function test ( args )
{

   var configPath = "./config.txt";
   var dbcl = args.testCL;

   var doc = [];
   for( var i = 0; i < 30000; i++ )
   {
      doc.push( { a: i, b: i, c: i, d: "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" + i } )
   }
   dbcl.insert( doc );

   //读取配置文件config.txt中的参数，进行访问计划展示
   var file = new File( configPath );
   while( true )
   {
      try
      {
         var explainObj = JSON.parse( file.readLine().split( "\n" )[0] );
         var explainCursor = dbcl.find( { a: { $in: [1, 10000] } } ).explain( explainObj );
         while( explainCursor.next() ) { };
      }
      catch( e )
      {
         if( e.message == SDB_EOF )
         {
            break;
         }
         else
         {
            throw new Error( e );
         }
      }
   }
}
