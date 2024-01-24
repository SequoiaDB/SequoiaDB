/******************************************************************************
 * @Description   : seqDB-12131:插入数据为特殊字符
                    seqDB-12132:插入数据为多层嵌套对象
                    seqDB-12137:插入记录值为中文字符
 * @Author        : Wu Yan
 * @CreateTime    : 2019.05.29
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12131_12132_12137";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var expRecords = insertRecords( varCL );
   var actRecords = varCL.find().sort( { no: 1 } );
   commCompareResults( actRecords, expRecords, false );
}

function insertRecords ( cl )
{
   var insertObj1 = { _id: 1, "no": 0, "!@#%^&": "&*()?><" };
   var objValue = { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": { "a": 100 } } } } } } } } } };
   var insertObj2 = { _id: 2, "no": 2, name: "��Ǯ�����" };

   var docs = [];
   docs.push( insertObj1 );
   docs.push( { _id: 3, "no": 1, obj: objValue } );
   docs.push( insertObj2 );
   cl.insert( docs );
   return docs;
}
