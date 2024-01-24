/************************************************************************
*@Description:	此脚本包含两个测试用例：
               seqDB-827:主表下挂载多个子表,在主表批量插入不同分区范围的数据_SD.subCL.01.014
					seqDB-824:挂载子表,在主表做truncate_SD.subCL.01.011
*@Author:  		TingYU  2015/10/22
************************************************************************/
main();

function main ()
{
	try
	{
		if( commIsStandalone( db ) )
		{
			println( " Deploy mode is standalone!" );
			return;
		}
		if( commGetGroupsNum( db ) < 2 )
		{
			println( "This testcase needs at least 2 groups to split sub cl!" );
			return;
		}

		var csName = COMMCSNAME;  //maincl and subcl has the same cs
		var mainCLName = COMMCLNAME + "_maincl";
		var subCLName1 = COMMCLNAME + "_subcl1";
		var subCLName2 = COMMCLNAME + "_subcl2";
		var subCLName3 = COMMCLNAME + "_subcl3";

		//create main&sub cl
		var maincl = createMainCL( csName, mainCLName );
		var groupsOfSplit = select2RG();
		var subcl1 = createSubCL( csName, subCLName1, groupsOfSplit );
		var subcl2 = createSubCL( csName, subCLName2, groupsOfSplit );
		var subcl3 = createSubCL( csName, subCLName3, groupsOfSplit );
		attachCL( maincl, csName, subCLName1, subCLName2, subCLName3 );

		//insert and check
		var istRecNum = 300;
		insertRecs( maincl, istRecNum );
		insertErrorRecs( maincl );
		checkResult1( maincl, subcl1, subcl2, subcl3, istRecNum );//check result after insert

		//truncate and check
		truncateRecs( maincl );
		var istRecNum = 0;
		checkResult2( maincl, subcl1, subcl2, subcl3, istRecNum );//check result after truncate

		clean( csName, mainCLName, subCLName1, subCLName2, subCLName3 );
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
	}
}

function createMainCL ( csName, mainCLName )
{
	println( "\n---Begin to create main cl" );

	commDropCL( db, csName, mainCLName, true, true, "drop main cl in begin" );
	var option = { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true };
	var cl = commCreateCL( db, csName, mainCLName, option, true, false,
		"create mian cl in begin" );
	return cl;
}

function select2RG ()
{
	var dataRGInfo = commGetGroups( db );
	var rgsName = {};
	rgsName.sourceRG = dataRGInfo[0][0]["GroupName"];
	rgsName.targetRG = dataRGInfo[1][0]["GroupName"];

	return rgsName;
}

function createSubCL ( csName, clName, groupsOfSplit )
{
	println( "\n---Begin to create sub cl" );

	commDropCL( db, csName, clName, true, true, "drop main cl in begin" );

	var option = {
		ShardingKey: { b: 1 },
		ShardingType: "hash",
		Group: groupsOfSplit.sourceRG
	};
	var cl = commCreateCL( db, csName, clName, option, true, false,
		"create sub cl in begin" );

	cl.split( groupsOfSplit.sourceRG, groupsOfSplit.targetRG, 50 );

	return cl;
}

function attachCL ( maincl, csName, subCLName1, subCLName2, subCLName3 )
{
	println( "\n---Begin to attach cl" );

	maincl.attachCL( csName + '.' + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 100 } } );
	maincl.attachCL( csName + '.' + subCLName2, { LowBound: { a: 100 }, UpBound: { a: 200 } } );
	maincl.attachCL( csName + '.' + subCLName3, { LowBound: { a: 200 }, UpBound: { a: 300 } } );
}

function insertRecs ( maincl, cnt )
{
	println( "\n---Begin to insert records" );

	for( i = 0; i < cnt; i++ )
	{
		maincl.insert( { a: i, b: i } );
	}
}

function insertErrorRecs ( maincl )
{
	println( "\n---Begin to insert error records" );

	try
	{
		maincl.insert( { a: 300 } );
		var throwErr135 = false;
	}
	catch( e )
	{
		if( e === -135 )
		{
			throwErr135 = true;
		}
		else
		{
			throw buildException( "", null, "maincl.insert({a:300})", "e=-135", "e=" + e );
		}
	}
	if( throwErr135 === false )
	{
		throw buildException( "", null, "maincl.insert({a:300})",
			"e=-135", "did not throw error" );
	}
}

