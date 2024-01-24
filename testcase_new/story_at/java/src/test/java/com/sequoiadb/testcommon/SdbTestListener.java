/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:SdbTestListener.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年7月1日下午1:19:48
 *  @version 1.00
 */
package com.sequoiadb.testcommon;
import java.util.ArrayList;
import java.util.List;

import org.testng.ITestContext;
import org.testng.ITestListener ;
import org.testng.ITestResult;

import com.sequoiadb.base.Sequoiadb;

public class SdbTestListener implements ITestListener {
    private List<IChecker> lstChecker = new ArrayList<IChecker>() ;
    @Override
    public void onFinish( ITestContext testContext ) {
        System.out.println("finish exec " + testContext.getName() ) ;
        System.out.println("start check...") ;
        try(Sequoiadb db = new Sequoiadb(SdbTestBase.coordUrl, "", "")){
            for ( IChecker checker: lstChecker) {
                System.out.println("Start " + checker.getName() ) ;
                checker.check( db ) ;
                System.out.println("finish " + checker.getName() ) ;
            }
        }
        System.out.println("finish check...") ;
    }

    @Override
    public void onStart( ITestContext testContext ) {
        System.out.println("start exec " + testContext.getName() ) ;
        synchronized(lstChecker){
            if(lstChecker.isEmpty()) {
                lstChecker.add( new TransChecker() ) ;
                lstChecker.add( new SessionChecker() ) ;
                lstChecker.add( new ContextChecker() ) ;
            }
        }
        
    }

    @Override
    public void onTestFailedButWithinSuccessPercentage( ITestResult arg0 ) {
        
    }

    @Override
    public void onTestFailure( ITestResult arg0 ) {
        
    }

    @Override
    public void onTestSkipped( ITestResult arg0 ) {
        
    }

    @Override
    public void onTestStart( ITestResult arg0 ) {
        
    }

    @Override
    public void onTestSuccess( ITestResult arg0 ) {
        
    }

}
