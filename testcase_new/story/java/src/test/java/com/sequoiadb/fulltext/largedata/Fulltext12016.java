package com.sequoiadb.fulltext.largedata;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.bson.util.JSON;
import org.testng.Assert;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.fulltext.utils.FullTextDBUtils;
import com.sequoiadb.fulltext.utils.FullTextESUtils;
import com.sequoiadb.fulltext.utils.FullTextUtils;
import com.sequoiadb.fulltext.utils.StringUtils;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.FullTestBase;

/**
 * FileName: Fulltext12016: 插入记录并创建全文索引再执行hash切分
 * 
 * @author liuxiaoxuan
 * @Date 2018.11.20
 */
public class Fulltext12016 extends FullTestBase {

    private String clName = "ES_hash_12016";
    private String textIndexName = "fulltext12016";
    private String srcGroupName = "";
    private String destGroupName = "";

    private List< String > cappedNames = null;
    private List< String > esIndexNames = null;

    @Override
    protected void initTestProp() {
        caseProp.setProperty( IGNORESTANDALONE, "true" );
        caseProp.setProperty( IGNOREONEGROUP, "true" );

        ArrayList< String > groupsName = CommLib.getDataGroupNames( sdb );
        srcGroupName = groupsName.get( 0 );
        destGroupName = groupsName.get( 1 );
        caseProp.setProperty( CLNAME, clName );
        caseProp.setProperty( CLOPT,
                "{ShardingKey:{a:1},ShardingType:'hash',Group:'" + srcGroupName
                        + "'}" );
    }

    @Override
    protected void caseFini() throws Exception {
        // 检查全文索引是否残留
        if ( esIndexNames != null ) {
            Assert.assertTrue( FullTextUtils.isIndexDeleted( sdb, esIndexNames,
                    cappedNames ) );
        }
    }

    @Test
    public void test() throws Exception {
        // 创建全文索引，包含分区键和非分区键
        BSONObject indexObj = new BasicBSONObject();
        indexObj.put( "a", "text" );
        indexObj.put( "b", "text" );
        indexObj.put( "c", "text" );
        indexObj.put( "d", "text" );
        indexObj.put( "e", "text" );
        indexObj.put( "f", "text" );
        cl.createIndex( textIndexName, indexObj, false, false );

        insertData( cl, FullTextUtils.INSERT_NUMS );

        cl.split( srcGroupName, destGroupName, 50 );

        cappedNames = new ArrayList< String >();
        cappedNames.add( FullTextDBUtils.getCappedName( cl, textIndexName ) );
        esIndexNames = FullTextDBUtils.getESIndexNames( cl, textIndexName );

        // 检查ES数据是否同步，且主备节点的原始集合和固定集合数据是否一致
        Assert.assertTrue( FullTextUtils.isIndexCreated( cl, textIndexName,
                FullTextUtils.INSERT_NUMS ) );

        // 分别检查源组、目标组对应的全文索引同步数据是否正确
        Node srcMaster = sdb.getReplicaGroup( srcGroupName ).getMaster();
        Node destMaster = sdb.getReplicaGroup( destGroupName ).getMaster();
        String srcESIndexName = "";
        String destESIndexName = "";
        for ( String esIndexName : esIndexNames ) {
            if ( esIndexName.contains( srcGroupName ) ) {
                srcESIndexName = esIndexName;
            }
            if ( esIndexName.contains( destGroupName ) ) {
                destESIndexName = esIndexName;
            }
        }
        DBCollection srcCL = srcMaster.connect().getCollectionSpace( csName )
                .getCollection( clName );
        DBCollection destCL = destMaster.connect().getCollectionSpace( csName )
                .getCollection( clName );
        Assert.assertEquals( FullTextESUtils.getCountFromES( srcESIndexName ),
                srcCL.getCount() );
        Assert.assertEquals( FullTextESUtils.getCountFromES( destESIndexName ),
                destCL.getCount() );
    }

    private void insertData( DBCollection cl, int insertNums ) {
        List< BSONObject > insertObjs = new ArrayList<>();
        for ( int i = 0; i < 100; i++ ) {
            for ( int j = 0; j < insertNums / 100; j++ ) {
                insertObjs.add( ( BSONObject ) JSON
                        .parse( "{a: 'test_hash12016_" + i * j + "', b: '"
                                + StringUtils.getRandomString( 32 ) + "', c: '"
                                + StringUtils.getRandomString( 64 ) + "', d: '"
                                + StringUtils.getRandomString( 64 ) + "', e: '"
                                + StringUtils.getRandomString( 128 ) + "', f: '"
                                + StringUtils.getRandomString( 128 ) + "'}" ) );

            }
            cl.insert( insertObjs, 0 );
            insertObjs.clear();
        }
    }
}
