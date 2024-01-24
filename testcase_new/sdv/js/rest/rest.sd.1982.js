/************************************************************************
*@Description:	用rest对多条记录进行创建cl、alter cl、增删改查、切分等操作
               对应testlink用例：seqDB-1982:不并发+多条记录+分区_rest.sd.002
*@Author:  		TingYU  2015/10/29
************************************************************************/
main();

function main ()
{
	var csName = COMMCSNAME + "_restsdv1982";
	var clName = COMMCLNAME + "_restsdv1982";

	try
	{
		if( commIsStandalone( db ) )
		{
			println( " Deploy mode is standalone!" );
			return;
		}
		if( commGetGroupsNum( db ) < 2 )
		{
			println( "This testcase needs at least 2 groups to split cl!" );
			return;
		}

		ready( csName, clName );

		//create split cl
		var groupsofSplit = select2RG();	      //{srcRG:'xx', tgtRG:'xx'}
		var srcRG = groupsofSplit.srcRG;
		var tgtRG = groupsofSplit.tgtRG;
		createCL( csName, clName, srcRG );
		var alterOpt = '{ShardingKey:{a:1}, ShardingType:"hash"}';
		alterCL( csName, clName, alterOpt );

		//insert and split
		var recs = [];
		var totalCnt = 1000;
		for( var i = 0; i < totalCnt; i++ ) 
		{
			var rec = { a: i, b: true, c: "str_" + i.toString() };
			recs.push( rec );
		}
		insertRecs( csName, clName, recs );

		var splitPercent = 50;
		splitCL( csName, clName, srcRG, tgtRG, splitPercent );
		checkSplit( csName, clName, srcRG, tgtRG, totalCnt );


		deleteRecs( csName, clName, recs );
		upsertRecs( csName, clName, recs );
		queryRecs( csName, clName, recs );
		updateRecs( csName, clName, recs );
		countCL( csName, clName, recs.length );
		queryAndRemoveRecs( csName, clName, recs );
		queryAndUpdateRecs( csName, clName, recs );


		dropCL( csName, clName );
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
	}
}

function alterCL ( csName, clName, optStr )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=alter collection',
		'name=' + csName + '.' + clName,
		'options=' + optStr];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );
}

function insertRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	for( var i in recs )
	{
		var recStr = JSON.stringify( recs[i] );

		var curlPara = ['cmd=insert',
			'name=' + csName + '.' + clName,
			'insertor=' + recStr];
		var expErrno = 0;
		var curlInfo = runCurl( curlPara, expErrno );
	}

	checkByQuery( csName, clName, recs );
}

function splitCL ( csName, clName, sourceRG, targetRG, percent )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=split',
		'name=' + csName + '.' + clName,
		'source=' + sourceRG,
		'target=' + targetRG,
		'splitpercent=' + percent];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );
}

function checkSplit ( csName, clName, srcRG, tgtRG, totalCnt )
{
	//get every group count 
	var srcHostPort = db.getRG( srcRG ).getMaster().toString();
	var tgtHostPort = db.getRG( tgtRG ).getMaster().toString();
	var srcCnt = new Sdb( srcHostPort ).getCS( csName ).getCL( clName ).count();
	var tgtCnt = new Sdb( tgtHostPort ).getCS( csName ).getCL( clName ).count();
	srcCnt = Number( srcCnt );
	tgtCnt = Number( tgtCnt );

	//every group should have records
	if( srcCnt <= 0 )
	{
		throw buildException( 'checkSplit()', null,
			'new Sdb(' + srcHostPort + ').' + csName + '.' + clName + '.count()',
			'>0', srcCnt );
	}
	if( tgtCnt <= 0 )
	{
		throw buildException( 'checkSplit()', null,
			'new Sdb(' + srcHostPort + ').' + csName + '.' + clName + '.count()',
			'>0', tgtCnt );
	}

	//sum of every group count = totaol count
	if( totalCnt !== srcCnt + tgtCnt )
	{
		println( 'source group count = ' + srcCnt + ', ' +
			'target group count = ' + tgtCnt + ', ' +
			'total group count  = ' + totalCnt );
		throw buildException( 'checkSplit()', null,
			'compare totalCnt with sum of every groups count',
			'equal', 'not equal' );
	}
}

function deleteRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=delete',
		'name=' + csName + '.' + clName,
		'deletor={a:{$gte:800}}'];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	recs.splice( 800, recs.length - 800 );  //delete elements which index >= 800    
	checkByQuery( csName, clName, recs );
}

function upsertRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=upsert',
		'name=' + csName + '.' + clName,
		'updator={$set:{b:false}}',
		'filter={a:{$lt:400}}',
		'setoninsert={d:9.9}'];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	for( var i = 0; i < 400; i++ )
	{
		recs[i]["b"] = false;
	}
	checkByQuery( csName, clName, recs );
}

function queryRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=query',
		'name=' + csName + '.' + clName,
		'filter={a:{$lt:600}}',
		'skip=300',
		'returnnum=200',
		'sort={a:1}',
		'selector={a:"",b:""}'];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	var expRecs = recs.concat();        //copy array
	expRecs = expRecs.slice( 300, 500 );//just need [300,500) elements
	for( var i in expRecs )
	{
		delete expRecs[i]["c"];          //only need field a and b
		delete expRecs[i]["d"];
	}
	isEqual( expRecs, curlInfo.records );
}

function updateRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=update',
		'name=' + csName + '.' + clName,
		'updator={$set:{b:true}}',
		'filter={a:{$lt:100}}'];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	for( var i = 0; i < 100; i++ )
	{
		recs[i]["b"] = true;
	}
	checkByQuery( csName, clName, recs );
}

function queryAndRemoveRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=queryandremove',
		'name=' + csName + '.' + clName,
		'filter={b:false}',
		'sort={a:1}',
		'selector={a:"",b:""}'];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	var expRtn = recs.slice( 100, 400 );  //expect return [100,400) elements
	recs.splice( 100, 300 );              //delete [100,400) elements in cl
	for( var i in expRtn )
	{
		delete expRtn[i]["c"];             //only need field a and b
		delete expRtn[i]["d"];
	}
}

function queryAndUpdateRecs ( csName, clName, recs )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=queryandupdate',
		'name=' + csName + '.' + clName,
		'updator={$set:{b:null}}',
		'filter={a:{$et:0}}',
		'sort={a:1}',
		'selector={a:"",b:""}',
		'returnnew=true'];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	recs[0]["b"] = null;               //only update element 1
	var expRtn = recs.slice( 0, 1 );    //expect return element 1
	for( var i in expRtn )
	{
		delete expRtn[i]["c"];          //only need field a and b
		delete expRtn[i]["d"];
	}
	isEqual( expRtn, curlInfo.records );
}

function dropCL ( csName, clName )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ["cmd=drop collection", "name=" + csName + '.' + clName];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	var curlPara = ["cmd=drop collection", "name=" + csName + '.' + clName];
	var expErrno = -23;  //cl is not existed
	var curlInfo = runCurl( curlPara, expErrno );
}

function ready ( csName, clName )
{
	println( "\n---Begin to excute " + getFuncName() );

	commDropCL( db, csName, clName, true, true, "drop cl in begin" );
	commCreateCS( db, csName, true, "create cs in begin" ); //create cs in case cs is not existed
}