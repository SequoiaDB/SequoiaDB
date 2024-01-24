/**
 * Copyright (c) 2017, SequoiaDB Ltd. File Name:GroupMgr.java 类的详细描述
 *
 * @author wenjingwang Date:2017-2-23上午10:19:55
 * @version 1.00
 */
package com.sequoiadb.commlib;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.ReliabilityException;

public class GroupMgr {
    private Map< String, GroupWrapper > name2group = new HashMap<>();
    private Map< Integer, GroupWrapper > id2group = new HashMap<>();
    private static GroupMgr mgr = new GroupMgr();
    private Sequoiadb sdb = null;
    private String coordUrl = SdbTestBase.coordUrl;
    private String inputUrl = null;
    private long refreshTime;
    private boolean refreshFlag = false;

    private GroupMgr() {
    }

    /**
     * 刷新 SDB 连接
     * 
     * @param sdb
     */
    public void setSdb( Sequoiadb sdb ) {
        this.sdb = sdb;
        refreshFlag = true;
    }

    public void refresh( String coordUrl ) throws ReliabilityException {
        if ( System.currentTimeMillis() - refreshTime < 1000 && !refreshFlag && coordUrl.equals( inputUrl )) {
            return;
        }
        refreshFlag = false;

        name2group.clear();
        id2group.clear();
        DBCursor cursor = null;

        do {
            try {
                if ( sdb == null || sdb.isClosed() || !sdb.isValid() ) {
                    sdb = new Sequoiadb( coordUrl, "", "" );
                } else if ( !coordUrl.equals( inputUrl ) ) {
                    sdb.close();
                    sdb = new Sequoiadb( coordUrl, "", "" );
                }
                inputUrl = coordUrl;
                cursor = sdb.getList( Sequoiadb.SDB_LIST_GROUPS, null, null,
                        null );
                while ( cursor.hasNext() ) {
                    BasicBSONObject obj = ( BasicBSONObject ) cursor.getNext();

                    String groupName = obj.getString( "GroupName" );

                    GroupWrapper group = new GroupWrapper( obj,
                            sdb.getReplicaGroup( groupName ), this );
                    group.init();
                    name2group.put( groupName, group );
                    id2group.put( group.getGroupID(), group );
                }
                break;
            } catch ( BaseException e ) {
                if ( e.getErrorCode() == -104 ) {
                    continue;
                }
                throw new ReliabilityException( e );
            } finally {
                refreshTime = System.currentTimeMillis();
                if ( cursor != null ) {
                    cursor.close();
                }
            }
        } while ( true );
    }

    public void refresh() throws ReliabilityException {
        refresh( coordUrl );
    }

    public List< GroupWrapper > getAllDataGroup() {
        return getAllDataGroup( coordUrl );
    }

    public List< GroupWrapper > getAllDataGroup( String coordUrl ) {
        try {
            refresh( coordUrl );
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
        }
        List< GroupWrapper > dataGroups = new ArrayList<>();
        for ( Entry< String, GroupWrapper > entry : name2group.entrySet() ) {
            if ( !entry.getKey().equals( "SYSSpare" )
                    && !entry.getKey().equals( "SYSCatalogGroup" )
                    && !entry.getKey().equals( "SYSCoord" ) ) {
                dataGroups.add( entry.getValue() );
            }
        }

        return dataGroups;
    }

    public List< String > getAllDataGroupName() {
        return getAllDataGroupName( coordUrl );
    }

    public List< String > getAllDataGroupName( String coordUrl ) {
        try {
            refresh( coordUrl );
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            return null;
        }
        List< String > names = new ArrayList<>();
        for ( Entry< String, GroupWrapper > entry : name2group.entrySet() ) {
            if ( !entry.getKey().equals( "SYSSpare" )
                    && !entry.getKey().equals( "SYSCatalogGroup" )
                    && !entry.getKey().equals( "SYSCoord" ) ) {
                names.add( entry.getKey() );
            }
        }
        return names;
    }

    public List< String > getAllHosts() {
        return getAllHosts( coordUrl );
    }

