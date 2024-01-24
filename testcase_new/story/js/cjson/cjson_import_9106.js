/************************************************************************
*@Description:   seqDB-9106:导入非法timestamp类型数据:
                                 a、函数值不满足要求，如{time:Timestamp(12344*aa)}、{time:Timestamp(1433492413)}、{time:Timestamp("2015-06-05-00-16.10.33.000000")} 
                                 b、函数值格式不正确，如{time:Timestamp2015-06-05-00-16.10.33.000000)}、{time:Timestamp('1433492413', 0)} 
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9106";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：{time:Timestamp(12344*aa)}、{time:Timestamp(1433492413)}、{time:Timestamp("2015-06-05-00-16.10.33.000000")}--------------------------
   //import datas          
   var imprtFile1 = tmpFileDir + "9106a.json";
   var srcDatas1 = "{time:Timestamp(12344*aa)}\n{time:Timestamp(1433492413)}\n{time:Timestamp('2015-16-05-16.10.33.000000')}"
   var rcInfos1 = importData( COMMCSNAME, clName, imprtFile1, srcDatas1 );

   //check the Return Infos of the import datas
   var parseFail1 = 3;
   var importRes1 = 0;
   checkImportReturn( rcInfos1, parseFail1, importRes1 );

   //---------------------test b：{time:Timestamp2015-06-05-00-16.10.33.000000)}、{time:Timestamp('1433492413', 0)}  -------------------------
   //import datas          
   var imprtFile2 = tmpFileDir + "9106b.json";
   var srcDatas2 = "{time:Timestamp2015-06-05-00-16.10.33.000000)}\n{time:Timestamp('1433492413', 0)}"
   var rcInfos2 = importData( COMMCSNAME, clName, imprtFile2, srcDatas2 );

   //check the Return Infos of the import datas
   var parseFail2 = 2;
   var importRes2 = 0;
   checkImportReturn( rcInfos2, parseFail2, importRes2 );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}

