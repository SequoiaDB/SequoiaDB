package com.sequoiadb.decimal;

import java.util.Date;

import org.bson.types.BSONDecimal;
import org.testng.Assert;
import org.testng.annotations.AfterClass;
import org.testng.annotations.BeforeClass;
import org.testng.annotations.Test;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.testcommon.SdbTestBase;

/**
 * Copyright (C), 2016-2016, ShenZhen info. Co., Ltd. FileName: Decimal9592.java
 * TestLink: seqDB-9592
 * 
 * @author zhaoyu
 * @Date 2016.9.28
 * @version 1.00
 */

public class Decimal9592 extends SdbTestBase {

    @BeforeClass
    public void setUp() {
        try {
        } catch ( BaseException e ) {
            Assert.fail( "prepare env failed" + e.getMessage() );
        }
    }

    @AfterClass
    public void tearDown() {
        try {
        } catch ( BaseException e ) {
            Assert.fail( "clear env failed, errMsg:" + e.getMessage() );
        }
    }

    @Test
    public void test() {
        String value1 = "-49406.5645841246544e-333";
        String value2 = "1.7e+310";
        String value3 = "2147483647";
        int precision = 1000;
        int scale = 500;
        try {
            BSONDecimal decimalNoPrecision1 = new BSONDecimal( value1 );
            BSONDecimal decimalWithPrecision1 = new BSONDecimal( value1,
                    precision, scale );
            BSONDecimal decimalNoPrecision2 = new BSONDecimal( value2 );
            BSONDecimal decimalWithPrecision2 = new BSONDecimal( value2,
                    precision, scale );
            BSONDecimal decimalNoPrecision3 = new BSONDecimal( value3 );
            BSONDecimal decimalWithPrecision3 = new BSONDecimal( value3,
                    precision, scale );
            BSONDecimal decimalWithPrecision4 = new BSONDecimal( value3, 10,
                    0 );
            int intNumber = Integer.parseInt( value3 );

            if ( decimalNoPrecision3.compareTo( decimalWithPrecision3 ) != 0 ) {
                Assert.fail(
                        "compare decimal data failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3
                                + ", decimalWithPrecision3 is :"
                                + decimalWithPrecision3 );
            }

            if ( decimalNoPrecision3
                    .compareTo( decimalWithPrecision2 ) != -1 ) {
                Assert.fail(
                        "compare decimal data failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3
                                + ", decimalWithPrecision2 is :"
                                + decimalWithPrecision2 );
            }

            if ( decimalNoPrecision3.compareTo( decimalWithPrecision1 ) != 1 ) {
                Assert.fail(
                        "compare decimal data failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3
                                + ", decimalWithPrecision1 is :"
                                + decimalWithPrecision1 );
            }

            if ( decimalWithPrecision3
                    .compareTo( decimalWithPrecision3 ) != 0 ) {
                Assert.fail(
                        "compare decimal data failed, decimalWithPrecision3 is :"
                                + decimalWithPrecision3
                                + ", decimalWithPrecision3 is :"
                                + decimalWithPrecision3 );
            }

            if ( decimalWithPrecision3
                    .compareTo( decimalWithPrecision2 ) != -1 ) {
                Assert.fail(
                        "compare decimal data failed, decimalWithPrecision3 is :"
                                + decimalWithPrecision3
                                + "decimalWithPrecision2 is :"
                                + decimalWithPrecision2 );
            }

            if ( decimalWithPrecision3
                    .compareTo( decimalWithPrecision1 ) != 1 ) {
                Assert.fail(
                        "compare decimal data failed, decimalWithPrecision3 is :"
                                + decimalWithPrecision3
                                + ", decimalWithPrecision1 is :"
                                + decimalWithPrecision1 );
            }

            if ( !decimalNoPrecision3.equals( decimalWithPrecision3 ) ) {
                Assert.fail(
                        "equal decimal data failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3
                                + ", decimalWithPrecision3 is :"
                                + decimalWithPrecision3 );
            }

            if ( decimalWithPrecision3.equals( intNumber ) ) {
                Assert.fail(
                        "equal decimal data failed, decimalWithPrecision3 is :"
                                + decimalWithPrecision3 + ", intNumber is :"
                                + intNumber );
            }

            if ( decimalWithPrecision2.equals( decimalWithPrecision3 ) ) {
                Assert.fail(
                        "equal decimal data failed, decimalWithPrecision2 is :"
                                + decimalWithPrecision2
                                + ", decimalWithPrecision3 is :"
                                + decimalWithPrecision3 );
            }

            if ( !decimalWithPrecision3.equals( decimalWithPrecision3 ) ) {
                Assert.fail(
                        "equal decimal data failed, decimalWithPrecision3 is :"
                                + decimalWithPrecision3
                                + ", decimalWithPrecision3 is :"
                                + decimalWithPrecision3 );
            }

            if ( decimalWithPrecision3.hashCode() != decimalWithPrecision3
                    .hashCode() ) {
                Assert.fail(
                        "decimal hashCode() failed, decimalWithPrecision3 is :"
                                + decimalWithPrecision3.hashCode()
                                + ",decimalWithPrecision3 is :"
                                + decimalWithPrecision3.hashCode() );
            }

            if ( decimalNoPrecision3.hashCode() != decimalWithPrecision3
                    .hashCode() ) {
                Assert.fail(
                        "decimal hashCode() failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3.hashCode()
                                + ",decimalWithPrecision3 is :"
                                + decimalWithPrecision3.hashCode() );
            }

            if ( decimalNoPrecision3.hashCode() != decimalNoPrecision3
                    .hashCode() ) {
                Assert.fail(
                        "decimal hashCode() failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3.hashCode()
                                + ", decimalNoPrecision3 is :"
                                + decimalNoPrecision3.hashCode() );
            }

            if ( decimalNoPrecision3.hashCode() == decimalNoPrecision1
                    .hashCode() ) {
                Assert.fail(
                        "decimal hashCode() failed, decimalNoPrecision3 is :"
                                + decimalNoPrecision3.hashCode()
                                + ", decimalNoPrecision1 is :"
                                + decimalNoPrecision1.hashCode() );
            }
        } catch ( IllegalArgumentException e ) {
            Assert.fail(
                    "generate decimal data failed, errMsg:" + e.getMessage() );
        }
    }
}