    public List< String > getAllHosts( String coordUrl ) {
        try {
            refresh( coordUrl );
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
            return null;
        }
        List< String > hosts = new ArrayList<>();
        Set< String > origHosts = new HashSet<>();
        for ( Entry< String, GroupWrapper > entry : name2group.entrySet() ) {
            Set< String > hostsPerGroup = entry.getValue().getAllHosts();
            origHosts.addAll( hostsPerGroup );
        }
        hosts.addAll( origHosts );
        return hosts;
    }

    private GroupWrapper getGroupByNameInner( String name ) {
        if ( name2group.containsKey( name ) ) {
            return name2group.get( name );
        } else {
            return null;
        }
    }

    public GroupWrapper getGroupByName( String name ) {
        return getGroupByName( name, coordUrl );
    }

    public GroupWrapper getGroupByName( String name, String coordUrl ) {
        try {
            refresh( coordUrl );
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
        }
        return getGroupByNameInner( name );
    }

    public GroupWrapper getGroupById( int id ) {
        if ( id2group.containsKey( id ) ) {
            return id2group.get( id );
        } else {
            return null;
        }
    }

    public static GroupMgr getInstance() throws ReliabilityException {
        mgr.refresh();
        return mgr;
    }

    public static GroupMgr getInstance( String coordUrl )
            throws ReliabilityException {
        mgr.refresh( coordUrl );
        return mgr;
    }

    public boolean checkBusiness( int timeOutSecond, String coordUrl )
            throws ReliabilityException {

        return checkBusiness( timeOutSecond, false, coordUrl );
    }

    /**
     * 持续检测集群当前的状态（组内是否有主，各节点可连接，ServiceStatus），若在timeOutSecond时间内检测仍不通过，
     * 则会打印当前集群状态信息（也可能会抛出异常，如-104，网络错误等等），并返回false
     * Assert.assertEquals(checkBusiness(120), true, "Message");
     *
     * @param timeOutSecond
     *            建议>120（秒）
     * @return
     * @throws ReliabilityException
     */
    public boolean checkBusiness( int timeOutSecond, boolean ignoreIndeploy,
            String coordUrl ) throws ReliabilityException {
        refresh( coordUrl );
        long timestamp = System.currentTimeMillis();
        boolean isPrintRes = false;
        boolean ret = false;
        ReliabilityException prev = null;
        do {
            if ( System.currentTimeMillis() - timestamp >= timeOutSecond
                    * 1000 ) {
                isPrintRes = true;
            }
            try {
                ret = checkBusiness( isPrintRes, ignoreIndeploy, coordUrl );
                if ( ret ) {
                    break;
                }
            } catch ( ReliabilityException e ) {
                if ( prev == null
                        || prev.getExceptionType() != e.getExceptionType() ) {
                    e.printStackTrace();
                    prev = e;
                }
                if ( isPrintRes ) {
                    throw e;
                }
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                // ignore
            }
        } while ( !isPrintRes );

        return ret;
    }

    /**
     * 持续检测集群当前的状态120秒（组内是否有主，各节点可连接，ServiceStatus），若在timeOutSecond时间内检测仍不通过，
     * 则会打印当前集群状态信息（也可能会抛出异常，如-104，网络错误等等），并返回false
     * Assert.assertEquals(checkBusiness(), true, "Message");
     *
     * @return
     * @throws ReliabilityException
     */
    public boolean checkBusiness() throws ReliabilityException {
        return checkBusiness( 120, coordUrl );
    }

    public boolean checkBusiness( int timeOutSecond )
            throws ReliabilityException {
        return checkBusiness( timeOutSecond, coordUrl );
    }

    public boolean checkBusiness( int timeOutSecond, boolean ignoreIndeploy )
            throws ReliabilityException {
        return checkBusiness( timeOutSecond, ignoreIndeploy, coordUrl );
    }

    public boolean checkBusiness( String coordUrl )
            throws ReliabilityException {
        return checkBusiness( 120, coordUrl );
    }

    private void refreshAllGroup() {
        for ( Entry< String, GroupWrapper > entry : name2group.entrySet() ) {
            String name = entry.getKey();
            GroupWrapper group = entry.getValue();
            if ( name.equals( "SYSCoord" ) )
                continue;
            group.refresh();
        }
    }

