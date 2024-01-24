package com.sequoiadb.testdata;

import org.bson.types.BSONDecimal;

import java.math.BigDecimal;

public class DecimalBean {
    BSONDecimal bsonDecimal;
    BigDecimal bigDecimal;

    /**
     * @return the bsonDecimal
     */
    public BSONDecimal getBsonDecimal() {
        return bsonDecimal;
    }

    /**
     * @return the bigDecimal
     */
    public BigDecimal getBigDecimal() {
        return bigDecimal;
    }

    /**
     * @param bsonDecimal the bsonDecimal to set
     */
    public void setBsonDecimal(BSONDecimal bsonDecimal) {
        this.bsonDecimal = bsonDecimal;
    }

    /**
     * @param bigDecimal the bigDecimal to set
     */
    public void setBigDecimal(BigDecimal bigDecimal) {
        this.bigDecimal = bigDecimal;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "DecimalBean [bsonDecimal=" + bsonDecimal + ", bigDecimal="
            + bigDecimal + "]";
    }

}
