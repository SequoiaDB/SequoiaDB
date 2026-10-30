package com.mongodb.utils;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.nio.charset.StandardCharsets;
import javax.xml.bind.DatatypeConverter;

import org.apache.log4j.Logger;
import org.testng.ITestResult;
import org.testng.TestListenerAdapter;

/**
 * Created by laojingtang on 17-11-23.
 */
public class TimePrinterListener extends TestListenerAdapter {
    private static final Logger logger = Logger
            .getLogger( TimePrinterListener.class );

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
        if ( itr.getName().equals( "springTestContextAfterTestClass" ) ) {
            printEndTime( itr );
        }
    }

    @Override
    public void onConfigurationFailure( ITestResult itr ) {
        super.onConfigurationFailure( itr );
        printFailure( itr, "configuration" );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
        }
    }

    @Override
    public void onConfigurationSkip( ITestResult itr ) {
        super.onConfigurationSkip( itr );
        if ( itr.getMethod().isAfterClassConfiguration() ) {
            printEndTime( itr );
        }
    }

    @Override
    public void beforeConfiguration( ITestResult itr ) {
        super.beforeConfiguration( itr );
        if ( itr.getName().equals( "springTestContextBeforeTestClass" ) ) {
            printBeginTime( itr );
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

    private void printBeginTime( ITestResult itr ) {
        logger.info( "\tBegin testcase: java-mongo-"
                + MongodbTestBase.javaMongoVersion + " "
                + getTestMethodName( itr ) );
    }

    private void printEndTime( ITestResult itr ) {
        logger.info(
                "\tEnd testcase: java-mongo-" + MongodbTestBase.javaMongoVersion
                        + " " + getTestMethodName( itr ) );
    }

    private String getTestMethodName( ITestResult itr ) {
        return itr.getTestClass().getRealClass().getName();
    }
}
