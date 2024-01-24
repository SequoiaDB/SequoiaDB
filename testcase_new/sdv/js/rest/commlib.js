/****************************************************
@description: commlib of rest sdv
@modify list:
              2015-10-28 Ting YU init
****************************************************/

/****************************************************************************
@discription: generate curl command
@parameter:
   curlPara: eg:[ "cmd=query", "name=foo.bar" ]
@return object ex:
   object.errno: 0
   object.records: [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }]
   object.curlCommand: 'curl http://127.0.0.1:11814/ -d "cmd=query&name=foo.bar"'
*****************************************************************************/
function runCurl ( curlPara, expErrno )
{
	var curlCommand = geneCurlCommad( curlPara );//generate curl command

	//run	 
	try
	{
		var cmd = new Cmd();
		var rtnInfo = cmd.run( curlCommand );
	}
	catch( e )
	{
		throw buildException( "runCurl():run", null,
			curlCommand, "excute succed", "excute fail" );
	}

	//check return value
	var curlInfo = resolveRtnInfo( rtnInfo );
	curlInfo["curlCommand"] = curlCommand;

	if( curlInfo.errno !== expErrno )
	{
		println( "\nreturn information: " + rtnInfo );
		throw buildException( "runCurl():check return value", null,
			curlInfo.curlCommand, expErrno, curlInfo.errno );
	}

	return curlInfo;
}

/****************************************************************************
@discription: generate curl command
@parameter:
   curlPara: eg:[ "cmd=query", "name=foo.bar" ]
@return string
   eg:'curl http://127.0.0.1:11814/ -d "cmd=query&name=foo.bar" 2>/dev/null'
*****************************************************************************/
function geneCurlCommad ( curlPara )
{
	//head of curl command
	var restPort = parseInt( COORDSVCNAME, 10 ) + 4;
	var curlHead = 'curl http://' + COORDHOSTNAME + ':' + restPort + '/ -d';

	//main of curl command
	var curlMain = "'";     //begin with ', not with "
	for( var i in curlPara )
	{
		curlMain += curlPara[i];
		curlMain += '&';
	}
	curlMain = curlMain.substring( 0, curlMain.length - 1 ); //remove last character &
	curlMain += "'";        //end with ', not with "

	//tail of curl command
	var curlTail = '2>/dev/null';

	var curlCommand = curlHead + ' ' + curlMain + ' ' + curlTail;
	return curlCommand;
}

/****************************************************************************
@discription: resolve returned infomation by curl commad
@parameter:
   rtnInfo: eg:{ "errno": 0 }{ "_id": 1, "a": 1 }{ "_id": 2, "a": 2 }
@return object ex:
   object.errno: 0
   object.records: [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }]
*****************************************************************************/
function resolveRtnInfo ( rtnInfo )
{
	var rtn = {};

	//change to array
	var rtnInfoArr = rtnInfo.replace( /}{/g, "}\n{" ).split( "\n" );

	//get errno   
	var errnoStr = rtnInfoArr[0].slice( 11, 15 );
	var errno = parseInt( errnoStr, 10 );
	rtn["errno"] = errno;

	//get records
	rtnInfoArr.shift();      //abandon rtnInfoArr[0]
	for( var i in rtnInfoArr )
	{
		rtnInfoArr[i] = JSON.parse( rtnInfoArr[i] );
	}
	rtn["records"] = rtnInfoArr; //"records"这个字段名不太好

	return rtn;
}

function createCS ( csName )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=create collectionspace', 'name=' + csName]
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );
}

function createCL ( csName, clName, group )
{
	println( "\n---Begin to excute " + getFuncName() );

	if( group == undefined )
	{
		var curlPara = ['cmd=create collection',
			'name=' + csName + '.' + clName,
			'options={ReplSize:-1}'];
	}
	else
	{
		var curlPara = ['cmd=create collection',
			'name=' + csName + '.' + clName,
			'options={ReplSize:-1, Group:"' + group + '"}'];
	}
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );
}

function countCL ( csName, clName, expCnt )
{
	println( "\n---Begin to excute " + getFuncName() );

	var curlPara = ['cmd=get count',
		'name=' + csName + '.' + clName];
	var expErrno = 0;
	var curlInfo = runCurl( curlPara, expErrno );

	//compare with expected cnt
	var actAct = curlInfo["records"][0]["Total"];
	if( actAct != expCnt )
	{
		throw buildException( "compare with expected cnt", null,
			curlInfo.curlCommand, expCnt, actAct );
	}
}

function checkByQuery ( csName, clName, expRecs )
{
	//run query command
	var curlPara = ['cmd=query',
		'name=' + csName + '.' + clName,
		'sort={a:1}'];
	var expErrno = 0;

	var queryTimes = 0;
	var asExpect = false;
	while( queryTimes < 3000 )
	{
		queryTimes++;
		var curlInfo = runCurl( curlPara, expErrno );
		var actRecs = curlInfo.records;

		//check count
		if( actRecs.length === expRecs.length )
		{
			asExpect = true;
			break;
		}
		else
		{
			sleep( 100 );
		}
	}

	if( asExpect === false )
	{
		throw buildException( "checkByQuery(), check count by 5 minutes", null, curlInfo.curlCommand,
			expRecs.length, actRecs.length );
	}

	//check every records every fields
	for( var i in expRecs )
	{
		var actRec = actRecs[i];
		var expRec = expRecs[i];
		for( var f in expRec )
		{
			if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
			{
				println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
				println( "\nactual recs in cl= " + JSON.stringify( actRecs[i] ) + "\n\nexpect recs= " + JSON.stringify( expRecs[i] ) );
				throw buildException( "checkByQuery(), check fields", "rec ERROR" );
			}
		}
	}

}

function select2RG ()
{
	var dataRGInfo = commGetGroups( db );
	var rgsName = {};
	rgsName.srcRG = dataRGInfo[0][0]["GroupName"]; //source group
	rgsName.tgtRG = dataRGInfo[1][0]["GroupName"]; //target group

	return rgsName;
}

function isEqual ( arr1, arr2 )
{
	//check array length
	if( arr1.length !== arr2.length )
	{
		println( "array1 length= " + arr1.length +
			", array2 length= " + arr2.length );
		throw buildException( "isEqual(), check array length", null,
			"compare with array1 and array2",
			"equal", "not equal" );
	}

	//check every element
	for( var i in arr1 )
	{
		var element1 = arr1[i];
		var element2 = arr2[i];

		if( JSON.stringify( element1 ) !== JSON.stringify( element2 ) )
		{
			println( "a element in array1= " + JSON.stringify( element1 ) +
				", a element in array2= " + JSON.stringify( element2 ) );
			throw buildException( "isEqual(), check every element", null,
				"compare with element1 and element2",
				"equal", "not equal" );
		}
	}

	return true;
}

function getFuncName ()
{
	var func = getFuncName.caller.toString();
	var re = /function\s*(\w*)/i;
	var funcName = re.exec( func );

	return funcName[1] + "()";
}
