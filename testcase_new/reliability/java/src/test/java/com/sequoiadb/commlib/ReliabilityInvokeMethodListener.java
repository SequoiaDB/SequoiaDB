package com.sequoiadb.commlib;

import java.util.List;

import org.testng.IInvokedMethod;
import org.testng.IInvokedMethodListener;
import org.testng.ITestResult;

/**
 * @FileName
 * @Author laojingtang
 * @Date 17-8-18
 * @Version 1.00
 */
public class ReliabilityInvokeMethodListener implements IInvokedMethodListener {
    @Override
    public void beforeInvocation( IInvokedMethod iInvokedMethod,
            ITestResult iTestResult ) {
        if ( iTestResult != null && iTestResult.getTestClass() != null
                && iTestResult.getTestClass().getXmlTest() != null ) {
            List< String > testGroups = iTestResult.getTestClass().getXmlTest()
                    .getIncludedGroups();
            SdbTestBase.setTestGroup( testGroups );
        }
    }

    @Override
    public void afterInvocation( IInvokedMethod iInvokedMethod,
            ITestResult iTestResult ) {
        if ( iTestResult.getStatus() == ITestResult.FAILURE ) {
            iTestResult.getTestContext().getSuite().getSuiteState().failed();
        }
    }
}
