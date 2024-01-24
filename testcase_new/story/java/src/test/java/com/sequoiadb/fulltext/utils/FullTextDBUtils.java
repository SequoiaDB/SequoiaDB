package com.sequoiadb.fulltext.utils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.types.BasicBSONList;

import com.sequoiadb.base.CollectionSpace;
import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;

/**
 * DB的公共类，涉及DB内部操作的方法放于此类
 */
public class FullTextDBUtils {

    /**
     * 获取全文索引对应的固定集合名
     * 
     * @param cl
     * @param indexName
     * @return String 返回固定集合名，可以通过CL的属性"ExtDataName"获取。如果不存在固定集合则抛出异常
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static String getCappedName( DBCollection cl, String indexName ) {
        // 获取索引信息
        BSONObject indexInfos = cl.getIndexInfo( indexName );

        if ( indexInfos != null && indexInfos.containsField( "ExtDataName" ) ) {
            return ( String ) indexInfos.get( "ExtDataName" );
        } else {
            throw new BaseException( -52, "no such index: " + indexName + " from cl: " + cl.getFullName() );
        }
    }

    /**
     * 获取原始集合下的所有固定集合对象，原始集合可以是普通表、分区表
     * 
     * @param cl
     * @param textIndexName
     * @return List<DBCollection> 返回所有固定集合对象
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< DBCollection > getCappedCLs( DBCollection cl,
            String textIndexName ) {
        Sequoiadb db = cl.getSequoiadb();
        String cappedName = getCappedName( cl, textIndexName );
        List< String > groupNames = getCLGroups( cl );
        // 获取每个数据组主节点下的固定集合对象
        List< DBCollection > cappedCLs = new ArrayList<>();
        for ( String groupName : groupNames ) {
            DBCollection cappedCL = db.getReplicaGroup( groupName ).getMaster()
                    .connect().getCollectionSpace( cappedName )
                    .getCollection( cappedName );
            cappedCLs.add( cappedCL );
        }
        return cappedCLs;
    }

    /**
     * 指定索引名获取原始集合下的全文索引名，原始集合是普通表
     * 
     * @param cl
     * @param indexName
     * @return String 返回全文索引名
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static String getESIndexName( DBCollection cl, String indexName ) {

        String cappedName = getCappedName( cl, indexName );
        List< String > groupNames = getCLGroups( cl );

        return FullTextUtils.getFulltextPrefix().toLowerCase()
                + cappedName.toLowerCase() + "_" + groupNames.get( 0 );
    }

    /**
     * 获取原始集合下的所有全文索引名，原始集合可以是普通表、分区表
     * 
     * @param cl
     * @param indexName
     * @return List<String> 返回所有全文索引名
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< String > getESIndexNames( DBCollection cl,
            String indexName ) {
        String cappedName = getCappedName( cl, indexName );

        // 获取原始集合下的全文索引名
        List< String > esIndexNames = new ArrayList<>();
        List< String > groupNames = getCLGroups( cl );

        for ( String groupName : groupNames ) {
            esIndexNames.add( FullTextUtils.getFulltextPrefix().toLowerCase()
                    + cappedName.toLowerCase() + "_" + groupName );
        }

        // 分区表包含多个全文索引
        return esIndexNames;
    }

    /**
     * 获取固定集合的最大LogicalID
     * 
     * @param cappedCL
     * @return int 返回最大_id值，如果固定集合不存在记录则返回-1
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public int getLastLid( DBCollection cappedCL ) {
        long lastLogicalID = -1;
        BSONObject sortObj = new BasicBSONObject();
        BSONObject selectObject = new BasicBSONObject();
        sortObj.put( "_id", -1 );
        selectObject.put( "_id", 1 );
        BSONObject dataObj = cappedCL.queryOne( null, selectObject, sortObj,
                null, 0 );
        if ( dataObj != null ) {
            lastLogicalID = ( long ) dataObj.get( "_id" );
        }
        return ( int ) lastLogicalID;
    }

    /**
     * 读取游标记录组成List返回
     * 
     * @param cursor
     * @return List<BSONObject> 返回记录
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< BSONObject > getReadList( DBCursor cursor ) {
        List< BSONObject > objs = new ArrayList< BSONObject >();
        while ( cursor.hasNext() ) {
            BSONObject obj = cursor.getNext();
            objs.add( obj );
        }
        return objs;
    }

    /**
     * 获取原始集合对应的数据组，原始集合可以是普通表、分区表
     * 
     * @param cl
     * @return List<String> 返回所有数据组
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< String > getCLGroups( DBCollection cl ) {
        List< String > groupNames = new ArrayList< String >();
        Sequoiadb db = cl.getSequoiadb();
        if ( CommLib.isStandAlone( db ) ) {
            return groupNames;
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", cl.getFullName() );
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                null, null );
        HashSet< String > groupNamesSet = new HashSet< String >();
        while ( cur.hasNext() ) {
            BasicBSONList bsonLists = ( BasicBSONList ) cur.getNext()
                    .get( "CataInfo" );
            for ( int i = 0; i < bsonLists.size(); i++ ) {
                BasicBSONObject obj = ( BasicBSONObject ) bsonLists.get( i );
                groupNamesSet.add( obj.getString( "GroupName" ) );
            }
        }
        groupNames.addAll( groupNamesSet );
        // groupNames数组元素排序,排序是为了cl的esIndexNames和cappedCLs能一一对应
        Collections.sort( groupNames, new Comparator< Object >() {
            @Override
            public int compare( Object o1, Object o2 ) {
                String str1 = ( String ) o1;
                String str2 = ( String ) o2;
                if ( str1.compareToIgnoreCase( str2 ) < 0 ) {
                    return -1;
                }
                return 1;
            }
        } );

        return groupNames;
    }

    /**
     * 获取主表下所有子表的表名
     * 
     * @param db
     * @param mainCLFullName
     * @return List<String> 返回所有子表名
     * @Author liuxiaoxuan
     * @Date 2018-11-15
     */
    public static List< String > getSubCLNames( Sequoiadb db,
            String mainCLFullName ) {
        List< String > subCLNames = new ArrayList<>();
        if ( CommLib.isStandAlone( db ) ) {
            return subCLNames;
        }

        BSONObject matcher = new BasicBSONObject();
        matcher.put( "Name", mainCLFullName );
        DBCursor cur = db.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG, matcher,
                null, null );
        while ( cur.hasNext() ) {
            BasicBSONList bsonLists = ( BasicBSONList ) cur.getNext()
                    .get( "CataInfo" );
            for ( int i = 0; i < bsonLists.size(); i++ ) {
                BasicBSONObject obj = ( BasicBSONObject ) bsonLists.get( i );
                subCLNames.add( obj.getString( "SubCLName" ) );
            }
        }

