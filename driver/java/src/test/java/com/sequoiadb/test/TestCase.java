package com.sequoiadb.test;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;

/*
 * Super class of test class
 * */
public abstract class TestCase {
    @BeforeClass
    public static void setUpTestCase() {
    }

    @AfterClass
    public static void tearDownTestCase() {
    }

    @Before
    public void setUp() {
    }

    @After
    public void tearDown() {
    }
}
