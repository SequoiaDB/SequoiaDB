/**
 *
 */
package com.sequoiadb.testdata;

/**
 * @author qiushanggao
 */
public class NotWriteMethodClass {
    public NotWriteMethodClass() {
        System.out.println();
    }

    private long value = 0;

    public long getValue() {
        return value;
    }

    public void setValue(int value) {
        this.value = value;
    }
}
