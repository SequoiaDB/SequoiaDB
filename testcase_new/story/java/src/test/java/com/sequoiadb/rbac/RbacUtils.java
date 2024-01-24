package com.sequoiadb.rbac;

import java.util.*;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.recyclebin.RecycleBinUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;

import com.sequoiadb.base.*;
import org.bson.types.BasicBSONList;
import org.bson.types.ObjectId;
import org.bson.util.JSON;
import org.testng.Assert;

public class RbacUtils extends SdbTestBase {

    public static void findActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean executeNotSupport ) {
        // 插入数据和lob用于find
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        String indexName = "index_" + clName;
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );

        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        ObjectId oid = lob.getID();

        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行权限支持的操作
        DBCursor cursor = null;
        cursor = dbcl.query();
        cursor.getNext();
        cursor.close();

        List< BSONObject > aggregateObj = new ArrayList< BSONObject >();
        BSONObject limitObj = ( BSONObject ) JSON.parse( "{$limit:5}" );
        aggregateObj.add( limitObj );
        dbcl.aggregate( aggregateObj );

        dbcl.queryOne();

        dbcl.getCount();

        cursor = dbcl.listLobs();
        cursor.getNext();
        cursor.close();

        dbcl.getIndexInfo( indexName );

        cursor = dbcl.getIndexes();
        cursor.getNext();
        cursor.close();

        sdb.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        dbcl.getIndexStat( indexName );

        DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_READ );
        byte[] rbuff = new byte[ ( int ) rLob.getSize() ];
        rLob.read( rbuff );
        rLob.close();

        rLob = dbcl.openLob( oid, DBLob.SDB_LOB_SHAREREAD );
        rLob.close();

        rLob = dbcl.openLob( oid );
        rLob.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.createLob();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                cursor = dbcl.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 20000 ) ),
                        0, -1, 0, false );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                cursor = dbcl.queryAndRemove( new BasicBSONObject( "a", 1 ),
                        null, null, null, -1, -1, 0 );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.dropIndex( indexName );
        rootCL.truncate();
    }

    public static void insertActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean executeNotSupport ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 执行权限支持的操作
        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = dbcl.createLob();
        lob.write( wlobBuff );
        lob.close();
        ObjectId oid = lob.getID();

        dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                String indexName = "index_" + clName;
                dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.updateRecords( new BasicBSONObject( "a", 2 ),
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "a", 3 ) ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.openLob( oid, DBLob.SDB_LOB_WRITE );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.truncate();
    }

    public static void updateActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean executeNotSupport ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );

        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        ObjectId oid = lob.getID();

        // 执行权限支持的操作
        dbcl.updateRecords( new BasicBSONObject( "a", 1 ),
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 2 ) ) );
        dbcl.updateRecords( new BasicBSONObject( "a", 2 ),
                new BasicBSONObject( "$set", new BasicBSONObject( "a", 1 ) ) );

        // 执行lob偏移写操作
        DBLob rLob = dbcl.openLob( oid, DBLob.SDB_LOB_WRITE );
        rLob.lockAndSeek( writeLobSize, writeLobSize );
        rLob.write( wlobBuff );
        rLob.getID();
        rLob.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            DBCursor cursor = null;
            try {
                cursor = dbcl.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 20000 ) ),
                        0, -1, 0, false );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                cursor = dbcl.queryAndRemove( new BasicBSONObject( "a", 1 ),
                        null, null, null, -1, -1, 0 );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.createLob();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.truncate();
    }

    public static void removeActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, DBCollection dbcl, boolean executeNotSupport ) {
        // 插入数据和lob用于find
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        ObjectId oid = lob.getID();

        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );

        // 执行权限支持的操作
        dbcl.removeLob( oid );
        dbcl.deleteRecords( new BasicBSONObject( "a", 1 ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcl.truncate();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            DBCursor cursor = null;
            try {
                cursor = dbcl.queryAndUpdate( null, null, null, null,
                        new BasicBSONObject( "$set",
                                new BasicBSONObject( "b", 20000 ) ),
                        0, -1, 0, false );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                cursor = dbcl.queryAndRemove( new BasicBSONObject( "a", 1 ),
                        null, null, null, -1, -1, 0 );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.truncate();
    }

    public static void getDetailActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean executeNotSupport ) {
        // 插入数据和lob用于find
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        Random random = new Random();
        int writeLobSize = random.nextInt( 1024 * 1024 );
        byte[] wlobBuff = getRandomBytes( writeLobSize );
        DBLob lob = rootCL.createLob();
        lob.write( wlobBuff );
        lob.close();
        lob.getID();

        rootCL.insertRecord( new BasicBSONObject( "a", 1 ) );
        String indexName = "index_" + clName;
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );

        // 执行权限支持的操作
        dbcl.getCount();

        dbcl.getIndexInfo( indexName );

        dbcl.getIndexes();

        sdb.analyze(
                new BasicBSONObject( "Collection", csName + "." + clName ) );
        dbcl.getIndexStat( indexName );

        DBCursor cursor = dbcl.listLobs();
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcl.queryOne();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.createLob();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCL.dropIndex( indexName );
        rootCL.truncate();
    }

    public static void createIndexActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean executeNotSupport ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 执行权限支持的操作
        String indexName = "index_" + clName;
        dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        rootCL.dropIndex( indexName );
        long taskId = dbcl.createIndexAsync( indexName,
                new BasicBSONObject( "a", 1 ), null, null );
        long[] taskIds = new long[ 1 ];
        taskIds[ 0 ] = taskId;
        sdb.waitTasks( taskIds );
        rootCL.dropIndex( indexName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            // 重建一个集合不包含_id索引
            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.alterCollection( new BasicBSONObject( "ReplSize", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void dropIndexActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean executeNotSupport ) {
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );

        // 执行权限支持的操作
        String indexName = "index_" + clName;
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        dbcl.dropIndex( indexName );

        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        long taskId = dbcl.dropIndexAsync( indexName );
        long[] taskIds = new long[ 1 ];
        taskIds[ 0 ] = taskId;
        sdb.waitTasks( taskIds );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcl.dropIdIndex();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                BasicBSONObject options = new BasicBSONObject();
                options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
                options.put( "ShardingType", "hash" );
                dbcl.enableSharding( options );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void alterCLActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, DBCollection dbcl,
            boolean executeNotSupport ) {
        // 执行权限支持的操作
        String indexName = "index_" + clName;

        dbcl.alterCollection( new BasicBSONObject( "ReplSize", -1 ) );

        BasicBSONObject options = new BasicBSONObject();
        String fieldName = "field_" + clName;
        options.put( "Field", fieldName );
        options.put( "AcquireSize", 1 );
        dbcl.createAutoIncrement( options );
        dbcl.dropAutoIncrement( fieldName );
        dbcl.dropIdIndex();
        dbcl.createIdIndex( null );
        dbcl.disableCompression();
        options.clear();
        options.put( "ShardingKey", new BasicBSONObject( "a", 1 ) );
        options.put( "ShardingType", "hash" );
        dbcl.enableSharding( options );
        dbcl.disableSharding();
        dbcl.setAttributes( new BasicBSONObject( "ReplSize", 1 ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcl.createIndex( indexName, new BasicBSONObject( "a", 1 ),
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void alterCSActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        // 执行权限支持的操作
        String domainName1 = "domain_1_" + csName;
        String domainName2 = "domain_2_" + csName;

        if ( sdb.isDomainExist( domainName1 ) ) {
            sdb.dropDomain( domainName1 );
        }

        if ( sdb.isDomainExist( domainName2 ) ) {
            sdb.dropDomain( domainName2 );
        }

        sdb.createDomain( domainName1,
                new BasicBSONObject( "Groups", groupNames ) );
        sdb.createDomain( domainName2,
                new BasicBSONObject( "Groups", groupNames ) );

        dbcs.alterCollectionSpace(
                new BasicBSONObject( "Domain", domainName1 ) );
        dbcs.alterCollectionSpace(
                new BasicBSONObject( "Domain", domainName2 ) );
        dbcs.removeDomain();
        dbcs.setDomain( new BasicBSONObject( "Domain", domainName1 ) );
        dbcs.removeDomain();

        DBCollection dbcl = dbcs.getCollection( clName );
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcs.dropCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropDomain( domainName1 );
        sdb.dropDomain( domainName2 );
    }

    public static void createCLActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean executeNotSupport ) {
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );

        // 执行权限支持的操作
        String testCLName = clName + "test_create_cl";
        dbcs.createCollection( testCLName );

        DBCollection dbcl = dbcs.getCollection( testCLName );
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcs.dropCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcl.insertRecord( new BasicBSONObject( "a", 1 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.dropCollection( testCLName );
    }

    public static void dropCLActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, CollectionSpace dbcs, boolean executeNotSupport ) {
        String testCLName = clName + "test_create_cl";
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( testCLName );

        // 执行权限支持的操作
        dbcs.dropCollection( testCLName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcs.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void renameCLActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean executeNotSupport ) {
        String testCLName = clName + "test_create_cl";
        String testCLNameNew = clName + "test_create_cl_new";
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.createCollection( testCLName );

        // 执行权限支持的操作
        dbcs.renameCollection( testCLName, testCLNameNew );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcs.createCollection( testCLName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcs.createCollection( testCLNameNew );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.dropCollection( testCLNameNew );
    }

    public static void findActionSupportCommand( Sequoiadb sdb, String csName,
            String clName, CollectionSpace dbcs, boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        String domainName = "domain_" + csName;
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }

        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );

        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.setDomain( new BasicBSONObject( "Domain", domainName ) );

        // 执行权限支持的操作
        dbcs.getDomainName();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcs.removeDomain();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcs.dropCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.removeDomain();
        sdb.dropDomain( domainName );
    }

    public static void getDetailActionSupportCommand( Sequoiadb sdb,
            String csName, String clName, CollectionSpace dbcs,
            boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        String domainName = "domain_" + csName;
        if ( sdb.isDomainExist( domainName ) ) {
            sdb.dropDomain( domainName );
        }

        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );

        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        rootCS.setDomain( new BasicBSONObject( "Domain", domainName ) );

        // 执行权限支持的操作
        // dbcs.getDomainName();
        // String domain = dbcs.getDomainName();
        // List< String > test = dbcs.getCollectionNames();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                dbcs.removeDomain();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                dbcs.dropCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootCS.removeDomain();
        sdb.dropDomain( domainName );
    }

    public static void alterBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 获取回收站原始配置
        BSONObject recycleBinAttr = sdb.getRecycleBin().getDetail();

        // 修改回收站相关配置
        userSdb.getRecycleBin().disable();
        userSdb.getRecycleBin().enable();
        BasicBSONObject recycleBinAttrNew = new BasicBSONObject( "MaxItemNum",
                500 );
        userSdb.getRecycleBin().setAttributes( recycleBinAttrNew );
        userSdb.getRecycleBin().alter( recycleBinAttr );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getRecycleBin().dropAll( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void countBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 获取回收站项目数量
        userSdb.getRecycleBin().getCount( null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                DBCursor cursor = userSdb.getRecycleBin().list( null, null,
                        null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void dropAllBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 删除回收站全部项目
        userSdb.getRecycleBin().dropAll( new BasicBSONObject( "Async", true ) );
        userSdb.getRecycleBin().dropAll( null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                DBCursor cursor = userSdb.getRecycleBin().snapshot( null, null,
                        null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void dropItemBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行truncate
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        dbcl.truncate();
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, csName + "." + clName, "Truncate" )
                .get( 0 );
        // 删除回收站项目
        userSdb.getRecycleBin().dropItem( recycleName, null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getRecycleBin().dropAll( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getRecycleBin().returnItem( recycleName, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void getDetailBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 获取回收站配置
        BSONObject recycleBinAttr = userSdb.getRecycleBin().getDetail();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getRecycleBin().alter( recycleBinAttr );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void listBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        DBCursor cursor = userSdb.getRecycleBin().list( null, null, null );
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                cursor = userSdb.getRecycleBin().snapshot( null, null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                // cursor = userSdb.getList( Sequoiadb.SDB_LIST_RECYCLEBIN,
                // null,
                // null, null );
                cursor = userSdb.getList( Sequoiadb.SDB_LIST_COLLECTIONSPACES,
                        null, null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getRecycleBin().getCount( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void snapshotBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        DBCursor cursor = userSdb.getRecycleBin().snapshot( null, null, null );
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                cursor = userSdb.getRecycleBin().list( null, null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                // cursor = userSdb.getSnapshot( Sequoiadb.SDB_SNAP_RECYCLEBIN,
                // new BasicBSONObject(), null, null );
                cursor = userSdb.getSnapshot(
                        Sequoiadb.SDB_LIST_COLLECTIONSPACES,
                        new BasicBSONObject(), null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getRecycleBin().getCount( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void returnItemBinActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        CollectionSpace rootCS = sdb.getCollectionSpace( csName );
        DBCollection dbcl = rootCS.getCollection( clName );
        // 执行truncate
        dbcl.truncate();
        // 删除集合
        rootCS.dropCollection( clName );

        // 恢复回收站项目
        String recycleName = RecycleBinUtils
                .getRecycleName( sdb, csName + "." + clName, "Truncate" )
                .get( 0 );
        userSdb.getRecycleBin().returnItem( recycleName, null );

        // 重命名恢复回收站项目
        recycleName = RecycleBinUtils
                .getRecycleName( sdb, csName + "." + clName, "Drop" ).get( 0 );
        String clNameNew = clName + "new";
        userSdb.getRecycleBin().returnItemToName( recycleName,
                csName + "." + clNameNew, null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.dropCollectionSpace( csName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        rootCS.dropCollection( clNameNew );
    }

    public static void backupActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String backupName = "backup_" + csName;

        // 执行备份
        try {
            sdb.removeBackup( new BasicBSONObject( "Name", backupName ) );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_BAR_BACKUP_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
        userSdb.backup( new BasicBSONObject( "Name", backupName ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.removeBackup(
                        new BasicBSONObject( "Name", backupName ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.removeBackup( new BasicBSONObject( "Name", backupName ) );
    }

    public static void removeBackupActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String backupName = "backup_" + csName;

        // 执行备份
        sdb.backup( new BasicBSONObject( "Name", backupName ) );
        userSdb.removeBackup( new BasicBSONObject( "Name", backupName ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.backup( new BasicBSONObject( "Name", backupName ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void createCSActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // _root角色用户删除集合空间
        sdb.dropCollectionSpace( csName );

        userSdb.createCollectionSpace( csName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.dropCollectionSpace( csName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.getCollectionSpace( csName ).createCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.getCollectionSpace( csName ).createCollection( clName );
    }

    public static void dropCSActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        userSdb.dropCollectionSpace( csName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.createCollectionSpace( csName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.createCollectionSpace( csName ).createCollection( clName );
    }

    public static void cancelTaskActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 获取集合
        DBCollection dbcl = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        String indexName = "index_" + clName;
        long taskID = dbcl.createIndexAsync( indexName,
                new BasicBSONObject( "a", 1 ), null, null );

        // 执行取消任务
        try {
            userSdb.cancelTask( taskID, true );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_TASK_ALREADY_FINISHED
                    .getErrorCode() ) {
                throw e;
            }
        }

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getCollectionSpace( csName ).getCollection( clName )
                        .dropIndex( indexName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        try {
            dbcl.dropIndex( indexName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_IXM_NOTEXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    public static void createRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName = "role_" + csName;
        RbacUtils.dropRole( sdb, roleName );
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['createRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        userSdb.createRole( role );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.dropRole( roleName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName );
    }

    public static void dropRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName = "role_" + csName;
        RbacUtils.dropRole( sdb, roleName );
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        userSdb.dropRole( roleName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.createRole( role );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void listRolesActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName = "role_" + csName;
        RbacUtils.dropRole( sdb, roleName );
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        DBCursor cursor = userSdb.listRoles( null );
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getRole( roleName, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName );
    }

    public static void updateRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName = "role_" + csName;
        RbacUtils.dropRole( sdb, roleName );
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 更新角色信息
        String updateRole = "{Privileges:[{Resource:{ Cluster:true}, Actions: ['createRole'] }]}";
        BSONObject updateRoleObj = ( BSONObject ) JSON.parse( updateRole );
        userSdb.updateRole( roleName, updateRoleObj );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.grantPrivilegesToRole( roleName, updateRoleObj );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName );
    }

    public static void grantPrivilegesToRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName = "role_" + csName;
        RbacUtils.dropRole( sdb, roleName );
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 更新角色信息
        String updateRole = "[{Resource:{ Cluster:true}, Actions: ['createRole'] }]";
        BSONObject updateRoleObj = ( BSONObject ) JSON.parse( updateRole );
        userSdb.grantPrivilegesToRole( roleName, updateRoleObj );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.updateRole( roleName, updateRoleObj );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName );
    }

    public static void revokePrivilegesFromRoleActionSupportCommand(
            Sequoiadb sdb, Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName = "role_" + csName;
        RbacUtils.dropRole( sdb, roleName );
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole','createRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        // 更新角色信息
        String updateRole = "[{Resource:{ Cluster:true}, Actions: ['createRole'] }]";
        BSONObject updateRoleObj = ( BSONObject ) JSON.parse( updateRole );
        userSdb.revokePrivilegesFromRole( roleName, updateRoleObj );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.grantPrivilegesToRole( roleName, updateRoleObj );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName );
    }

    public static void grantRolesToRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName1 = "role_1_" + csName;
        RbacUtils.dropRole( sdb, roleName1 );
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['createRole'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        String roleName2 = "role_2_" + csName;
        RbacUtils.dropRole( sdb, roleName2 );
        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }] }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 更新角色信息
        BasicBSONList roleList = new BasicBSONList();
        roleList.add( roleName2 );
        userSdb.grantRolesToRole( roleName1, roleList );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.revokeRolesFromRole( roleName1, roleList );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName1 );
        sdb.dropRole( roleName2 );
    }

    public static void revokeRolesFromRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 创建角色
        String roleName1 = "role_1_" + csName;
        RbacUtils.dropRole( sdb, roleName1 );
        String roleStr1 = "{Role:'" + roleName1
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['createRole'] }] }";
        BSONObject role1 = ( BSONObject ) JSON.parse( roleStr1 );
        sdb.createRole( role1 );

        String roleName2 = "role_2_" + csName;
        RbacUtils.dropRole( sdb, roleName2 );
        String roleStr2 = "{Role:'" + roleName2
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }],{Roles:['"
                + roleName1 + "']} }";
        BSONObject role2 = ( BSONObject ) JSON.parse( roleStr2 );
        sdb.createRole( role2 );

        // 更新角色信息
        BasicBSONList roleList = new BasicBSONList();
        roleList.add( roleName1 );
        userSdb.revokeRolesFromRole( roleName2, roleList );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.grantRolesToRole( roleName2, roleList );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.dropRole( roleName1 );
        sdb.dropRole( roleName2 );
    }

    public static void createUsrActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        String user = "user_" + csName;
        String password = "password_" + csName;
        userSdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_clusterMonitor']}" ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.removeUser( user, password );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.removeUser( user, password );
    }

    public static void dropUsrActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        String user = "user_" + csName;
        String password = "password_" + csName;
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_clusterMonitor']}" ) );
        userSdb.removeUser( user, password );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.createUser( user, password, ( BSONObject ) JSON
                        .parse( "{Roles:['_clusterMonitor']}" ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void grantRolesToUserActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        String user = "user_" + csName;
        String password = "password_" + csName;
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_clusterMonitor']}" ) );
        BasicBSONList roleList = new BasicBSONList();
        roleList.add( "_backup" );
        userSdb.grantRolesToUser( user, roleList );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.revokeRolesFromUser( user, roleList );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.removeUser( user, password );
    }

    public static void revokeRolesFromUserActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        String user = "user_" + csName;
        String password = "password_" + csName;
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_clusterMonitor']}" ) );
        BasicBSONList roleList = new BasicBSONList();
        roleList.add( "_clusterMonitor" );
        userSdb.revokeRolesFromUser( user, roleList );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.grantRolesToUser( user, roleList );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.removeUser( user, password );
    }

    public static void createDomainActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        // 执行权限支持的操作
        String domainName = "domain_" + csName;
        userSdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getDomain( domainName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                DBCursor cursor = userSdb.listDomains( null, null, null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.dropDomain( domainName );
    }

    public static void dropDomainActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        // 执行权限支持的操作
        String domainName = "domain_" + csName;
        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );
        userSdb.dropDomain( domainName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getDomain( domainName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                userSdb.createDomain( domainName,
                        new BasicBSONObject( "Groups", groupNames ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void createProcedureActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String procedureName = "procedure_" + csName;
        try {
            sdb.rmProcedure( procedureName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_FMP_FUNC_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
        String code = "function " + procedureName + "(x, y){return x/y;}";
        userSdb.crtJSProcedure( code );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.rmProcedure( procedureName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.rmProcedure( procedureName );
    }

    public static void removeProcedureActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String procedureName = "procedure_" + csName;
        String code = "function " + procedureName + "(x, y){return x/y;}";
        sdb.crtJSProcedure( code );
        userSdb.rmProcedure( procedureName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.crtJSProcedure( procedureName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void createSequenceActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String seqName = "seq_" + csName;
        userSdb.createSequence( seqName,
                ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.dropSequence( seqName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.dropSequence( seqName );
    }

    public static void dropSequenceActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String seqName = "seq_" + csName;
        sdb.createSequence( seqName,
                ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );
        userSdb.dropSequence( seqName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.createSequence( seqName,
                        ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void evalActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String procedureName = csName + "_procedure";
        try {
            sdb.rmProcedure( procedureName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_FMP_FUNC_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
        sdb.crtJSProcedure(
                "function " + procedureName + "(x,y){return x+y;}" );
        // 执行支持的操作
        userSdb.evalJS( procedureName + "(1,2)" );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.rmProcedure( procedureName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.rmProcedure( procedureName );
    }

    public static void flushConfigureActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        userSdb.flushConfigure( new BasicBSONObject( "Role", "data" ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                DBCursor cursor = userSdb.listRoles( null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void invalidateCacheActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        userSdb.invalidateCache( null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.invalidateUserCache( null, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                DBCursor cursor = userSdb.getList( Sequoiadb.SDB_LIST_USERS,
                        null, null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void invalidateUserCacheActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        // 执行支持的操作
        userSdb.invalidateUserCache( null, null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.invalidateCache( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                DBCursor cursor = userSdb.listRoles( null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void renameCSActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String csNameNew = "new_" + csName;
        if ( sdb.isCollectionSpaceExist( csNameNew ) ) {
            sdb.dropCollectionSpace( csNameNew );
        }
        userSdb.renameCollectionSpace( csName, csNameNew );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getCollectionSpace( csNameNew ).getCollection( clName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        userSdb.renameCollectionSpace( csNameNew, csName );
    }

    public static void resetSnapshotActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        userSdb.resetSnapshot( null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                DBCursor cursor = userSdb.getSnapshot(
                        Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                        new BasicBSONObject(), null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void syncActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        userSdb.sync( null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.resetSnapshot( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void waitTasksActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String indexName = "index_" + clName;
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        long taskID = rootCL.createIndexAsync( indexName,
                new BasicBSONObject( "a", 1 ), null, null );
        long[] taskIDs = new long[ 1 ];
        taskIDs[ 0 ] = taskID;
        userSdb.waitTasks( taskIDs );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.resetSnapshot( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        rootCL.dropIndex( indexName );
    }

    public static void getRoleActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String roleName = "role_" + csName;
        String roleStr = "{Role:'" + roleName
                + "',Privileges:[{Resource:{ Cluster:true}, Actions: ['dropRole'] }] }";
        BSONObject role = ( BSONObject ) JSON.parse( roleStr );
        sdb.createRole( role );

        userSdb.getRole( roleName, null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                DBCursor cursor = userSdb.listRoles( null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.dropRole( roleName );
    }

    public static void getUserActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String user = "user_" + csName;
        String password = "password_" + csName;
        sdb.createUser( user, password,
                ( BSONObject ) JSON.parse( "{Roles:['_clusterMonitor']}" ) );
        userSdb.getUser( user, null );
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                DBCursor cursor = userSdb.getList( Sequoiadb.SDB_LIST_USERS,
                        null, null, null );
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.removeUser( user, password );
    }

    public static void getDomainActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String domainName = "domain_" + csName;
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );
        Domain domain = userSdb.getDomain( domainName );
        DBCursor cursor = userSdb.listDomains( null, null, null, null );
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                cursor = domain.listCSInDomain();
                cursor.getNext();
                cursor.close();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.dropDomain( domainName );
    }

    public static void getSequenceActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String seqName = "seq_" + csName;
        sdb.createSequence( seqName,
                ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );
        DBSequence seq = userSdb.getSequence( seqName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                seq.fetch( 1 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.dropSequence( seqName );
    }

    public static void getTaskActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        String indexName = "index_" + clName;
        DBCollection rootCL = sdb.getCollectionSpace( csName )
                .getCollection( clName );
        rootCL.createIndex( indexName, new BasicBSONObject( "a", 1 ), null );
        DBCursor cursor = userSdb.listTasks( null, null, null, null );
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.backup( null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        rootCL.dropIndex( indexName );
    }

    public static void fetchSequenceActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, DBSequence seq, String seqName,
            boolean executeNotSupport ) {
        seq.fetch( 2 );
        seq.getNextValue();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                seq.getCurrentValue();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.restart( 1 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.setAttributes(
                        new BasicBSONObject( "CurrentValue", 1000 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.setCurrentValue( 10 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                String seqNameNew = "new_" + seqName;
                userSdb.renameSequence( seqName, seqNameNew );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void getSequenceCurrentValueActionSupportCommand(
            Sequoiadb sdb, Sequoiadb userSdb, DBSequence seq, String seqName,
            boolean executeNotSupport ) {
        seq.getCurrentValue();
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                seq.fetch( 2 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.getNextValue();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.restart( 1 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.setAttributes(
                        new BasicBSONObject( "CurrentValue", 1000 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.setCurrentValue( 10 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                String seqNameNew = "new_" + seqName;
                userSdb.renameSequence( seqName, seqNameNew );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void alterSequenceActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, DBSequence seq, String seqName,
            boolean executeNotSupport ) {
        DBSequence rootSeq = sdb.getSequence( seqName );
        seq.restart( 1 );
        seq.setAttributes( new BasicBSONObject( "CurrentValue", 1000 ) );
        seq.setCurrentValue( rootSeq.getNextValue() + 10 );
        String seqNameNew = "new_" + seqName;
        userSdb.renameSequence( seqName, seqNameNew );
        userSdb.renameSequence( seqNameNew, seqName );
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                seq.fetch( 2 );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.getNextValue();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                seq.getCurrentValue();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void alterDomainActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, Domain domain, List< String > groupNames,
            boolean executeNotSupport ) {
        BasicBSONList groupList = new BasicBSONList();
        groupList.add( groupNames.get( 0 ) );
        domain.alterDomain( new BasicBSONObject( "Groups", groupList ) );
        groupList.add( groupNames.get( 1 ) );
        domain.setAttributes( new BasicBSONObject( "Groups", groupList ) );
        groupList.clear();
        groupList.add( groupNames.get( 2 ) );
        domain.addGroups( new BasicBSONObject( "Groups", groupList ) );
        domain.removeGroups( new BasicBSONObject( "Groups", groupList ) );
        domain.setGroups( new BasicBSONObject( "Groups", groupNames ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getList( Sequoiadb.SDB_LIST_RECYCLEBIN, null, null,
                        null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void listActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        int[] lists = { Sequoiadb.SDB_LIST_CONTEXTS,
                Sequoiadb.SDB_LIST_CONTEXTS_CURRENT,
                Sequoiadb.SDB_LIST_SESSIONS,
                Sequoiadb.SDB_LIST_SESSIONS_CURRENT,
                Sequoiadb.SDB_LIST_COLLECTIONS,
                Sequoiadb.SDB_LIST_COLLECTIONSPACES,
                Sequoiadb.SDB_LIST_STORAGEUNITS, Sequoiadb.SDB_LIST_GROUPS,
                Sequoiadb.SDB_LIST_TASKS, Sequoiadb.SDB_LIST_TRANSACTIONS,
                Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT,
                Sequoiadb.SDB_LIST_SVCTASKS, Sequoiadb.SDB_LIST_SEQUENCES,
                Sequoiadb.SDB_LIST_USERS, Sequoiadb.SDB_LIST_BACKUPS,
                Sequoiadb.SDB_LIST_DATASOURCES, Sequoiadb.SDB_LIST_RECYCLEBIN };
        DBCursor cursor = null;
        for ( int list : lists ) {
            cursor = userSdb.getList( list, null, null, null );
            cursor.getNext();
            cursor.close();
        }

        cursor = userSdb.listCollectionSpaces();
        cursor.getNext();
        cursor.close();

        cursor = userSdb.listReplicaGroups();
        cursor.getNext();
        cursor.close();

        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        group.getMaster();
        group.getSlave();
        group.getDetail();
        group.getGroupName();

        String domainName = "domain_" + csName;
        sdb.createDomain( domainName,
                new BasicBSONObject( "Groups", groupNames ) );
        userSdb.getDomain( domainName );
        cursor = userSdb.listDomains( null, null, null, null );
        cursor.getNext();
        cursor.close();
        sdb.dropDomain( domainName );

        cursor = userSdb.listBackup( null, null, null, null );
        cursor.getNext();
        cursor.close();

        cursor = userSdb.listTasks( null, null, null, null );
        cursor.getNext();
        cursor.close();

        String seqName = "seq_" + csName;
        sdb.createSequence( seqName,
                ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );
        userSdb.getSequence( seqName );
        sdb.dropSequence( seqName );

        cursor = userSdb.listDataSources( null, null, null, null );
        cursor.getNext();
        cursor.close();

        cursor = userSdb.listProcedures( null );
        cursor.getNext();
        cursor.close();

        cursor = userSdb.getRecycleBin().list( null, null, null );
        cursor.getNext();
        cursor.close();

        int[] snapshots = { Sequoiadb.SDB_SNAP_CONTEXTS,
                Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT,
                Sequoiadb.SDB_SNAP_SESSIONS,
                Sequoiadb.SDB_SNAP_SESSIONS_CURRENT,
                Sequoiadb.SDB_SNAP_COLLECTIONS,
                Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                Sequoiadb.SDB_SNAP_DATABASE, Sequoiadb.SDB_SNAP_SYSTEM,
                Sequoiadb.SDB_SNAP_CATALOG, Sequoiadb.SDB_SNAP_TRANSACTIONS,
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT,
                Sequoiadb.SDB_SNAP_ACCESSPLANS, Sequoiadb.SDB_SNAP_HEALTH,
                Sequoiadb.SDB_SNAP_CONFIGS, Sequoiadb.SDB_SNAP_SVCTASKS,
                Sequoiadb.SDB_SNAP_SEQUENCES, Sequoiadb.SDB_SNAP_QUERIES,
                Sequoiadb.SDB_SNAP_LOCKWAITS, Sequoiadb.SDB_SNAP_LATCHWAITS,
                Sequoiadb.SDB_SNAP_INDEXSTATS, Sequoiadb.SDB_SNAP_TASKS,
                Sequoiadb.SDB_SNAP_TRANSWAITS, Sequoiadb.SDB_SNAP_TRANSDEADLOCK,
                Sequoiadb.SDB_SNAP_RECYCLEBIN };
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            for ( int snapshot : snapshots ) {
                try {
                    userSdb.getSnapshot( snapshot, new BasicBSONObject(), null,
                            null );
                    Assert.fail( "should error but success" );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }

    public static void snapshotActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        int[] snapshots = { Sequoiadb.SDB_SNAP_CONTEXTS,
                Sequoiadb.SDB_SNAP_CONTEXTS_CURRENT,
                Sequoiadb.SDB_SNAP_SESSIONS,
                Sequoiadb.SDB_SNAP_SESSIONS_CURRENT,
                Sequoiadb.SDB_SNAP_COLLECTIONS,
                Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                Sequoiadb.SDB_SNAP_DATABASE, Sequoiadb.SDB_SNAP_SYSTEM,
                Sequoiadb.SDB_SNAP_CATALOG, Sequoiadb.SDB_SNAP_TRANSACTIONS,
                Sequoiadb.SDB_SNAP_TRANSACTIONS_CURRENT,
                Sequoiadb.SDB_SNAP_ACCESSPLANS, Sequoiadb.SDB_SNAP_HEALTH,
                Sequoiadb.SDB_SNAP_CONFIGS, Sequoiadb.SDB_SNAP_SVCTASKS,
                Sequoiadb.SDB_SNAP_SEQUENCES, Sequoiadb.SDB_SNAP_QUERIES,
                Sequoiadb.SDB_SNAP_LOCKWAITS, Sequoiadb.SDB_SNAP_LATCHWAITS,
                Sequoiadb.SDB_SNAP_INDEXSTATS, Sequoiadb.SDB_SNAP_TASKS,
                Sequoiadb.SDB_SNAP_TRANSWAITS, Sequoiadb.SDB_SNAP_TRANSDEADLOCK,
                Sequoiadb.SDB_SNAP_RECYCLEBIN };
        DBCursor cursor = null;
        for ( int snapshot : snapshots ) {
            cursor = userSdb.getSnapshot( snapshot, new BasicBSONObject(), null,
                    null );
            cursor.getNext();
            cursor.close();
        }

        cursor = userSdb.getRecycleBin().snapshot( null, null, null );
        cursor.getNext();
        cursor.close();

        int[] lists = { Sequoiadb.SDB_LIST_CONTEXTS,
                Sequoiadb.SDB_LIST_CONTEXTS_CURRENT,
                Sequoiadb.SDB_LIST_SESSIONS,
                Sequoiadb.SDB_LIST_SESSIONS_CURRENT,
                Sequoiadb.SDB_LIST_COLLECTIONS,
                Sequoiadb.SDB_LIST_COLLECTIONSPACES,
                Sequoiadb.SDB_LIST_STORAGEUNITS, Sequoiadb.SDB_LIST_GROUPS,
                Sequoiadb.SDB_LIST_TASKS, Sequoiadb.SDB_LIST_TRANSACTIONS,
                Sequoiadb.SDB_LIST_TRANSACTIONS_CURRENT,
                Sequoiadb.SDB_LIST_SVCTASKS, Sequoiadb.SDB_LIST_SEQUENCES,
                Sequoiadb.SDB_LIST_USERS, Sequoiadb.SDB_LIST_BACKUPS,
                Sequoiadb.SDB_LIST_DATASOURCES, Sequoiadb.SDB_LIST_RECYCLEBIN };
        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            for ( int list : lists ) {
                try {
                    userSdb.getList( list, null, null, null );
                    Assert.fail( "should error but success" );
                } catch ( BaseException e ) {
                    if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                            .getErrorCode() ) {
                        throw e;
                    }
                }
            }
        }
    }

    public static void listCollectionSpacesActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        DBCursor cursor = userSdb.listCollectionSpaces();
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getSnapshot( Sequoiadb.SDB_SNAP_COLLECTIONSPACES,
                        new BasicBSONObject(), null, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void listBackupActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String csName, String clName,
            boolean executeNotSupport ) {
        DBCursor cursor = userSdb.listBackup( null, null, null, null );
        cursor.getNext();
        cursor.close();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.getList( Sequoiadb.SDB_LIST_USERS, null, null, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void createRGActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String groupName, boolean executeNotSupport ) {
        // 执行支持的操作
        userSdb.createReplicaGroup( groupName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.removeReplicaGroup( groupName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        sdb.removeReplicaGroup( groupName );
    }

    public static void removeRGActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, String groupName, boolean executeNotSupport ) {
        // 执行支持的操作
        sdb.createReplicaGroup( groupName );
        userSdb.removeReplicaGroup( groupName );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.createReplicaGroup( groupName );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void getRGActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        group.getGroupName();
        DBCursor cursor = userSdb.listReplicaGroups();
        cursor.getNext();
        cursor.close();

        List< String > nodeAddress = CommLib.getNodeAddress( sdb,
                groupNames.get( 0 ) );
        group.getNode( nodeAddress.get( 0 ) );
        group.getMaster();
        group.getSlave();
        group.getDetail();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                group.reelect();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void deleteConfActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        userSdb.deleteConfig( new BasicBSONObject( "metacacheexpired", 1 ),
                new BasicBSONObject() );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.updateConfig(
                        new BasicBSONObject( "metacacheexpired", 30 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void updateConfActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        userSdb.updateConfig( new BasicBSONObject( "metacacheexpired", 30 ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.deleteConfig(
                        new BasicBSONObject( "metacacheexpired", 1 ),
                        new BasicBSONObject() );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
        sdb.deleteConfig( new BasicBSONObject( "metacacheexpired", 1 ),
                new BasicBSONObject() );
    }

    public static void getNodeActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        group.getGroupName();
        DBCursor cursor = userSdb.listReplicaGroups();
        cursor.getNext();
        cursor.close();

        List< String > nodeAddress = CommLib.getNodeAddress( sdb,
                groupNames.get( 0 ) );
        group.getNode( nodeAddress.get( 0 ) );
        group.getMaster();
        group.getSlave();
        group.getDetail();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                group.reelect();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void createNodeActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        String hostName = rootGroup.getMaster().getHostName();
        int port = SdbTestBase.reservedPortBegin + 10;
        String dataPath = SdbTestBase.reservedDir + "/data/" + port;

        // 执行支持的操作
        group.createNode( hostName, port, dataPath );

        rootGroup.detachNode( hostName, port,
                new BasicBSONObject( "KeepData", true ) );

        group.attachNode( hostName, port,
                new BasicBSONObject( "KeepData", true ) );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                group.removeNode( hostName, port, null );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }

            try {
                group.detachNode( hostName, port,
                        new BasicBSONObject( "KeepData", true ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootGroup.removeNode( hostName, port, null );
    }

    public static void removeNodeActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        String hostName = rootGroup.getMaster().getHostName();
        int port = SdbTestBase.reservedPortBegin + 10;
        String dataPath = SdbTestBase.reservedDir + "/data/" + port;
        rootGroup.createNode( hostName, port, dataPath );

        // 执行支持的操作
        group.detachNode( hostName, port,
                new BasicBSONObject( "KeepData", true ) );

        rootGroup.attachNode( hostName, port,
                new BasicBSONObject( "KeepData", true ) );

        group.removeNode( hostName, port, null );

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                group.reelect();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void reelectActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        // 执行支持的操作
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        group.reelect();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                userSdb.removeReplicaGroup( groupNames.get( 0 ) );
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }
    }

    public static void startRGActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        // 执行支持的操作
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        rootGroup.stop();
        group.start();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                group.stop();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        CommLib.waitGroupSelectMasterNode( sdb, groupNames.get( 0 ), 300 );
    }

    public static void stopRGActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        // 执行支持的操作
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        group.stop();
        rootGroup.start();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                group.start();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        CommLib.waitGroupSelectMasterNode( sdb, groupNames.get( 0 ), 300 );
    }

    public static void startNodeActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        // 执行支持的操作
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        Node slaveNode = group.getSlave();
        slaveNode.start();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                slaveNode.stop();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        CommLib.waitGroupSelectMasterNode( sdb, groupNames.get( 0 ), 300 );
    }

    public static void stopNodeActionSupportCommand( Sequoiadb sdb,
            Sequoiadb userSdb, boolean executeNotSupport ) {
        // 执行支持的操作
        List< String > groupNames = CommLib.getDataGroupNames( sdb );
        ReplicaGroup group = userSdb.getReplicaGroup( groupNames.get( 0 ) );
        ReplicaGroup rootGroup = sdb.getReplicaGroup( groupNames.get( 0 ) );
        Node slaveNode = group.getSlave();
        slaveNode.stop();

        // 执行部分不支持的操作
        if ( executeNotSupport ) {
            try {
                slaveNode.start();
                Assert.fail( "should error but success" );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != SDBError.SDB_NO_PRIVILEGES
                        .getErrorCode() ) {
                    throw e;
                }
            }
        }

        rootGroup.start();
        CommLib.waitGroupSelectMasterNode( sdb, groupNames.get( 0 ), 300 );
    }

    public static void removeUser( Sequoiadb sdb, String user,
            String password ) {
        try {
            sdb.removeUser( user, password );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_USER_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    public static void dropRole( Sequoiadb sdb, String roleName ) {
        try {
            sdb.dropRole( roleName );
        } catch ( BaseException e ) {
            if ( e.getErrorCode() != SDBError.SDB_AUTH_ROLE_NOT_EXIST
                    .getErrorCode() ) {
                throw e;
            }
        }
    }

    /**
     * generating byte to write lob
     *
     * @param length
     *            generating byte stream size
     * @return byte[] bytes
     */
    public static byte[] getRandomBytes( int length ) {
        byte[] bytes = new byte[ length ];
        Random random = new Random();
        random.nextBytes( bytes );
        return bytes;
    }

    public static String[] getRandomActions( String[] actions, int count ) {
        if ( count <= 0 || count > actions.length ) {
            throw new IllegalArgumentException( "Invalid count value" );
        }

        Random random = new Random();
        String[] randomActions = new String[ count ];

        for ( int i = 0; i < count; i++ ) {
            int randomIndex;
            do {
                randomIndex = random.nextInt( actions.length );
            } while ( contains( randomActions, actions[ randomIndex ] ) );

            randomActions[ i ] = actions[ randomIndex ];
        }

        return randomActions;
    }

    public static boolean contains( String[] array, String value ) {
        for ( String element : array ) {
            if ( element != null && element.equals( value ) ) {
                return true;
            }
        }
        return false;
    }

    public static String arrayToCommaSeparatedString( String[] array ) {
        StringBuilder sb = new StringBuilder();

        for ( String element : array ) {
            if ( sb.length() > 0 ) {
                sb.append( ", " );
            }
            sb.append( "'" ).append( element ).append( "'" );
        }

        return sb.toString();
    }

    public static boolean compareBSONListsIgnoreOrder( BasicBSONList list1,
            BasicBSONList list2 ) {
        if ( list1.size() != list2.size() )
            return false;

        Set< Object > set1 = new HashSet<>( list1 );
        Set< Object > set2 = new HashSet<>( list2 );

        return set1.equals( set2 );
    }
}
