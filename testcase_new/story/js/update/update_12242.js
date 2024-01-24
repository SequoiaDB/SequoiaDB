/******************************************************************************
 * @Description   : seqDB-12242:unset不存在的字段
 * @Author        : Zhang Yanan
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12242";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var insertCount = 1000;
   var docs = [];
   for( i = 0; i < insertCount; i++ ) 
   {
      docs.push( { a: i, b: "fdafdsaf$#@$@%$#%#@!$#@!$", c: null, d: { id: 1.0, name: "qiu" }, e: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );
   }
   varCL.insert( docs );

   varCL.update( { "$unset": { noexist: "" } } )

   var rc = varCL.find();
   commCompareResults( rc, docs );
}