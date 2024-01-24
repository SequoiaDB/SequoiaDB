package com.sequoiadb.test.common;

import org.bson.types.BSONDecimal;
import org.bson.types.BSONTimestamp;
import org.bson.types.ObjectId;

import java.math.BigDecimal;
import java.util.ArrayList;
import java.util.List;


public class DecimalTmpA {

    private int fieldA;
    private String fieldB;
    private ObjectId fieldC;
    private BSONTimestamp fieldD;
    private BSONDecimal fieldE;
    private List<Integer> fieldF = new ArrayList<Integer>();
    private BigDecimal fieldZ;

    public DecimalTmpA() {
        this.fieldA = 10;
        this.fieldB = "10";
        this.fieldC = new ObjectId();
        this.fieldD = new BSONTimestamp(1000, 1000);
        this.fieldE = new BSONDecimal("10.0", 10, 1);
        this.fieldF = new ArrayList<Integer>();
        this.fieldF.add(10);
        this.fieldF.add(20);
    }

    public List<Integer> getFieldF() {
        return fieldF;
    }

    public void setFieldF(List<Integer> fieldF) {
        this.fieldF = fieldF;
    }

    public BSONTimestamp getFieldD() {
        return fieldD;
    }

    public void setFieldD(BSONTimestamp fieldD) {
        this.fieldD = fieldD;
    }

    public BSONDecimal getFieldE() {
        return fieldE;
    }

    public void setFieldE(BSONDecimal fieldE) {
        this.fieldE = fieldE;
    }

    public int getFieldA() {
        return fieldA;
    }

    public void setFieldA(int fieldA) {
        this.fieldA = fieldA;
    }

    public String getFieldB() {
        return fieldB;
    }

    public void setFieldB(String fieldB) {
        this.fieldB = fieldB;
    }

    public ObjectId getFieldC() {
        return fieldC;
    }

    public void setFieldC(ObjectId fieldC) {
        this.fieldC = fieldC;
    }

    /**
     * @return the fieldZ
     */
    public BigDecimal getFieldZ() {
        return fieldZ;
    }

    /**
     * @param fieldZ the fieldZ to set
     */
    public void setFieldZ(BigDecimal fieldZ) {
        this.fieldZ = fieldZ;
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "DecimalTmpA [fieldZ=" + fieldZ + ", fieldA=" + fieldA
            + ", fieldB=" + fieldB + ", fieldC=" + fieldC + ", fieldD="
            + fieldD + ", fieldE=" + fieldE + ", fieldF=" + fieldF + "]";
    }
}