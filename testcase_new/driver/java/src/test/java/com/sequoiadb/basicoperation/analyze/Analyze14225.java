package com.sequoiadb.basicoperation.analyze;

import com.sequoiadb.base.DBCollection;
import com.sequoiadb.base.Node;
import com.sequoiadb.base.ReplicaGroup;
import com.sequoiadb.testcommon.SdbTestBase;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import java.util.ArrayList;
import java.util.List;

import static org.testng.Assert.assertTrue;

/**
 * Created by laojingtang on 18-1-31.
 */
public class Analyze14225 extends SdbTestBase {
    private SdbWarpper db;
    private SdbClWarpper dbcl;
    private String srcGroup;
    private String destGroup;

    @BeforeClass
    public void setup() {
        String pre = this.getClass().getSimpleName();
        db = new SdbWarpper( coordUrl );
        List< ReplicaGroup > rgs = db.getDataRG();
        if ( rgs.size() < 2 ) {
            throw new SkipException( "need at least 2 rgs" );
        }
        srcGroup = rgs.get( 0 ).getGroupName();
        destGroup = rgs.get( 1 ).getGroupName();
        BSONObject options = new BasicBSONObject();
        options.put( "PageSize", 4096 );
        SdbCsProperties cs = new SdbCsProperties( pre + "cs", options );
        db.createCS( cs );
        String clName = this.getClass().getSimpleName();
        dbcl = db.createCL( SdbClProperties.newBuilder( cs, clName )
                .group( srcGroup ).shardingKey( new BasicBSONObject( "a", 1 ) )
                .shardingType( "range" ).build() );
    }

    @AfterClass
    public void teardown() {
        db.dropCollectionSpace( dbcl.getCSName() );
        db.close();
    }

    /**
     * 1.创建切分表落在不同组上，并创建索引，插入包含索引字段的数据，例如：插入数据包含a字段，值为0的2000条记录落在group1上，值为2000的记录落在group2上，其中a为索引字段
     * 2.在cl中使用索引字段执行查询，检查扫描方式，例如：使用a=0、a=2000分别执行查询
     * 3.执行统计，其中：参数指定group，执行步骤2中查询，检查扫描方式，例如：使用a=0执行查询
     * <p>
     * 1.数据插入成功，索引创建成功 2.均使用索引执行查询 3.其他组使用索引查询，统计后的组使用表扫描执行查询
     */
    @Test
    public void test() {
        // some records
        List< BSONObject > records = new ArrayList< BSONObject >();
        for ( int i = 0; i < 25; i++ ) {
            records.add( new BasicBSONObject( "a", 0 ).append( "b",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
            records.add( new BasicBSONObject( "a", 2000 ).append( "b",
                    AnalyzeUtil.getRandomString( 4096 ) ) );
        }

        dbcl.insert( records );
        dbcl.split( srcGroup, destGroup, new BasicBSONObject( "a", 200 ),
                new BasicBSONObject( "a", 200000 ) );

        Explain e = new Explain.Builder( dbcl )
                .matcher( new BasicBSONObject( "a", 0 ) )
                .options( new BasicBSONObject( "Run", true ) ).build();
        assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );

        e = new Explain.Builder( dbcl )
                .matcher( new BasicBSONObject( "a", 2000 ) )
                .options( new BasicBSONObject( "Run", true ) ).build();
        assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );

        db.analyze( new BasicBSONObject( "GroupName", srcGroup ) );

        // query should use tbscan
        SdbClWarpper srcCl = getGroupCl( srcGroup );
        try {
            e = new Explain.Builder( srcCl )
                    .matcher( new BasicBSONObject( "a", 0 ) )
                    .options( new BasicBSONObject( "Run", true ) ).build();
            assertTrue( e.isQueryUseTbscan(), e.getExplainResult() );
        } finally {
            srcCl.getSequoiadb().close();
        }
        SdbClWarpper destCl = getGroupCl( destGroup );
        try {
            e = new Explain.Builder( destCl )
                    .matcher( new BasicBSONObject( "a", 2000 ) )
                    .options( new BasicBSONObject( "Run", true ) ).build();
            assertTrue( e.isQueryUseIxscan(), e.getExplainResult() );
        } finally {
            destCl.getSequoiadb().close();
        }
    }

    private SdbClWarpper getGroupCl( String groupName ) {
        Node node = db.getReplicaGroup( groupName ).getMaster();
        @SuppressWarnings("resource")
        SdbWarpper temdb = new SdbWarpper( node.getNodeName() );
        DBCollection temcl = temdb.getCollectionSpace( dbcl.getCSName() )
                .getCollection( dbcl.getName() );
        return new SdbClWarpper( temcl );
    }
}
