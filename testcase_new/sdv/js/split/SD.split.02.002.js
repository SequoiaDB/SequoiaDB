/************************************
*@Description: 目标分区组不存在
*@author:     Qiangzhong Deng 2015/10/19
**************************************/

var clName = CHANGEDPREFIX + "_cl";

main( db )
function main ( db )
{
	if( commIsStandalone( db ) ) return;
	commDropCL( db, COMMCSNAME, clName );

	var objCL = commCreateCL( db, COMMCSNAME, clName, { ShardingKey: { no: 1 }, ShardingType: "range" } );
	var insertNum = 10;
	insertData( db, COMMCSNAME, clName, insertNum );
	var srcGroup = getSrcGroup( COMMCSNAME, clName );
	//target group doesnot exist
	var tgtGroup = "non_exist_group_name";
	try
	{
		objCL.split( srcGroup, tgtGroup, 50 );
	}
	catch( e )
	{
		if( e !== -154 )
		{
			throw buildException( "SD.split.02.002", e, "split", "rc=-154", "rc=" + e );
		}
	}
	finally
	{
		commDropCL( db, COMMCSNAME, clName );
	}
}