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
 * @FileName seqDB-23211:并发修改删除同一个序列
 * @Author chenxiaodan
 * @Date 2021-6-4
 */
public class Sequence23211 extends SdbTestBase {
    private Sequoiadb db = null;
    private List< String > coorUrl1;
    private String seq_name = "s1_23211";

    @BeforeClass
    public void setUp() throws InterruptedException {
        db = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
        if ( CommLib.isStandAlone( db ) ) {
            throw new SkipException( "skip standAlone mode" );
        }
        coorUrl1 = CommLib.getAllCoordUrls( db );
        if ( coorUrl1.size() < 3 ) {
            throw new SkipException( "skip one coordNode" );
        }
        db.createSequence( seq_name,
                ( BSONObject ) JSON.parse( "{'Increment':10}" ) );
    }

    @Test
    public void test() throws Exception {
        BSONObject new_obj = ( BSONObject ) JSON.parse( "{'Cycled':true}" );
        AlterSequence alterSequence = new AlterSequence( coorUrl1.get( 0 ),
                seq_name, new_obj );
        DropSequence dropSequence = new DropSequence( coorUrl1.get( 1 ),
                seq_name );
        GetSequenceValue getSequenceValue = new GetSequenceValue(
                coorUrl1.get( 2 ), seq_name );
        ThreadExecutor thExecutor = new ThreadExecutor();
        thExecutor.addWorker( alterSequence );
        thExecutor.addWorker( dropSequence );
        thExecutor.addWorker( getSequenceValue );
        thExecutor.run();
        List< BSONObject > exprList = new ArrayList<>();
        if ( dropSequence.getRetCode() == 0 ) {
            exprList.clear();
        } else {
            if ( getSequenceValue.getRetCode() == 0
                    && alterSequence.getRetCode() == 0 ) {
                BSONObject all_sucess = ( BSONObject ) JSON
                        .parse( "{'Cycled':true,'CurrentValue':9991}" );
                exprList.add( all_sucess );
            } else if ( getSequenceValue.getRetCode() != 0
                    && alterSequence.getRetCode() == 0 ) {
                BSONObject get_sucess = ( BSONObject ) JSON
                        .parse( "{'Cycled':true,'CurrentValue':1}" );
                exprList.add( get_sucess );
            } else if ( getSequenceValue.getRetCode() == 0
                    && alterSequence.getRetCode() != 0 ) {
                BSONObject alter_sucess = ( BSONObject ) JSON
                        .parse( "{'Cycled':false,CurrentValue':9991}" );
                exprList.add( alter_sucess );
            }
        }
        SequenceUtil.checkSequence( db, seq_name, exprList );
        if ( dropSequence.getRetCode() != 0 ) {
            db.dropSequence( seq_name );
        }
    }

    @AfterClass
    public void tearDown() {
        db.close();
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

        @ExecuteOrder(step = 1, desc = "多线程并发修改同一个序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            DBSequence sequence = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                sequence = db1.getSequence( seq );
                if ( sequence != null ) {
                    sequence.setAttributes( obj );
                }
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

        @ExecuteOrder(step = 1, desc = "多线程并发删除同一个序列")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.dropSequence( seq );
            } catch ( BaseException e ) {
                saveResult( -1, e );
            } finally {
                db1.close();
            }
        }
    }

    private class GetSequenceValue extends ResultStore {
        private String coorUrl;
        private String seq;

        public GetSequenceValue( String coorUrl, String seq ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发获取同一个序列的值")
        private void Sequence() {
            Sequoiadb db1 = null;
            DBSequence sequence = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                sequence = db1.getSequence( seq );
                if ( sequence != null ) {
                    sequence.fetch( 5 );
                }
            } catch ( BaseException e ) {
                saveResult( -1, e );
            } finally {
                db1.close();
            }
        }
    }
}
