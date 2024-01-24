package com.sequoiadb.testdata;

public class NestedClassA {
    private BasicClass basicObjA = new BasicClass();
    private BasicClass basicObjB = new BasicClass();
    private BasicClass basicObjC = null;

    public BasicClass getBasicObjA() {
        return basicObjA;
    }

    public void setBasicObjA(BasicClass value) {
        basicObjA = value;
    }

    public BasicClass getBasicObjB() {
        return basicObjB;
    }

    public void setBasicObjB(BasicClass value) {
        basicObjB = value;
    }

    public BasicClass getBasicObjC() {
        return basicObjC;
    }

    public void setBasicObjC(BasicClass value) {
        basicObjC = value;
    }

    @Override
    public boolean equals(Object otherObj) {
        if (otherObj == null || !(otherObj instanceof NestedClassA)) {
            return false;
        }

        NestedClassA other = (NestedClassA) otherObj;
        if ((this.basicObjA == null && other.basicObjA != null) ||
            (this.basicObjA != null && !this.basicObjA.equals(other.basicObjA))) {
            return false;
        }

        if ((this.basicObjB == null && other.basicObjB != null) ||
            (this.basicObjB != null && !this.basicObjB.equals(other.basicObjB))) {
            return false;
        }

        if ((this.basicObjC == null && other.basicObjC != null) ||
            (this.basicObjC != null && !this.basicObjC.equals(other.basicObjC))) {
            return false;
        }

        return true;

    }
}