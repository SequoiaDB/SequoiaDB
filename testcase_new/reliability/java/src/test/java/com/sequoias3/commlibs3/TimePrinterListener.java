package com.sequoias3.commlibs3;

import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.Date;
import javax.xml.bind.DatatypeConverter;

/**
 * Created by laojingtang on 17-11-23.
 */
public class TimePrinterListener extends TestListenerAdapter {

    @Override
    public void onTestSuccess( ITestResult itr ) {
        super.onTestSuccess( itr );
        printResult( itr, "pass" );
    }

    @Override
    public void onTestFailure( ITestResult itr ) {
        super.onTestFailure( itr );
        printResult( itr, "fail" );
        printFailure( itr, "fail" );
    }

    @Override
    public void onTestSkipped( ITestResult itr ) {
        super.onTestSkipped( itr );
        printResult( itr, "skip" );
    }

    @Override
    public void onConfigurationSuccess( ITestResult itr ) {
        super.onConfigurationSuccess( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void onConfigurationFailure( ITestResult itr ) {
        super.onConfigurationFailure( itr );
        printFailure( itr, "configuration" );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void onConfigurationSkip( ITestResult itr ) {
        super.onConfigurationSkip( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            dbMsgEndTime( itr );
        }
    }

    @Override
    public void beforeConfiguration( ITestResult tr ) {
        super.beforeConfiguration( tr );
        if ( tr.getMethod().isBeforeClassConfiguration() ) {
            dbMsgBeginTime( tr );
        }
    }

    private synchronized void printResult( ITestResult tr, String status ) {
        long elapsed = Math.max( 0L, tr.getEndMillis() - tr.getStartMillis() );
        String name = tr.getTestClass().getRealClass().getName() + "#"
                + tr.getMethod().getMethodName();
        System.out.printf( "TEST_RESULT|%s|%s|%s%n", name, status, elapsed );
    }

    private synchronized void printFailure( ITestResult tr, String type ) {
        StringWriter detail = new StringWriter();
        Throwable throwable = tr.getThrowable();
        if ( throwable == null ) {
            detail.append( "TestNG did not provide a throwable." );
        } else {
            throwable.printStackTrace( new PrintWriter( detail ) );
        }
        long elapsed = Math.max( 0L, tr.getEndMillis() - tr.getStartMillis() );
        String name = tr.getTestClass().getRealClass().getName() + "#"
                + tr.getMethod().getMethodName();
        String encoded = DatatypeConverter.printBase64Binary(
                detail.toString().getBytes( StandardCharsets.UTF_8 ) );
        System.out.printf( "TEST_FAILURE|%s|%s|%s|%s%n", type, name, elapsed,
                encoded );
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
