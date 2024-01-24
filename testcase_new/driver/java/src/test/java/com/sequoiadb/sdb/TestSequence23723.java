package com.sequoiadb.sdb;

import com.sequoiadb.base.*;
import com.sequoiadb.testcommon.CommLib;
import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.testng.Assert;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * @Descreption seqDB-23723:Sequence相关接口测试
 * @Author Yipan
 * @Date 2021/3/23
 */
public class TestSequence23723 extends SdbTestBase {
    private Sequoiadb sdb;
    private DBSequence sequence;
    private String sequenceOldName = "sequence23723a";
    private String sequenceNewName = "sequence23723b";

    @BeforeClass
    public void setUp() {
        sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        CommLib commlib = new CommLib();
        if ( commlib.isStandAlone( sdb ) ) {
            throw new SkipException( "skip StandAlone" );
        }
    }

    @Test
    public void test() {
        // 创建Sequence
        sequence = sdb.createSequence( sequenceOldName );

        // 删除Sequence
        sdb.dropSequence( sequenceOldName );

        // 指定参数创建Sequence
        BSONObject options1 = new BasicBSONObject();
        options1.put( "StartValue", 1 );
        options1.put( "MinValue", 1 );
        options1.put( "MaxValue", 1000000L );
        options1.put( "Increment", 1 );
        options1.put( "CacheSize", 1000 );
        options1.put( "AcquireSize", 1000 );
        options1.put( "Cycled", false );
        sdb.createSequence( sequenceOldName, options1 );

        // 快照查询
        BSONObject selector = new BasicBSONObject();
        selector.put( "StartValue", "" );
        selector.put( "MinValue", "" );
        selector.put( "MaxValue", "" );
        selector.put( "Increment", "" );
        selector.put( "CacheSize", "" );
        selector.put( "AcquireSize", "" );
        selector.put( "Cycled", "" );
        DBCursor snapshot = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES,
                new BasicBSONObject( "Name", sequenceOldName ), selector,
                null );
        Assert.assertEquals( snapshot.getCurrent().toString(),
                options1.toString() );

        // Sequence重命名
        sdb.renameSequence( sequenceOldName, sequenceNewName );

        // getSequence
        sequence = sdb.getSequence( sequenceNewName );

        // getName
        Assert.assertEquals( sequence.getName(), sequenceNewName );

        // fetch连续获取10个序列
        BSONObject act = sequence.fetch( 10 );
        BSONObject exp = new BasicBSONObject();
        exp.put( "NextValue", 1 );
        exp.put( "ReturnNum", 10 );
        exp.put( "Increment", 1 );
        Assert.assertEquals( act.toString(), exp.toString() );

        // getCurrentValue获取当前值
        long currentValue = sequence.getCurrentValue();
        Assert.assertEquals( currentValue, 10 );

        // getNextValue获取下一个值
        long nextValue = sequence.getNextValue();
        Assert.assertEquals( nextValue, 11 );

        // restart设定起始为100获取当前值
        sequence.restart( 100 );
        Assert.assertEquals( sequence.getNextValue(), 100 );

        // setCurrentValue设定当前值为1000，获取当前值
        sequence.setCurrentValue( 1000 );
        Assert.assertEquals( sequence.getCurrentValue(), 1000 );

        // setAttributes修改属性
        BSONObject options2 = new BasicBSONObject();
        options2.put( "AcquireSize", 2000 );
        options2.put( "CacheSize", 2000 );
        options2.put( "Cycled", false );
        options2.put( "Increment", 10 );
        options2.put( "MaxValue", 1000001L );
        options2.put( "MinValue", 10 );
        options2.put( "StartValue", 10 );
        sequence.setAttributes( options2 );

        // 快照查询
        snapshot = sdb.getSnapshot( Sequoiadb.SDB_SNAP_SEQUENCES,
                new BasicBSONObject( "Name", sequenceNewName ), selector,
                null );
        Assert.assertEquals( snapshot.getCurrent().toString(),
                options2.toString() );
    }

    @AfterClass
    public void tearDown() {
        sdb.dropSequence( sequenceNewName );
        sdb.close();
    }
}
