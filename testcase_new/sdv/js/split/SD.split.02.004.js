/************************************
*@Description: 范围切分时，指定分区范围不正确
*@author:     Qiangzhong Deng 2015/10/20
**************************************/

var clName = CHANGEDPREFIX + "_splitcl002004";

main( db );
function main ( db )
{
	if( commGetGroupsNum( db ) < 2 ) return;
	commDropCL( db, COMMCSNAME, clName );

	//create a collection and don't insert any records
	var objCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ShardingType: "range" } );
	var srcGroup = getSrcGroup( COMMCSNAME, clName );
	var tgtGroup = getOneTargetGroup( db, srcGroup );

	//insert data and range split,range doesnot include any data in source group
	var insertNum = 100;
	insertData( db, COMMCSNAME, clName, insertNum );
	splitCL( objCL, srcGroup, tgtGroup, { no: 100 }, { no: 200 } );

	//insert data and check data location
	checkDataLocation( db, COMMCSNAME, clName, tgtGroup );

	commDropCL( db, COMMCSNAME, clName );
}
function getOneTargetGroup ( sdb, sourceGroup )
{
	var targetGroup;
	var allGroups = getGroupName( db, true );
	for( var i = 0; i < allGroups.length; i++ )
	{
		if( allGroups[i][0] !== sourceGroup )
		{
			targetGroup = allGroups[i][0];
			break;
		}
	}
	return targetGroup;
}
function checkDataLocation ( sdb, cs, cl, targetGroup )
{
	var insertNum = 200;
	insertData( sdb, cs, cl, insertNum );
	var svcName;
	var allGroups = getGroupName( db, true );
	for( var i = 0; i < allGroups.length; i++ )
	{
		if( allGroups[i][0] === targetGroup )
		{
			svcName = allGroups[i][2];
			break;
		}
	}

	var tmpDB = new Sdb( allGroups[i][1], svcName );
	try
	{
		var cnt = tmpDB.getCS( cs ).getCL( cl ).count();
		println( "the find count of " + allGroups[i][1] + ":" + svcName + " is " + cnt )
		if( Number( cnt ) !== 100 )
		{
			println( "HostName:" + COORDHOSTNAME + " svcName:" + svcName + "cs: " + cs + " cl:" + cl );
			throw buildException( "SD.split.02.004", -1, "checkDataLocation", "count() should return 100", "count() returns " + cnt );
		}
	}
	catch( e )
	{
		throw e;
	}
	finally
	{
		tmpDB.close();
	}
}