package com.sequoias3.commlibs3;

import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Created by laojingtang on 17-11-23.
 */
public class TimePrinterListener extends TestListenerAdapter {

    @Override
    public void onConfigurationSuccess( ITestResult itr ) {
        super.onConfigurationSuccess( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void onConfigurationFailure( ITestResult itr ) {
        super.onConfigurationFailure( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void onConfigurationSkip( ITestResult itr ) {
        super.onConfigurationSkip( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void beforeConfiguration( ITestResult tr ) {
        super.beforeConfiguration( tr );
        if ( tr.getMethod().isBeforeClassConfiguration() ) {
            printBeginTime( tr );
            dbMsgBeginTime( tr );
        }
    }

    private void printBeginTime( ITestResult tr ) {
        System.out.println( getCurTimeStr() + "\tBegin testcase: "
                + getTestMethodName( tr ) );
    }

    private void printEndTime( ITestResult tr ) {
        System.out.println( getCurTimeStr() + "\tEnd testcase: "
                + getTestMethodName( tr ) );
    }

    private String getTestMethodName( ITestResult tr ) {
        return tr.getTestClass().getRealClass().getName();
    }

    private String getCurTimeStr() {
        return new SimpleDateFormat( "yyyy-MM-dd HH:mm:ss:S" )
                .format( new Date() );
    }

    private void dbMsgBeginTime( ITestResult tr ) {
        try {
            Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            sdb.msg( getCurTimeStr() + "\tBegin testcase: "
                    + getTestMethodName( tr ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
        }
    }

    private void dbMsgEndTime( ITestResult tr ) {
        try {
            Sequoiadb sdb = new Sequoiadb( S3TestBase.coordUrl, "", "" );
            sdb.msg( getCurTimeStr() + "\tEnd testcase: "
                    + getTestMethodName( tr ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
        }
    }
}
