/**************************************************************************
@description:     SEQUOIADBMAINSTREAM-5992：php驱动实现索引统计信息快照接口
@testlink cases:   seqDB-7660
@input:        1 setUp()         环境准备:创建cs、创建cl
               2 test()          插入数据、创建索引、分析并收集统计数据、获取统计信息
               3 tearDown()      清理环境：删除cs、关闭db连接
@output:     success
@modify analyze snapshot:
        2020-09-09 Hailin Zhao init
**************************************************************************/
<?php
include_once dirname(__FILE__).'/../global.php';
include_once dirname(__FILE__).'/../commlib/analyzeUtils.php';
class AnalyzeSnapshot22741 extends PHPUnit_Framework_TestCase
{
   protected static $db;
   private static $csName;
   private static $clName;
   private static $clDB;
   private static $indexName;
   private static $masterGroupName;
    
   public function setUp()
   {
      self::$db = new Sequoiadb();
      $err = self::$db -> connect( globalParameter::getHostName() , 
                                   globalParameter::getCoordPort() ) ;
      analyzeUtils::checkErrno( 0, self::$db -> getError()['errno'] );

      self::$csName = globalParameter::getChangedPrefix()."_cs_22741";
      self::$clName = globalParameter::getChangedPrefix().'_cl_22741';
      self::$indexName = "index_22741";
      if ( analyzeUtils::isStandAlone( self::$db ) ) 
      {
         self::$masterGroupName = ""; 
      }
      else
      {
         $groupNames = analyzeUtils::getDataGroups(self::$db);
         self::$masterGroupName = $groupNames[0]; 
      }
      $options = array( "Group" => self::$masterGroupName );
      $cs = self::$db -> selectCS( self::$csName );
      self::$clDB = $cs -> selectCL( self::$clName, $options );
   }

   function test()
   {
      echo "\n---Begin to insert records.\n";
      $recsArray = array();
      $expCount = 6;
      for( $i = 0; $i < $expCount; $i++ )
      {
         $recsArray[$i]  = array( 'a' => $i );
      }
      self::$clDB -> bulkInsert( $recsArray);
      $actCount = self::$clDB -> count( );
      $this -> assertEquals( $expCount, $actCount );

      echo "\n---Begin to createindex.\n";
      self::$clDB ->createIndex( array( 'a' => 1 ), self::$indexName );
      $this -> assertEquals( 0, self::$db -> getLastErrorMsg()["errno"] );

      echo "\n---Begin to index analyze.\n";
      $collectName = self::$csName.".".self::$clName;
      $option = array( "Collection" => $collectName, "Index" => self::$indexName );
      self::$db -> analyze( $option );
      $this -> assertEquals( 0, self::$db -> getLastErrorMsg()["errno"] );
      
      echo "\n---Begin to get index analyze by snapshot.\n";
      $actCount = $this -> getIndexAnalyzeBySnapshot( SDB_SNAP_INDEXSTATS, self::$indexName, self::$masterGroupName );
      $result = $this -> getFormatResult( $actCount );
      $expectation = array( );
      $expectation["Collection"] = self::$csName.".".self::$clName;
      $expectation["Index"]      = self::$indexName;
      $expectation["GroupName"]  =  self::$masterGroupName;
      $expectation["MinValue"]   =  array( "a" => 0 );
      $expectation["MaxValue"]   = array( "a" => 5 ) ;
      $this -> assertEquals( $expectation, $result );
   }

   function getIndexAnalyzeBySnapshot( $snapType, $indexName, $masterGroupName )
   {
      $condition  = array();
      $result     = array();
      if( analyzeUtils::isStandAlone( self::$db ) )
      {
         $condition["Index"]= $indexName;
      }
      else
      {
         $masterNode =  self::$db -> getGroup( $masterGroupName )-> getMaster();
         if( empty( $masterNode ) ) 
         {
            $err = self::$db -> getLastErrorMsg() ;
            echo "Failed to get the master node, error code: ".$err['errno'];
            return ;
         }
         $nodeName = $masterNode -> getName();
         $condition["Index"] = $indexName;
         $condition["NodeName"] = $nodeName;
      }
      $cursor = self::$db -> snapshot( $snapType, $condition );
      if( empty( $cursor ) ) 
      {
         $err = self::$db -> getLastErrorMsg();
         echo "Failed to call snapshot, error code: ".$err['errno']."\n";
         return ;
      }
      while( $record = $cursor -> next() ) 
      {
         array_push( $result ,$record);
      }
      return $result;
   }
   
   function getFormatResult( $info )
   {
      $result = array();
      if( analyzeUtils::isStandAlone( self::$db ) )
      {
         $detail = $info[0];
         $result["Collection"] = $detail["Collection"];
         $result["Index"]      = $detail["Index"];
         $result["GroupName"]  = $detail["GroupName"];
         $result["MinValue"]   = $detail["MinValue"];
         $result["MaxValue"]   = $detail["MaxValue"];
      }
      else
      {
         $detail     = $info[0];
         $stat_info  = $detail["StatInfo"];
         $group_info = $stat_info[0]["Group"];
         $result["Collection"] = $detail["Collection"];
         $result["Index"]      = $detail["Index"];
         $result["GroupName"]  = $stat_info[0]["GroupName"];
         $result["MinValue"]   = $group_info[0]["MinValue"];
         $result["MaxValue"]   = $group_info[0]["MaxValue"];
      }
      return $result;
   }

   public function tearDown()
   {
      $err = self::$db -> dropCS( self::$csName );
      analyzeUtils::checkErrno( 0, $err['errno'] );
      self::$db -> close();
   }
}
?>
