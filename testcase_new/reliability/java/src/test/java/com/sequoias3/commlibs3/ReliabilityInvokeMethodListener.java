package com.sequoias3.commlibs3;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import org.apache.log4j.Logger;
import org.testng.IInvokedMethod;
import org.testng.IInvokedMethodListener;
import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-18
 * @Version 1.00
 */
public class ReliabilityInvokeMethodListener extends TestListenerAdapter
        implements IInvokedMethodListener {
    private static final String errorCode = "DBError";
    private static final Logger logger = Logger
            .getLogger( ReliabilityInvokeMethodListener.class );

    @Override
    public void beforeInvocation( IInvokedMethod iInvokedMethod,
            ITestResult iTestResult ) {
    }

    @Override
    public void afterInvocation( IInvokedMethod iInvokedMethod,
            ITestResult iTestResult ) {
        if ( iTestResult.getStatus() == ITestResult.FAILURE ) {
            iTestResult.getTestContext().getSuite().getSuiteState().failed();
        }
    }

    @Override
    public void onConfigurationSuccess( ITestResult itr ) {
        super.onConfigurationSuccess( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            logger.info( "[" + itr.getInstanceName() + "] end" );
        }
    }

    @Override
    public void onConfigurationFailure( ITestResult itr ) {
        super.onConfigurationFailure( itr );
        Throwable throwable = itr.getThrowable();
        if ( throwable != null
                && throwable.getMessage().contains( errorCode ) ) {
            logger.info( "[ " + itr.getInstanceName() + ":transaction snapshot:"
                    + transSnapshot() + "]" );
        }
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            logger.info( "[" + itr.getInstanceName() + "] end" );
        }
    }

    @Override
    public void onConfigurationSkip( ITestResult itr ) {
        super.onConfigurationSkip( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            logger.info( "[" + itr.getInstanceName() + "] end" );
        }
    }

    @Override
    public void beforeConfiguration( ITestResult itr ) {
        super.beforeConfiguration( itr );
        if ( itr.getMethod().isBeforeClassConfiguration() ) {
            logger.info( "[" + itr.getInstanceName() + "] start" );
        }
    }

    @Override
    public void onTestFailure( ITestResult tr ) {
        super.onTestFailure( tr );
        Throwable throwable = tr.getThrowable();
        if ( throwable != null
                && throwable.getMessage().contains( errorCode ) ) {
            logger.info( "[ " + tr.getInstanceName() + ":transaction snapshot:"
                    + transSnapshot() + "]" );
        }
    }

    // transactions snapshot
    private static String transSnapshot() {
        Sequoiadb db = null;
        DBCursor cursor = null;
        StringBuilder builder = new StringBuilder();
        builder.append( "[" );
        try {
            db = new Sequoiadb( S3TestBase.getDefaultCoordUrl(), "", "" );
            cursor = db.getSnapshot( 9, "", "", "" );
            while ( cursor.hasNext() ) {
                builder.append( cursor.getNext().toString() + ",\n" );
            }
        } catch ( Exception e ) {
            System.out.println( "snapshot 9 failed,coord = "
                    + S3TestBase.getDefaultCoordUrl() );
            e.printStackTrace();
        } finally {
            if ( cursor != null ) {
                cursor.close();
            }
            if ( db != null ) {
                db.close();
            }
        }
        builder.append( "]" );
        return builder.toString();
    }
}
