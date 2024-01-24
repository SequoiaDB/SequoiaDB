package com.sequoiadb.sequence;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.DBSequence;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ResultStore;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-23210:并发操作同一个序列
 * @Author chenxiaodan
 * @Date 2021-6-4
 */
public class Sequence23210 extends SdbTestBase {
    private Sequoiadb db = null;
    private List< String > coorUrl1;
    private String old_seq = "s1_23210";
    private String new_seq0 = "s0_new_23210";
    private String new_seq1 = "s1_new_23210";
    BSONObject obj0 = ( BSONObject ) JSON.parse( "{'Increment':2}" );
    BSONObject obj1 = ( BSONObject ) JSON.parse( "{'Increment':3}" );
    List< BSONObject > exprList = new ArrayList<>();
    List< BSONObject > exprnull = new ArrayList<>();
    BSONObject all_obj = null;

    @BeforeClass
    public void setUp() throws InterruptedException {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "skip standAlone mode" );
        }
        coorUrl1 = CommLib.getAllCoordUrls( db );
        if ( coorUrl1.size() < 2 ) {
            throw new SkipException( "skip one coordNode" );
        }
    }

    @Test
    public void test() throws Exception {
        // 创建
        CreateSequence createSequence = new CreateSequence( coorUrl1.get( 0 ),
                old_seq, obj0 );
        CreateSequence createSequence1 = new CreateSequence( coorUrl1.get( 1 ),
                old_seq, obj1 );
        ThreadExecutor thExecutor = new ThreadExecutor();
        thExecutor.addWorker( createSequence );
        thExecutor.addWorker( createSequence1 );
        thExecutor.run();
        if ( createSequence.getRetCode() == 0 ) {
            all_obj = obj0;
        } else {
            all_obj = obj1;
        }
        exprList.add( all_obj );
        SequenceUtil.checkSequence( db, old_seq, exprList );
        exprList.clear();

        // 修改
        BSONObject new_obj0 = ( BSONObject ) JSON.parse( "{'Cycled':true}" );
        BSONObject new_obj1 = ( BSONObject ) JSON
                .parse( "{'AcquireSize':900}" );
        AlterSequence alterSequence = new AlterSequence( coorUrl1.get( 0 ),
                old_seq, new_obj0 );
        AlterSequence alterSequence1 = new AlterSequence( coorUrl1.get( 1 ),
                old_seq, new_obj1 );
        ThreadExecutor thExecutor1 = new ThreadExecutor();
        thExecutor1.addWorker( alterSequence );
        thExecutor1.addWorker( alterSequence1 );
        thExecutor1.run();
        if ( alterSequence.getRetCode() == 0 ) {
            all_obj.put( "Cycled", true );
        }
        if ( alterSequence1.getRetCode() == 0 ) {
            all_obj.put( "AcquireSize", 900 );
        }
        exprList.add( all_obj );
        SequenceUtil.checkSequence( db, old_seq, exprList );

        // 重命名
        RenameSequence renameSequence = new RenameSequence( coorUrl1.get( 0 ),
                old_seq, new_seq0 );
        RenameSequence renameSequence1 = new RenameSequence( coorUrl1.get( 1 ),
                old_seq, new_seq1 );
        ThreadExecutor thExecutor2 = new ThreadExecutor();
        thExecutor2.addWorker( renameSequence );
        thExecutor2.addWorker( renameSequence1 );
        thExecutor2.run();
        String fail_drop_seq, succeed_drop_seq;
        if ( renameSequence.getRetCode() == 0 ) {
            succeed_drop_seq = new_seq0;
            fail_drop_seq = new_seq1;
        } else {
            succeed_drop_seq = new_seq1;
            fail_drop_seq = new_seq0;
        }
        SequenceUtil.checkSequence( db, succeed_drop_seq, exprList );
        SequenceUtil.checkSequence( db, fail_drop_seq, exprnull );
        SequenceUtil.checkSequence( db, old_seq, exprnull );

        // 删除
        ThreadExecutor thExecutor3 = new ThreadExecutor();
        thExecutor3.addWorker(
                new DropSequence( coorUrl1.get( 0 ), succeed_drop_seq ) );
        thExecutor3.addWorker(
                new DropSequence( coorUrl1.get( 1 ), succeed_drop_seq ) );
        thExecutor3.run();
        SequenceUtil.checkSequence( db, succeed_drop_seq, exprnull );
    }

    @AfterClass
    public void tearDown() {
        db.close();
    }

    private class CreateSequence extends ResultStore {
        private String coorUrl;
        private String seq;
        private BSONObject obj;

        public CreateSequence( String coorUrl, String seq, BSONObject obj ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
            this.obj = obj;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发创建同一个序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.createSequence( seq, obj );
            } catch ( BaseException e ) {
                saveResult( -1, e );
            } finally {
                db1.close();
            }
        }
    }

    private class AlterSequence extends ResultStore {
        private String coorUrl;
        private String seq;
        private BSONObject obj;

        public AlterSequence( String coorUrl, String seq, BSONObject obj ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
            this.obj = obj;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发修改系统序列的相同属性")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                DBSequence s = db1.getSequence( seq );
                s.setAttributes( obj );
            } catch ( BaseException e ) {
                saveResult( -1, e );
            } finally {
                db1.close();
            }
        }
    }

    private class RenameSequence extends ResultStore {
        private String coorUrl;
        private String seq;
        private String new_seq;

        public RenameSequence( String coorUrl, String seq, String new_seq ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
            this.new_seq = new_seq;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发修改相同序列的序列名")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.renameSequence( seq, new_seq );
            } catch ( BaseException e ) {
                saveResult( -1, e );
            } finally {
                db1.close();
            }
        }
    }

    private class DropSequence extends ResultStore {
        private String coorUrl;
        private String seq;

        public DropSequence( String coorUrl, String seq ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发删除相同的序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.dropSequence( seq );
            } catch ( BaseException e ) {
                if ( e.getErrorCode() != -324 ) {
                    throw e;
                }
            } finally {
                db1.close();
            }
        }
    }
}
