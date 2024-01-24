/************************************
*@Description: 源分区组数据为空，进行百分比切分
*@author:     Qiangzhong Deng 2015/10/19
**************************************/

var clName = CHANGEDPREFIX + "_splitcl002003";

main( db );
function main ( db )
{
	if( commIsStandalone( db ) ) return;
	commDropCL( db, COMMCSNAME, clName );

	//create a collection and don't insert any records
	var objCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ShardingType: "range" } );

	//get two groups for split,the first one is source group, the second one is target group
	var maxTarGroupsNumber = 1;
	var testGroups = getSplitGroups( COMMCSNAME, clName, maxTarGroupsNumber );
	if( 1 === testGroups.length )
	{
		println( "--least two groups" );
		return;
	}

	try
	{
		objCL.split( testGroups[0].GroupName, testGroups[1].GroupName, 50 );
	}
	catch( e )
	{
		if( e !== -296 )
		{
			throw buildException( "SD.split.02.003", e, "split", "rc=-296", "rc=" + e );
		}
	}

	commDropCL( db, COMMCSNAME, clName );
}