    /**
     * 检测集群状态（组内是否有主，各节点可连接，ServiceStatus），
     * 失败则根据变量printAndThrowAllException决定是否打印集群信息，是否屏蔽检测过程中发生的异常，并返回false
     *
     * @param printAndThrowAllException
     * @return
     * @throws ReliabilityException
     */
    // TODO:可被替代，屏蔽
    private boolean checkBusiness( boolean printAndThrowAllException,
            boolean ignoreIndeploy, String coordUrl )
            throws ReliabilityException {
        List< GroupCheckResult > results = checkGroup(
                printAndThrowAllException );

        for ( GroupCheckResult result : results ) {
            if ( !result.check( ignoreIndeploy ) ) {
                if ( printAndThrowAllException ) {
                    System.out.println( result.toString() );
                }
                refreshAllGroup();
                return false;
            }
        }

        // 尝试创建一个ReplSize=3的测试集合（检测所有数据节点是否Alive）
        if ( createTestCollection( printAndThrowAllException, coordUrl ) ) {
            // 因为catalog的同步策略不受ReplSize影响，所以删除cl后要等待catalog同步，以免影响外部
            return dropTestCollection( printAndThrowAllException, coordUrl )
                    && waitCatalogSync( printAndThrowAllException );
        } else {
            return false;
        }
    }

    private boolean isCatalogSync( boolean printAndThrowAllException )
            throws ReliabilityException {
        GroupWrapper catagroup = getGroupByNameInner( "SYSCatalogGroup" );
        List< NodeWrapper > nodes = catagroup.getNodes();
        boolean ret = true;
        long prevCount = -1;
        for ( NodeWrapper node : nodes ) {
            Sequoiadb db = node.connect();
            try {
                DBCollection cl = db.getCollectionSpace( "SYSCAT" )
                        .getCollection( "SYSCOLLECTIONS" );
                long count = cl.getCount();
                if ( prevCount == -1 ) {
                    prevCount = count;
                } else if ( prevCount != count ) {
                    ret = false;
                    break;
                }
            } catch ( BaseException e ) {
                ret = false;
                if ( printAndThrowAllException ) {
                    System.out.println(
                            "Check business:failed to query test collection(clForTestBusiness_reliability) on SYSCatalogGroup:"
                                    + e.getErrorCode() );
                }
            } finally {
                db.close();
            }
        }

        if ( printAndThrowAllException && !ret ) {
            System.out.println(
                    "SYSCatalogGroup SYSCAT.SYSCOLLECTIONS inconsistent" );
        }

        return ret;
    }

    private boolean waitCatalogSync( boolean printAndThrowAllException )
            throws ReliabilityException {
        int checkTimes = 30;
        int checkInterval = 1000; // 1s
        boolean ret = false;
        for ( int i = 0; i < checkTimes; ++i ) {
            if ( isCatalogSync( false ) ) {
                ret = true;
                break;
            }
            try {
                Thread.sleep( checkInterval );
            } catch ( InterruptedException e ) {
            }
        }
        if ( ret ) {
            return ret;
        } else {
            ret = isCatalogSync( true );
            return ret;
        }
    }

