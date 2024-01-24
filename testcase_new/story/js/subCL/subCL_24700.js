/*****************************************************************************
@Description : seqDB-24700:主子表逆序排序也能利用主子表顺序（主表的分区键设置多字段）
@Author      : xiaozhenfan
@CreateTime  : 2021.12.1
@LastEditTime: 2022.2.8
@LastEditors : xiaozhenfan
******************************************************************************/
testConf.skipStandAlone = true;

main( test ) ;
function test( )
{
    dataGroupNames = commGetDataGroupNames( db ) ;
    var csName = "cs_24700" ;
    var mainCLName = "maincl_24700" ;
    var mainCLOpt = { IsMainCL:true, ShardingKey:{a:1,b:-1} } ;
    var subCLName1 = "subcl1_24700" ;
    var subCLName2 = "subcl2_24700" ;
    var subCLOpt = { Group:dataGroupNames[0] } ;
    var subCLFullName1 = "cs_24700.subcl1_24700" ;
    var subCLFullName2 = "cs_24700.subcl2_24700" ;
    var subCLFullOpt1 = { LowBound:{a:0,b:1000},UpBound:{a:0,b:0} } ;
    var subCLFullOpt2 = { LowBound:{a:1000,b:1000},UpBound:{a:2000,b:0} } ;
    commDropCS( db, csName, true, "clear collectionSpace in the beginning", {EnsureEmpty:false} ) ;
    commCreateCS ( db, csName ) ;

    var mainCL = commCreateCL( db, csName, mainCLName, mainCLOpt ) ;
    commCreateCL( db, csName, subCLName1, subCLOpt ) ;
    commCreateCL( db, csName, subCLName2, subCLOpt ) ;
    mainCL.attachCL( subCLFullName1, subCLFullOpt1 ) ;
    mainCL.attachCL( subCLFullName2, subCLFullOpt2 ) ;
    var needReorder = mainCL.find({},{}).sort({a:-1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:-1,b:1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:1,b:1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, true ) ;
    needReorder = mainCL.find({},{}).sort({a:1,b:-1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, false ) ;
    needReorder = mainCL.find({},{}).sort({a:-1,b:-1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, true ) ;
    commDropCS( db, csName, true, "clear collectionSpace in the endning", {EnsureEmpty:false} ) ;
}
