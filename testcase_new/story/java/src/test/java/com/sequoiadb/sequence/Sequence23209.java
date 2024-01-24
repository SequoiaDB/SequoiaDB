package com.sequoiadb.sequence;

import java.util.ArrayList;
import java.util.List;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
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
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-23209:并发操作多个序列
 * @Author chenxiaodan
 * @Date 2021-6-1
 */
public class Sequence23209 extends SdbTestBase {
    private Sequoiadb db = null;
    private List< String > coorUrl1;
    private String seq1 = "s1_23209";
    private String seq2 = "s2_23209";
    private String new_seq1 = "s1_new_23209";
    private String new_seq2 = "s2_new_23209";

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
        BSONObject obj1 = ( BSONObject ) JSON
                .parse( "{'Increment':2,'Cycled':true}" );
        List< BSONObject > exprList1 = new ArrayList<>();
        exprList1.add( obj1 );
        BSONObject obj2 = new BasicBSONObject();
        obj2.put( "StartValue", ( long ) 10 );
        obj2.put( "MaxValue", ( long ) 10000 );
        List< BSONObject > exprList2 = new ArrayList<>();
        exprList2.add( obj2 );

        ThreadExecutor thExecutor = new ThreadExecutor();
        thExecutor.addWorker(
                new CreateSequence( coorUrl1.get( 0 ), seq1, obj1 ) );
        thExecutor.addWorker(
                new CreateSequence( coorUrl1.get( 1 ), seq2, obj2 ) );
        thExecutor.run();
        SequenceUtil.checkSequence( db, seq1, exprList1 );
        SequenceUtil.checkSequence( db, seq2, exprList2 );

        BSONObject new_obj1 = new BasicBSONObject();
        new_obj1.put( "StartValue", ( long ) 5 );
        new_obj1.put( "MaxValue", ( long ) 50000 );
        List< BSONObject > new_exprList1 = new ArrayList<>();
        new_exprList1.add( new_obj1 );
        BSONObject new_obj2 = ( BSONObject ) JSON
                .parse( "{'Increment':2,'Cycled':true}" );
        List< BSONObject > new_exprList2 = new ArrayList<>();
        new_exprList2.add( new_obj2 );

        ThreadExecutor thExecutor1 = new ThreadExecutor();
        thExecutor1.addWorker(
                new AlterSequence( coorUrl1.get( 0 ), seq1, new_obj1 ) );
        thExecutor1.addWorker(
                new AlterSequence( coorUrl1.get( 1 ), seq2, new_obj2 ) );
        thExecutor1.run();
        SequenceUtil.checkSequence( db, seq1, new_exprList1 );
        SequenceUtil.checkSequence( db, seq2, new_exprList2 );

        ThreadExecutor thExecutor2 = new ThreadExecutor();
        thExecutor2.addWorker(
                new RenameSequence( coorUrl1.get( 0 ), seq1, new_seq1 ) );
        thExecutor2.addWorker(
                new RenameSequence( coorUrl1.get( 1 ), seq2, new_seq2 ) );
        thExecutor2.run();
        List< BSONObject > exprnull = new ArrayList<>();
        SequenceUtil.checkSequence( db, seq1, exprnull );
        SequenceUtil.checkSequence( db, seq2, exprnull );
        SequenceUtil.checkSequence( db, new_seq1, new_exprList1 );
        SequenceUtil.checkSequence( db, new_seq2, new_exprList2 );

        ThreadExecutor thExecutor3 = new ThreadExecutor();
        thExecutor3
                .addWorker( new DropSequence( coorUrl1.get( 0 ), new_seq1 ) );
        thExecutor3
                .addWorker( new DropSequence( coorUrl1.get( 1 ), new_seq2 ) );
        thExecutor3.run();
        SequenceUtil.checkSequence( db, new_seq1, exprnull );
        SequenceUtil.checkSequence( db, new_seq2, exprnull );
    }

    @AfterClass
    public void tearDown() {
        db.close();
    }

    private class CreateSequence {
        private String coorUrl;
        private String seq;
        private BSONObject obj;

        public CreateSequence( String coorUrl, String seq, BSONObject obj ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
            this.obj = obj;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发创建不同的序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.createSequence( seq, obj );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db1.close();
            }
        }
    }

    private class AlterSequence {
        private String coorUrl;
        private String seq;
        private BSONObject obj;

        public AlterSequence( String coorUrl, String seq, BSONObject obj ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
            this.obj = obj;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发修改不同的序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                DBSequence s = db1.getSequence( seq );
                s.setAttributes( obj );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db1.close();
            }
        }
    }

    private class RenameSequence {
        private String coorUrl;
        private String seq;
        private String seq_new;

        public RenameSequence( String coorUrl, String seq, String seq_new ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
            this.seq_new = seq_new;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发修改不同的序列名")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.renameSequence( seq, seq_new );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db1.close();
            }
        }
    }

    private class DropSequence {
        private String coorUrl;
        private String seq;

        public DropSequence( String coorUrl, String seq ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发删除不同的序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.dropSequence( seq );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db1.close();
            }
        }
    }
}