    private boolean dropTestCollection( boolean printAndThrowAllException,
            String coordUrl ) throws ReliabilityException {
        boolean ret = true;
        Sequoiadb db = null;
        try {
            db = new Sequoiadb( coordUrl, "", "" );
            db.getCollectionSpace( SdbTestBase.csName )
                    .dropCollection( "clForTestBusiness_reliability" );
        } catch ( BaseException e ) {
            ret = false;
            if ( printAndThrowAllException ) {
                System.out.println(
                        "Check business:failed to drop test collection:"
                                + e.getErrorCode() );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
        return ret;
    }

    private boolean createTestCollection( boolean printAndThrowAllException,
            String coordUrl ) throws ReliabilityException {
        Sequoiadb db = null;
        List< String > groupNames = null;
        int index = 0;
        boolean result = true;
        try {
            groupNames = getAllDataGroupName( coordUrl );
            db = new Sequoiadb( coordUrl, "", "" );
            CollectionSpace cs = db.getCollectionSpace( SdbTestBase.csName );
            for ( index = 0; index < groupNames.size(); index++ ) {
                if ( cs.isCollectionExist( "clForTestBusiness_reliability" ) ) {
                    cs.dropCollection( "clForTestBusiness_reliability" );
                }
                cs.createCollection( "clForTestBusiness_reliability",
                        ( BSONObject ) JSON.parse( "{ReplSize:0,Group:'"
                                + groupNames.get( index ) + "'}" ) );

                if ( index != groupNames.size() - 1 ) {
                    cs.dropCollection( "clForTestBusiness_reliability" );
                }
            }
        } catch ( BaseException e ) {
            result = false;
            if ( printAndThrowAllException ) {
                System.out.println(
                        "Check business:failed to create test collection(clForTestBusiness_reliability) on "
                                + groupNames.get( index ) + ":"
                                + e.getErrorCode() );
                throw new ReliabilityException( e );
            }
        } finally {
            if ( db != null ) {
                db.close();
            }
        }
        return result;
    }

    /**
     * 持续检测集群当前的状态（组内是否有主，各节点可连接，ServiceStatus,LSN一致），若在timeOutSecond时间内检测仍不通过，
     * 则会打印当前集群状态信息（也可能会抛出异常，如-104，网络错误等等），并返回false
     * Assert.assertEquals(checkBusiness(120), true, "Message");
     *
     * @param timeOutSecond
     *            建议>120（秒）
     * @return
     * @throws ReliabilityException
     */
    public boolean checkBusinessWithLSN( int timeOutSecond, String coordUrl )
            throws ReliabilityException {
        refresh( coordUrl );
        long timestamp = System.currentTimeMillis();
        boolean ret = true;
        boolean printAndThrowAllException = false;
        do {
            ret = true;
            printAndThrowAllException = false;
            if ( System.currentTimeMillis() - timestamp > timeOutSecond
                    * 1000 ) {
                printAndThrowAllException = true;
            }
            Sequoiadb db = null;
            try {
                db = new Sequoiadb( coordUrl, "", "" );
                DBCursor snapCur = db.getSnapshot( Sequoiadb.SDB_SNAP_HEALTH,
                        "", "", "" );
                while ( snapCur.hasNext() ) {
                    BasicBSONObject nodeInfo = ( BasicBSONObject ) snapCur
                            .getNext();
                    if ( nodeInfo.containsField( "ErrNodes" ) ) {
                        if ( printAndThrowAllException ) {
                            System.out.println(
                                    "ErrNodes: " + nodeInfo.get( "ErrNodes" ) );
                        }
                        ret = false;
                        break;
                    }
                }
                snapCur.close();
            } catch ( BaseException e ) {
                if ( printAndThrowAllException ) {
                    throw e;
                } else {
                    e.printStackTrace();
                    ret = false;
                }
            } finally {
                if ( db != null ) {
                    db.close();
                }
            }
            try {
                if ( ret ) {
                    ret = checkBusinessWithLSN( printAndThrowAllException, true,
                            coordUrl );
                }
            } catch ( ReliabilityException e ) {
                if ( printAndThrowAllException ) {
                    throw e;
                } else {
                    e.printStackTrace();
                    ret = false;
                }
            }

            if ( !ret ) {
                try {
                    Thread.sleep( 1000 );
                } catch ( InterruptedException e ) {
                    // ignore
                }
            }
        } while ( !ret && !printAndThrowAllException );
        return ret;
    }

    /**
     * 检测集群状态（组内是否有主，各节点可连接，ServiceStatus,组内LSN一致），失败则打印集群信息，并返回false
     *
     * @return
     * @throws ReliabilityException
     */
    // TODO:可被替代，屏蔽
    public boolean checkBusinessWithLSN( String coordUrl )
            throws ReliabilityException {
        return checkBusinessWithLSN( 120, coordUrl );
    }

    public boolean checkBusinessWithLSN() throws ReliabilityException {
        return checkBusinessWithLSN( 120, coordUrl );
    }

    public boolean checkBusinessWithLSN( int timeOutSecond )
            throws ReliabilityException {
        return checkBusinessWithLSN( timeOutSecond, coordUrl );
    }

    /**
     * 检测集群状态（组内是否有主，各节点可连接，ServiceStatus,组内LSN一致），
     * 失败则根据变量printAndThrowAllException决定是否打印集群信息，是否屏蔽检测过程中发生的异常，并返回false
     *
     * @param printAndThrowAllException
     * @return
     * @throws ReliabilityException
     */
    // TODO:可被替代，屏蔽
    private boolean checkBusinessWithLSN( boolean printAndThrowAllException,
            boolean ignoreIndeploy, String coordUrl )
            throws ReliabilityException {
        boolean ret = false;
        // 尝试创建一个ReplSize=3的测试集合（检测所有数据节点是否Alive）
        if ( createTestCollection( printAndThrowAllException, coordUrl ) ) {
            // 因为catalog的同步策略不受ReplSize影响，所以删除cl后要等待catalog同步，以免影响外部
            ret = dropTestCollection( printAndThrowAllException, coordUrl )
                    && waitCatalogSync( printAndThrowAllException );
        } else {
            return false;
        }
        if ( !ret )
            return ret;

        List< GroupCheckResult > results = checkGroup(
                printAndThrowAllException );

        for ( GroupCheckResult result : results ) {
            if ( !result.checkWithLSN( ignoreIndeploy ) ) {
                if ( printAndThrowAllException ) {
                    System.out.println( result.toString() );
                }
                refreshAllGroup();
                return false;
            }
        }
        return true;
    }

    /**
     * 持续检测集群当前的状态（组内是否有主，各节点可连接，ServiceStatus,LSN一致，节点所在磁盘剩余空间大于128M），
     * 若在timeOutSecond时间内检测仍不通过， 则会打印当前集群状态信息（也可能会抛出异常，如-104，网络错误等等），并返回false
     * Assert.assertEquals(checkBusiness(120), true, "Message");
     *
     * @param timeOutSecond
     *            建议>120（秒）
     * @return
     * @throws ReliabilityException
     */
    public boolean checkBusinessWithLSNAndDisk( int timeOutSecond,
            String coordUrl ) throws ReliabilityException {
        refresh( coordUrl );
        long timestamp = System.currentTimeMillis();
        while ( !checkBusinessWithLSNAndDisk( false, coordUrl ) ) {
            if ( System.currentTimeMillis() - timestamp > timeOutSecond
                    * 1000 ) {
                return checkBusinessWithLSNAndDisk( true, coordUrl );
            }
            try {
                Thread.sleep( 1000 );
            } catch ( InterruptedException e ) {
                // ignore
            }
        }
        return true;
    }

    /**
     * 检测集群状态（组内是否有主，各节点可连接，ServiceStatus,组内LSN一致,节点所在磁盘剩余空间大于128M），失败则打印集群信息，
     * 并返回false
     *
     * @return
     * @throws ReliabilityException
     */
    public boolean checkBusinessWithLSNAndDisk( String coordUrl )
            throws ReliabilityException {
        return checkBusinessWithLSNAndDisk( 120, coordUrl );
    }

    public boolean checkBusinessWithLSNAndDisk( int timeOutSecond )
            throws ReliabilityException {
        return checkBusinessWithLSNAndDisk( 120, coordUrl );
    }

    /**
     * 检测集群状态（组内有主，各节点可连接，ServiceStatus,组内LSN一致,节点所在磁盘剩余空间大于128M），
     * 失败则根据变量printAndThrowAllException决定是否打印集群信息，是否屏蔽检测过程中发生的异常，并返回false
     *
     * @param printAndThrowAllException
     * @return
     * @throws ReliabilityException
     */
    // TODO:可被替代，屏蔽
    private boolean checkBusinessWithLSNAndDisk(
            boolean printAndThrowAllException, String coordUrl )
            throws ReliabilityException {
        boolean ret = false;
        // 尝试创建一个ReplSize=3的测试集合（检测所有数据节点是否Alive）
        if ( createTestCollection( printAndThrowAllException, coordUrl ) ) {
            // 因为catalog的同步策略不受ReplSize影响，所以删除cl后要等待catalog同步，以免影响外部
            ret = dropTestCollection( printAndThrowAllException, coordUrl )
                    && waitCatalogSync( printAndThrowAllException );
        } else {
            return false;
        }

        if ( !ret )
            return ret;
        List< GroupCheckResult > results = checkGroup(
                printAndThrowAllException );
        for ( GroupCheckResult result : results ) {
            if ( !result.checkWithLSNAndDiskThreshold() ) {
                if ( printAndThrowAllException ) {
                    System.out.println( result.toString() );
                }
                refreshAllGroup();
                return false;
            }
        }
        return true;
    }

    /**
     * 获取数据组和编目组检查结果
     *
     * @param printAndThrowAllException
     * @return
     * @throws ReliabilityException
     * @Author jt
     */
    private List< GroupCheckResult > checkGroup(
            boolean printAndThrowAllException ) throws ReliabilityException {
        List< GroupCheckResult > results = new ArrayList<>();
        for ( Entry< String, GroupWrapper > entry : name2group.entrySet() ) {
            String name = entry.getKey();
            GroupWrapper group = entry.getValue();
            if ( name.equals( "SYSCoord" ) )
                continue;
            try {
                results.add( group.checkBusiness( printAndThrowAllException ) );
            } catch ( Exception e ) {
                e.printStackTrace();
                throw new ReliabilityException( e );
            }
        }
        return results;
    }

    /**
     * 检测端口SdbTestBase.reservedPortBegin-SdbTestBase.reservedPortEnd是否有残留占用
     * 检测目录SdbTestBase.reservedDir下是否有数据文件残留,检测安装目录下是否有用例节点配置文件残留
     * 以上检测针对集群所有主机，检测不通过返回false，并打印检测信息
     *
     * @return
     */
    public boolean checkResidu() {
        boolean checkRet = true;
        List< String > hosts = getAllHosts();
        for ( String host : hosts ) {
            try {
                Ssh remote = new Ssh( host, "root", SdbTestBase.rootPwd );
                remote.scpTo( "./script/checkCfgResidu.sh",
                        SdbTestBase.workDir );
                remote.exec( "chmod 777 " + SdbTestBase.workDir
                        + "/checkCfgResidu.sh" );
                remote.scpTo( "./script/checkPortOccupied.sh",
                        SdbTestBase.workDir );
                remote.exec( "chmod 777 " + SdbTestBase.workDir
                        + "/checkPortOccupied.sh" );
                remote.scpTo( "./script/checkDataResidu.sh",
                        SdbTestBase.workDir );
                remote.exec( "chmod 777 " + SdbTestBase.workDir
                        + "/checkDataResidu.sh" );

                try {
                    remote.exec( SdbTestBase.workDir + "/checkPortOccupied.sh "
                            + SdbTestBase.reservedPortBegin + " "
                            + SdbTestBase.reservedPortEnd );
                } catch ( ReliabilityException e ) {
                    System.out.println( String.format( "%s used port:%s", host,
                            remote.getStdout() ) );
                    if ( remote.getStderr().length() != 0 ) {
                        System.out.println( "StdErr:" + remote.getStderr() );
                    }
                    checkRet = false;
                }

                try {
                    remote.exec( SdbTestBase.workDir + "/checkCfgResidu.sh "
                            + SdbTestBase.reservedPortBegin + " "
                            + SdbTestBase.reservedPortEnd );
                } catch ( ReliabilityException e ) {
                    System.out.println( String.format( "%s residu config:%s",
                            host, remote.getStdout() ) );
                    if ( remote.getStderr().length() != 0 ) {
                        System.out.println( "StdErr:" + remote.getStderr() );
                    }
                    checkRet = false;
                }

                try {
                    remote.exec( SdbTestBase.workDir + "/checkDataResidu.sh "
                            + SdbTestBase.reservedDir );
                } catch ( ReliabilityException e ) {
                    System.out.println( String.format( "%s residu data:%s",
                            host, remote.getStdout() ) );
                    if ( remote.getStderr().length() != 0 ) {
                        System.out.println( "StdErr:" + remote.getStderr() );
                    }
                    checkRet = false;
                }
            } catch ( ReliabilityException e ) {
                e.printStackTrace();
                return false;
            } finally {

            }
        }
        return checkRet;
    }

    public void close() {
        if ( sdb != null ) {
            sdb.close();
        }
    }

    public static void main( String[] args ) {
        GroupMgr mgr;
        try {
            SdbTestBase.coordUrl = "192.168.28.107:11810";
            mgr = new GroupMgr();
            SdbTestBase.csName = "reliability_test";
            boolean ret = mgr.checkBusiness();
            if ( !ret ) {
                System.out.println( "failed" );
            }
        } catch ( ReliabilityException e ) {
            e.printStackTrace();
        }

    }
}
