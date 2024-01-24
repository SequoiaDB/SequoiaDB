/************************************
*@Description: detaclCL子表并删除，创建同名子表关联新增数据组，在主表插入数据到该子表（多个coord操作）
*@author:      wangkexin
*@createDate:  2019.3.26
*@testlinkCase: seqDB-10967
**************************************/
main();
function main ()
{
    if( true == commIsStandalone( db ) )
    {
        println( "run mode is standalone" );
        return;
    }

    var csName1 = "cs10967_1";
    var csName2 = "cs10967_2";
    var mainCL_Name = "maincl10967";
    var subCL_Name1 = "subcl10967_1";
    var subCL_Name2 = "subcl10967_2";
    var domain1 = "domain10967_1";
    var domain2 = "domain10967_2";
    var logBackupPath = [];

    var coordGroup = commGetGroups( db, true, "", true, false, true );
    var nodeNum = eval( coordGroup[0].length - 1 );
    if( nodeNum < 2 )
    {
        println( "at least two coord nodes." );
        return;
    }

    var hostname1 = coordGroup[0][1].HostName;
    var svcname1 = coordGroup[0][1].svcname;

    var hostname2 = coordGroup[0][2].HostName;
    var svcname2 = coordGroup[0][2].svcname;

    var db1 = null;
    var db2 = null;
    try
    {
        db1 = new Sdb( hostname1, svcname1 );
        var subCL_Name = new Array();
        subCL_Name.push( subCL_Name1 );
        subCL_Name.push( subCL_Name2 );

        //创建域包含所有组
        println( "------db1 attachCL" );
        var groups = getDataGroupsName();
        db1.createDomain( domain1, groups );
        commCreateCS( db1, csName1, true, "db1 create cs", { "Domain": domain1 } );
        var maincl = readyCL( db1, csName1, mainCL_Name, subCL_Name );
        insertData( maincl );
        var newDataRGNames = createDataGroups( db1, hostname1, 2, logBackupPath );

        //连接另一个coord节点分离子表1，删除该子表，创建相同子表
        println( "------db2 detachCL" );
        db2 = new Sdb( hostname2, svcname2 );
        var maincl2 = db2.getCS( csName1 ).getCL( mainCL_Name );
        maincl2.detachCL( csName1 + "." + subCL_Name1 );
        db2.getCS( csName1 ).dropCL( subCL_Name1 );

        //创建域，指定新组
        println( "------create domain2" );
        db2.createDomain( domain2, newDataRGNames );
        var cs2 = commCreateCS( db2, csName2, true, "db2 create cs", { "Domain": domain2 } );
        cs2.createCL( subCL_Name1, { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 } );
        println( "------db2 attachCL" );
        maincl2.attachCL( csName2 + "." + subCL_Name1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

        //db1插入数据覆盖所有子表和所有组
        insertData( maincl );
        var expDataArr = getExpDataArr();
        checkData( maincl, expDataArr );
    }
    catch( e )
    {
        //将新建组日志备份到/tmp/ci/rsrvnodelog目录下
        var backupDir = "/tmp/ci/rsrvnodelog/10967";
        File.mkdir( backupDir );
        for( var i = 0; i < logBackupPath.length; i++ )
        {
            File.scp( logBackupPath[i], backupDir + "/sdbdiag" + i + ".log" );
        }
        throw e;
    }
    finally
    {
        if( db1 != null )
        {
            db1.close();
        }
        if( db2 != null )
        {
            db2.close();
        }

        //清理环境
        commDropCS( db, csName1, true, "drop CS1 in the end" );
        commDropCS( db, csName2, true, "drop CS2 in the end" );
        if( newDataRGNames !== undefined )
        {
            removeDataRG( db, newDataRGNames );
        }
        try
        {
            db.dropDomain( domain1 );
        }
        catch( e )
        {
            //-214 : SDB_CAT_DOMAIN_EXIST
            if( e != -214 )
            {
                throw e;
            }
        }

        try
        {
            db.dropDomain( domain2 );
        }
        catch( e )
        {
            if( e !== -214 )
            {
                throw e;
            }
        }
    }
}

function readyCL ( db, csName, mainCL_Name, subCL_Name )
{
    var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
    var maincl = commCreateCL( db, csName, mainCL_Name, mainCLOption, true, true );

    var subClOption = { ShardingKey: { "b": 1 }, ShardingType: "hash", AutoSplit: true, ReplSize: 0 };
    for( var i = 0; i < subCL_Name.length; i++ )
    {
        commCreateCL( db, csName, subCL_Name[i], subClOption, true, true );
        var options = { LowBound: { a: i * 1000 }, UpBound: { a: i * 1000 + 1000 } };
        maincl.attachCL( csName + "." + subCL_Name[i], options );
    }

    return maincl;
}

function insertData ( cl )
{
    var dataArray = new Array();
    for( var i = 0; i < 2000; i++ )
    {
        var data = { a: i };
        dataArray.push( data );
    }
    cl.insert( dataArray );
}

function createDataGroups ( db, hostName, groupNum, logBackupPath )
{
    var dataGroupNames = [];
    for( var i = 0; i < groupNum; i++ )
    {
        var rgName = "group10967_" + i;
        dataGroupNames.push( rgName );
        var dataRG = db.createRG( rgName );

        var port = parseInt( RSRVPORTBEGIN ) + ( i * 10 );
        var dataPath = RSRVNODEDIR + "data/" + port;
        var checkSucc = false;
        var times = 0;
        var maxRetryTimes = 10;
        do
        {
            try
            {
                dataRG.createNode( hostName, port, dataPath, { diaglevel: 5 } );
                checkSucc = true;
            }
            catch( e )
            {
                //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
                if( e == -145 || e == -290 )
                {
                    port = port + 10;
                    dataPath = RSRVNODEDIR + "data/" + port;
                }
                else
                {
                    throw "create node failed!  port = " + port + " dataPath = " + dataPath + " errorCode: " + e;
                }
                times++;
            }
        }
        while( !checkSucc && times < maxRetryTimes );
        dataRG.start();
        logBackupPath.push( hostName + ":11790@" + dataPath + "/diaglog/sdbdiag.log" );
    }
    return dataGroupNames;
}

function removeDataRG ( db, dataGroupNames )
{
    for( var i = 0; i < dataGroupNames.length; i++ )
    {
        db.removeRG( dataGroupNames[i] );
    }
}

function getExpDataArr ()
{
    var array = new Array();
    for( var i = 0; i < 1000; i++ )
    {
        var data = { a: i };
        array.push( data );
    }
    for( var i = 1000; i < 2000; i++ )
    {
        var data = { a: i };
        array.push( data );
        array.push( data );
    }
    return array;
}

function checkData ( cl, expArray )
{
    var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
    var rcRecs = new Array();
    while( tmpRecs = cursor.next() )
    {
        rcRecs.push( tmpRecs.toObj() );
    }

    var expRecs = JSON.stringify( expArray );
    var actRecs = JSON.stringify( rcRecs );
    if( expRecs !== actRecs )
    {
        throw buildException( "checkResult", null, "", expRecs, "  " + actRecs );
    }
}