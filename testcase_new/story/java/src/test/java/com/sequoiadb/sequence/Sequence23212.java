package com.sequoiadb.sequence;

import java.util.List;

import org.bson.BSONObject;
import org.bson.util.JSON;
import org.testng.SkipException;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.CommLib;
import com.sequoiadb.testcommon.SdbTestBase;
import com.sequoiadb.threadexecutor.ThreadExecutor;
import com.sequoiadb.threadexecutor.annotation.ExecuteOrder;

/**
 * @FileName seqDB-23212:并发getNextValue、fetch同一个序列
 * @Author chenxiaodan
 * @Date 2021-6-4
 */
public class Sequence23212 extends SdbTestBase {
    private Sequoiadb db = null;
    private List< String > coorUrl1;
    private String seq_name = "s1_23212";

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
        db.createSequence( seq_name,
                ( BSONObject ) JSON.parse( "{'Cycled':true}" ) );
    }

    @Test
    public void test() throws Exception {
        GetSeqNextValue getSequenceValue = new GetSeqNextValue(
                coorUrl1.get( 0 ), seq_name );
        FetchSeq fetchSeq = new FetchSeq( coorUrl1.get( 1 ), seq_name );
        ThreadExecutor thExecutor = new ThreadExecutor();
        thExecutor.addWorker( getSequenceValue );
        thExecutor.addWorker( fetchSeq );
        thExecutor.run();
    }

    @AfterClass
    public void tearDown() {
        db.dropSequence( seq_name );
        db.close();
    }

    private class GetSeqNextValue {
        private String coorUrl;
        private String seq;

        public GetSeqNextValue( String coorUrl, String seq ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发getNextValue")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.getSequence( seq ).getNextValue();
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db1.close();
            }
        }
    }

    private class FetchSeq {
        private String coorUrl;
        private String seq;

        public FetchSeq( String coorUrl, String seq ) {
            this.coorUrl = coorUrl;
            this.seq = seq;
        }

        @ExecuteOrder(step = 1, desc = "多线程并发fetch")
        private void Sequence() {
            Sequoiadb db1 = null;
            try {
                db1 = new Sequoiadb( coorUrl, "", "" );
                db1.getSequence( seq ).fetch( 10 );
            } catch ( BaseException e ) {
                throw e;
            } finally {
                db1.close();
            }
        }
    }
}
