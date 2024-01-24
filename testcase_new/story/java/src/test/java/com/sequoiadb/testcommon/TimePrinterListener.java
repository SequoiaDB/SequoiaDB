package com.sequoiadb.testcommon;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;

import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

/**
 * Created by laojingtang on 17-11-23.
 */
public class TimePrinterListener extends TestListenerAdapter {

    @Override
    public void onTestFailure( ITestResult itr ) {
        System.out.println( "runGroup"
                + itr.getMethod().getXmlTest().getIncludedGroups().toString()
                + " " + itr.getMethod().getTestClass().getRealClass()
                + " failed" );
        super.onTestFailure( itr );
    }

    @Override
    public void onTestFailedButWithinSuccessPercentage( ITestResult itr ) {
        super.onTestFailedButWithinSuccessPercentage( itr );

    }

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
        if ( tr.getMethod().isBeforeTestConfiguration() ) {
            SdbTestBase.setRunGroup(
                    tr.getTestClass().getXmlTest().getIncludedGroups() );
        }
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
        Sequoiadb sdb = null;
        ArrayList< String > groups = new ArrayList<>(
                Arrays.asList( tr.getMethod().getGroups() ) );

        try {
            if ( groups.contains( SdbTestBase.RBAC ) ) {
                sdb = new Sequoiadb( SdbTestBase.coordUrl,
                        SdbTestBase.rootUserName,
                        SdbTestBase.rootUserPassword );
            } else {
                sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            }
            sdb.msg( getCurTimeStr() + "\tBegin testcase: "
                    + getTestMethodName( tr ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }

    private void dbMsgEndTime( ITestResult tr ) {
        Sequoiadb sdb = null;
        ArrayList< String > groups = new ArrayList<>(
                Arrays.asList( tr.getMethod().getGroups() ) );

        try {
            if ( groups.contains( SdbTestBase.RBAC ) ) {
                sdb = new Sequoiadb( SdbTestBase.coordUrl,
                        SdbTestBase.rootUserName,
                        SdbTestBase.rootUserPassword );
            } else {
                sdb = new Sequoiadb( SdbTestBase.coordUrl, "", "" );
            }
            sdb.msg( getCurTimeStr() + "\tEnd testcase: "
                    + getTestMethodName( tr ) );
        } catch ( BaseException e ) {
            e.printStackTrace();
        } finally {
            if ( sdb != null ) {
                sdb.close();
            }
        }
    }
}
