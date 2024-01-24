/*****************************************************************************
@Description : seqDB-24695:主子表逆序排序也能利用主子表顺序（主表的分区键设置单字段）
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
    var csName = "cs_24695" ;
    var mainCLName = "maincl_24695" ;
    var mainCLOpt = { IsMainCL:true, ShardingKey:{a:1} } ;
    var subCLName1 = "subcl1_24695" ;
    var subCLName2 = "subcl2_24695" ;
    var subCLOpt = { Group:dataGroupNames[0] } ;
    var subCLFullName1 = "cs_24695.subcl1_24695" ;
    var subCLFullName2 = "cs_24695.subcl2_24695" ;
    var subCLFullOpt1 = { LowBound:{a:0},UpBound:{a:1000} } ;
    var subCLFullOpt2 = { LowBound:{a:1000},UpBound:{a:2000} } ;
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
    var needReorder = mainCL.find({},{}).sort({a:1}).explain({Detail:true,Expand:true})
        .next().toObj().PlanPath.ChildOperators[0].PlanPath.NeedReorder ;
    assert.equal( needReorder, false ) ;
    commDropCS( db, csName, true, "clear collectionSpace in the endning", {EnsureEmpty:false} ) ;
}