function checkResult1 ( maincl, subcl1, subcl2, subcl3, totalCnt )//check result after insert
{
	println( "\n---Begin to check result after insert" );

	var mainclCnt = parseInt( maincl.count() );
	var subcl1Cnt = parseInt( subcl1.count() );
	var subcl2Cnt = parseInt( subcl2.count() );
	var subcl3Cnt = parseInt( subcl3.count() );

	//check maincl count 
	if( mainclCnt !== totalCnt )
	{
		throw buildException( "check maincl count", null, "maincl.count()",
			totalCnt, mainclCnt );
	}

	//compare maincl count to sum of all subcl count
	if( mainclCnt !== ( subcl1Cnt + subcl2Cnt + subcl3Cnt ) )
	{
		println( "maincl count:" + mainclCnt +
			", subcl1 count:" + subcl1Cnt +
			", subcl2 count:" + subcl2Cnt +
			", subcl3 count:" + subcl3Cnt );
		throw buildException( "", null, "compare maincl count to sum of all subcl count",
			"equal", "no equal" );
	}

	//check subcl1
	var expCnt = totalCnt / 3;
	var cnt = subcl1.count( { $and: [{ a: { $gte: 0 } }, { a: { $lt: 100 } }] } );
	if( parseInt( cnt ) !== expCnt )
	{
		throw buildException( "check subcl:" + subCLName1, null,
			"cl.count({$and:[{a:{$gte:0  }},{a:{$lt:100}}]})",
			expCnt, cnt );
	}

	//check subcl2
	var expCnt = totalCnt / 3;
	var cnt = subcl2.count( { $and: [{ a: { $gte: 100 } }, { a: { $lt: 200 } }] } );
	if( parseInt( cnt ) !== expCnt )
	{
		throw buildException( "check subcl:" + subCLName2, null,
			"cl.count({$and:[{a:{$gte:100}},{a:{$lt:200}}]})",
			expCnt, cnt );
	}

	//check subcl3
	var expCnt = totalCnt / 3;
	var cnt = subcl3.count( { $and: [{ a: { $gte: 200 } }, { a: { $lt: 300 } }] } );
	if( parseInt( cnt ) !== expCnt )
	{
		throw buildException( "check subcl3:" + subCLName3, null,
			"cl.count({$and:[{a:{$gte:200}},{a:{$lt:300}}]})",
			expCnt, cnt );
	}

	//compare every records 
	var expCnt = 1;
	for( var k = 0; k < mainclCnt; k++ )
	{
		var cnt = maincl.find( { a: k } ).count();
		if( parseInt( cnt ) !== expCnt )
		{
			throw buildException( "compare every records", null,
				"maincl.find({a:" + k + "}).count()", expCnt, cnt );
		}
	}
}

function truncateRecs ( cl )
{
	println( "\n---Begin to truncate" );
	cl.truncate();
}

function checkResult2 ( maincl, subcl1, subcl2, subcl3, totalCnt )//check result after truncate
{
	println( "\n---Begin to check result after truncate" );

	var mainclCnt = parseInt( maincl.count() );
	var subcl1Cnt = parseInt( subcl1.count() );
	var subcl2Cnt = parseInt( subcl2.count() );
	var subcl3Cnt = parseInt( subcl3.count() );

	//check maincl count 
	if( mainclCnt !== totalCnt )
	{
		throw buildException( "check maincl count", null, "maincl.count()",
			totalCnt, mainclCnt );
	}

	//compare maincl count to sum of all subcl count
	if( mainclCnt !== ( subcl1Cnt + subcl2Cnt + subcl3Cnt ) )
	{
		println( "maincl count:" + mainclCnt +
			", subcl1 count:" + subcl1Cnt +
			", subcl2 count:" + subcl2Cnt +
			", subcl3 count:" + subcl3Cnt );
		throw buildException( "", null, "compare maincl count to sum of all subcl count",
			"equal", "no equal" );
	}
}

function clean ( csName, mainCLName, subCLName1, subCLName2, subCLName3 )
{
	println( "\n---begin to clean" );

	commDropCL( db, csName, subCLName1, true, true, "drop sub cl1 in clean" );
	commDropCL( db, csName, subCLName2, true, true, "drop sub cl2 in clean" );
	commDropCL( db, csName, subCLName3, true, true, "drop sub cl3 in clean" );
	commDropCL( db, csName, mainCLName, true, true, "drop main cl in clean" );
}

