/******************************************************************************
 * @Description   : seqDB-12130:string和array类型记录（记录长度较大）
 * @Author        : Wu Yan
 * @CreateTime    : 2019.05.29
 * @LastEditTime  : 2021.07.02
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12130";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var expRecords = insertRecords( varCL );
   var actRecords = varCL.find();
   commCompareResults( actRecords, expRecords );
}

function getRandomString ( len ) 
{
   var strLen = parseInt( 1500 + ( Math.random() * len ) );
   var str = "";
   var chars = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
   var maxPos = chars.length;
   for( var i = 0; i < strLen; i++ )
   {
      str += chars.charAt( Math.floor( Math.random() * maxPos ) );
   }
   return str;
}

function insertRecords ( cl )
{
   var strLength = 3000;
   var str1 = getRandomString( strLength );
   var str2 = "{name:\"qiu\",balance:1.2}";
   for( var i = 0; i < 1000; ++i )
   {
      str2 = str2 + ",{name:\"qiu\",balance:1.2}";
   }

   var objs = { "str1": str1, "array": [str2] };
   var doc = [];
   doc.push( objs );
   cl.insert( doc );
   return doc;
}