        return subCLNames;
    }

    /**
     * 删除全文索引，循环规避-147。该问题在bug#SEQUOIADBMAINSTAREM-3778跟踪，待问题解决后此方法可去除
     * 
     * @param cl
     * @param textIndexName
     * @return void
     * @Author liuxiaoxuan
     * @Date 2018-11-26
     */
    public static void dropFullTextIndex( DBCollection cl,
            String textIndexName ) {
        int timeout = 600;
        int doTimes = 0;
        // 删除全文索引，如果报错-147则重试 10min
        while ( doTimes < timeout ) {
            try {
                cl.dropIndex( textIndexName );
                // 删除索引成功，则退出
                break;
            } catch ( BaseException e ) {
                doTimes++;
                if ( -147 == e.getErrorCode() ) {
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e2 ) {
                        e2.printStackTrace();
                    }
                    continue;
                } else if ( -47 == e.getErrorCode() ) { // 索引已不存在，退出
                    // System.out.println( textIndexName + " is not exist" );
                    break;
                } else {
                    System.out.println( "drop " + textIndexName
                            + "failed, detail: " + e.getMessage() );
                    throw e;
                }

            }
        }

        // 如果最后在超时时间内删除成功，则打印信息；否则再次删除索引，操作失败后直接抛异常
        // if (doTimes < timeout) {
        // System.err.println(textIndexName + " drop success, drop times: " +
        // doTimes);
        // } else {
        // cl.dropIndex(textIndexName);
        // }

        if ( doTimes >= timeout ) {
            cl.dropIndex( textIndexName );
        }
    }

    /**
     * 删除原始集合空间，循环规避-147。该问题在bug#SEQUOIADBMAINSTAREM-3778跟踪，待问题解决后此方法可去除
     * 
     * @param db
     * @param csName
     * @return void
     * @Author liuxiaoxuan
     * @Date 2018-11-26
     */
    public static void dropCollectionSpace( Sequoiadb db, String csName ) {
        int timeout = 600;
        int doTimes = 0;
        // 删除集合空间，如果报错-147则重试 10min
        while ( doTimes < timeout ) {
            try {
                db.dropCollectionSpace( csName );
                // 删除cs成功，则退出
                break;
            } catch ( BaseException e ) {
                doTimes++;
                if ( -147 == e.getErrorCode() ) {
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e2 ) {
                        e2.printStackTrace();
                    }
                    continue;
                } else if ( -34 == e.getErrorCode() ) { // cs已不存在
                    // System.out.println( csName + " is not exist" );
                    break;
                } else {
                    System.out.println( "drop " + csName + "failed, detail: "
                            + e.getMessage() );
                    throw e;
                }

            }
        }

        // 如果最后在超时时间内删除成功，则打印信息；否则再次删除cs，操作失败后直接抛异常
        // if (doTimes < timeout) {
        // System.err.println(csName + " drop success, drop times: " + doTimes);
        // } else {
        // db.dropCollectionSpace(csName);
        // }

        if ( doTimes >= timeout ) {
            db.dropCollectionSpace( csName );
        }
    }

    /**
     * 删除原始集合，循环规避-147。该问题在bug#SEQUOIADBMAINSTAREM-3778跟踪，待问题解决后此方法可去除
     * 
     * @param cs
     * @param clName
     * @return void
     * @Author liuxiaoxuan
     * @Date 2018-11-26
     */
    public static void dropCollection( CollectionSpace cs, String clName ) {
        int timeout = 600;
        int doTimes = 0;
        // 删除集合，如果报错-147则重试 10min
        while ( doTimes < timeout ) {
            try {
                cs.dropCollection( clName );
                // 删除cl成功，则退出
                break;
            } catch ( BaseException e ) {
                doTimes++;
                if ( -147 == e.getErrorCode() ) {
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e2 ) {
                        e2.printStackTrace();
                    }
                    continue;
                } else if ( -23 == e.getErrorCode() ) { // cl已不存在
                    // System.out.println(clName + " is not exist");
                    break;
                } else {
                    System.out.println( "drop " + clName + "failed, detail: "
                            + e.getMessage() );
                    throw e;
                }

            }
        }

        // 如果最后在超时时间内删除成功，则打印信息；否则再次删除cs，操作失败后直接抛异常
        // if (doTimes < timeout) {
        // System.err.println(clName + " drop success, drop times: " + doTimes);
        // } else {
        // cs.dropCollection(clName);
        // }

        if ( doTimes >= timeout ) {
            cs.dropCollection( clName );
        }
    }

    /**
     * 判断是否主表
     * 
     * @param sdb
     * @param clFullName
     * @return boolean 是主表则返回true，否则返回false
     * @Author yinzhen
     * @Date 2018-12-21
     */
    public static boolean isMainCL( Sequoiadb sdb, String clFullName ) {
        DBCursor cursor = sdb.getSnapshot( Sequoiadb.SDB_SNAP_CATALOG,
                "{'Name':'" + clFullName + "'}", null, null );
        BasicBSONObject clInfo = ( BasicBSONObject ) cursor.getNext();
        if ( clInfo.containsField( "IsMainCL" ) ) {
            return clInfo.getBoolean( "IsMainCL" );
        }
        return false;
    }

    /**
     * 指定记录数插入记录，如: {id: 0, a: "clname", b: "8 byte str0", c: "32 byte str...",
     * d: "64 byte str..."}
     *
     * @param cl
     * @param insertNum
     * @return void
     * @Author luweikang
     * @Date 2019-05-08
     */
    public static void insertData( DBCollection cl, int insertNum ) {
        String clName = cl.getName();
        List< BSONObject > insertObjs = new ArrayList< BSONObject >();
        final int onceRecordNum = 1000;
        int insertTimes = insertNum / onceRecordNum;
        int residueNum = insertNum % onceRecordNum;
        String strB = StringUtils.getRandomString( 8 );
        String strC = StringUtils.getRandomString( 32 );
        String strD = StringUtils.getRandomString( 64 );
        for ( int i = 0; i < insertTimes; i++ ) {
            for ( int j = 0; j < onceRecordNum; j++ ) {
                int recordNum = i * onceRecordNum + j;
                BSONObject data = new BasicBSONObject();
                data.put( "recordId", recordNum );
                data.put( "a", clName );
                data.put( "b", strB + recordNum );
                data.put( "c", strC );
                data.put( "d", strD );

                insertObjs.add( data );
            }
            cl.insert( insertObjs, 0 );
            insertObjs.clear();
        }

        int recordNum = insertTimes * onceRecordNum;
        if ( residueNum != 0 ) {
            for ( int i = 0; i < residueNum; i++ ) {
                BSONObject data = new BasicBSONObject();
                data.put( "recordId", recordNum + i );
                data.put( "a", clName );
                data.put( "b", strB + recordNum + i );
                data.put( "c", strC );
                data.put( "d", strD );

                insertObjs.add( data );
            }
            cl.insert( insertObjs );
        }
    }

    /**
     * 检查集合空间是否删除成功,每秒检查一次,检测时间最长为5分钟
     * 
     * @param db
     * @param csName
     * @return boolean, 删除成功返回true,否则抛出删除失败异常
     * @throws Exception
     * @throws InterruptedException
     * @Author luweikang
     * @Date 2019-05-09
     */
    public boolean isCSDropSuccess( Sequoiadb db, String csName )
            throws Exception {
        if ( isExistCS( db, csName, false ) ) {
            throw new Exception( "cs '" + csName + "' deletion failed" );
        } else {
            return true;
        }
    }

    /**
     * 检查所有集合空间是否删除成功,每秒检查一次,检测时间最长为5分钟
     * 
     * @param db
     * @param csNames
     * @return boolean, 所有cs删除成功返回true,只要有一个失败则抛出删除失败异常
     * @throws Exception
     * @throws InterruptedException
     * @Author luweikang
     * @Date 2019-05-09
     */
    public boolean isAllCSDropSuccess( Sequoiadb db, List< String > csNames )
            throws Exception {
        for ( String csName : csNames ) {
            if ( !isCSDropSuccess( db, csName ) ) {
                return false;
            }
        }
        return true;
    }

    /**
     * 检查集合空间是存在,每秒检查一次,检测时间最长为5分钟
     * 
     * @param db
     * @param csName
     * @param expExist
     * @return boolean, cs存在返回true,否则返回false
     * @throws Exception
     * @throws InterruptedException
     * @Author luweikang
     * @Date 2019-05-09
     */
    private boolean isExistCS( Sequoiadb db, String csName, boolean expExist )
            throws Exception {
        if ( csName.isEmpty() ) {
            return false;
        }
        List< String > rgNames = CommLib.getDataGroupNames( db );
        boolean csExist = false;
        for ( String rgName : rgNames ) {
            csExist = isExistCS( db, csName, rgName, expExist );
            if ( expExist != csExist ) {
                break;
            }
        }
        return csExist;
    }

    /**
     * 检查集合空间是存在,每秒检查一次,检测时间最长为5分钟
     * 
     * @param db
     * @param csName
     * @param rgName
     * @param expExist
     * @return boolean, cs存在返回true,否则返回false
     * @throws Exception
     * @throws InterruptedException
     * @Author luweikang
     * @Date 2019-05-09
     */
    private boolean isExistCS( Sequoiadb db, String csName, String rgName,
            boolean expExist ) throws Exception {
        if ( csName.isEmpty() ) {
            return false;
        }
        boolean csExist = false;
        List< String > nodeList = CommLib.getNodeAddress( db, rgName );
        for ( String nodeAddress : nodeList ) {
            try ( Sequoiadb nodeConn = new Sequoiadb( nodeAddress, "", "" )) {
                for ( int i = 0; i < 300; i++ ) {
                    csExist = nodeConn.isCollectionSpaceExist( csName );
                    if ( expExist == csExist ) {
                        break;
                    }
                    try {
                        Thread.sleep( 1000 );
                    } catch ( InterruptedException e ) {
                        e.printStackTrace();
                    }
                }
            }
        }
        return csExist;
    }

}
