/******************************************************************************
@Description: seqDB-22417:逐条remove完最后一个块的数据后，继续逐条remove前一个块的记录
@author:2020-7-3    zhaoyu  Init
******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = false;
testConf.csName = "cs22417";
testConf.clName = "cl22417";
testConf.csOpt = { Capped: true };
testConf.clOpt = { Capped: true, Size: 1024 };

main( test );

function test ( testPara )
{
   //插入1个块+1条记录，！！！记录数及记录长度不要改变
   var expRecord = [];
   var obj = {"a": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
   for(var i = 0; i < 174763; ++i)
   {
      testPara.testCL.insert(obj);
      expRecord.push(obj);
   }
   
   //删除最后一个块的最后一条记录
   var _id = testPara.testCL.findOne().sort({_id:-1}).next().toObj()._id;
   testPara.testCL.remove({_id:_id});
   expRecord.pop();
   
   //删除前一个块的最后一条记录
   var _id = testPara.testCL.findOne().sort({_id:-1}).next().toObj()._id;
   testPara.testCL.remove({_id:_id});
   expRecord.pop();
   
   checkRecords( testPara.testCL, null, {a:''}, {_id:1}, -1, 0, expRecord );
}
