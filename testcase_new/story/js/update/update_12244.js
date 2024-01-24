/******************************************************************************
 * @Description   : seqDB-12244:set修改字段的值为另一类型
 * @Author        : Wang Wenjing
 * @CreateTime    : 2017.07.25
 * @LastEditTime  : 2021.07.03
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_12244";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   varCL.insert( { a: [1, 2] } );

   varCL.update( { $set: { "a.c": 0 } } );

   checkResult( varCL, {}, [{ a: [1, 2] }] );
   varCL.insert( { a: { a: 3 } } );

   varCL.update( { $set: { "a.a": "b" } } );
   checkResult( varCL, { "a.a": "b" }, [{ "a": { "a": "b" } }] );

   varCL.insert( { a: 3 } );

   varCL.update( { $set: { "a": 0 } } );
   checkResult( varCL, { a: 0 }, [{ a: 0 }, { a: 0 }, { a: 0 }] )
}