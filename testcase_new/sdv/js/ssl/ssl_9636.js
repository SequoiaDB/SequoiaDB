/***************************************************************************
@Description :seqDB-9636:SSL功能开启，使用SSL连接执行元数据操作
@Modify list :
              2016-8-31  wuyan  Init
****************************************************************************/
var clName = CHANGEDPREFIX + "_9636";

function main ( dbs )
{
	// drop collection in the beginning
	commDropCL( dbs, csName, clName, true, true, "drop collection in the beginning" );

	// create cs /cl
	var idxCL = commCreateCL( dbs, csName, clName, {}, true, true );
	//check test result
	CheckGetCL( dbs, csName, clName );
	checkInsert( dbs, csName, clName );

	// drop collection 
	commDropCL( dbs, csName, clName, false, false, "drop colleciton to test dropcl" );
	//check the cl
	CheckGetCL( dbs, csName, clName, true );
	//dropCS
	commDropCS( dbs, csName, false, "seqDB-9636: dropCS failed" );
}

try
{
	main( dbs );
	dbs.close();
}
catch( e )
{
	throw e;
}

/* *****************************************************************************
@discription: check if cl exists using getCL method
@author: wuyan
@parameter
	expectCS:cs to be checked
	expectCL:cl to be checked
	ignoreNotExist:default = false, value: true/false
	message: user define message, default:""
***************************************************************************** */
function CheckGetCL ( db, expectCS, expectCL, ignoreNotExist, message )
{
	if( ignoreNotExist == undefined ) { ignoreNotExist = false; }
	if( message == undefined ) { message = ""; }
	var tmpCS;
	try
	{
		tmpCS = db.getCS( expectCS );
	}
	catch( e )
	{
		throw buildException( "CheckGetCL()", e, "CheckGetCL", "getCS successfully", "getCS fail" );
	}
	try
	{
		tmpCS.getCL( expectCL );
	}
	catch( e )
	{
		if( e === -23 && ignoreNotExist )
		{
			//right situation, so do nothing
		}
		else
		{
			throw buildException( "CheckGetCL()", e, "CheckGetCL", "getCL successfully", "getCL fail" );
		}
	}
}

function checkInsert ( db, cs, cl )
{
	var clTmp = db.getCS( cs ).getCL( cl );
	try
	{
		clTmp.insert( { a: 1 } )
	}
	catch( e )
	{
		throw buildException( "checkInsert()", e, "checkInsert", "insert successfully", "insert fail" );
	}

	try 
	{
		clTmp.find( { a: 1 } )
	}
	catch( e )
	{
		throw buildException( "checkInsert()", e, "checkInsert", "find(after insert) successfully", "find(after insert) fail" );
	}
}