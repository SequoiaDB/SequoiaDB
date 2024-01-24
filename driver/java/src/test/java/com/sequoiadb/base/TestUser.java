package com.sequoiadb.base;

import com.sequoiadb.exception.BaseException;
import com.sequoiadb.exception.SDBError;
import com.sequoiadb.test.SingleCSCLTestCase;
import org.junit.Assert;
import org.junit.Test;

import java.io.UnsupportedEncodingException;

import static org.junit.Assert.fail;

public class TestUser extends SingleCSCLTestCase {
    @Test
    public void testCreateAndRemoveUser() {
        String user = "admin";
        String password = "admin";
        try {
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        } catch (BaseException e) {
            if (e.getErrorCode() != SDBError.SDB_RTN_COORD_ONLY.getErrorCode()) {
                fail(e.toString());
            }
        }
    }

    @Test
    public void testUserWithChinese(){

        // case 1: UTF-8
        try {
            String user = "用户";
            String password = new String("密码".getBytes("UTF-8"));
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        }catch (BaseException | UnsupportedEncodingException e){
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }

        // case 2: GBK
        try {
            String user = "用户";
            String password = new String("密码".getBytes("GBK"));
            sdb.createUser(user, password);
            sdb.removeUser(user, password);
        }catch (BaseException | UnsupportedEncodingException e){
            throw new BaseException(SDBError.SDB_INVALIDARG, e);
        }
    }

    @Test
    public void testCreateUserOutOfRange() {
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = 0; i < 257; i++) {
            stringBuilder.append("1");
        }
        String str = "test";
        String outRange = stringBuilder.toString();
        // case1: username out of range
        try {
            sdb.createUser(outRange, str);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }

        // case2: passwd out of range
        try {
            sdb.createUser(str, outRange);
            Assert.fail();
        } catch (BaseException e) {
            Assert.assertEquals(e.getErrorCode(), SDBError.SDB_INVALIDARG.getErrorCode());
        }
    }

    @Test
    public void testCreateUserWithMaxSize() {
        StringBuilder stringBuilder = new StringBuilder();
        for (int i = 0; i < 256; i++) {
            stringBuilder.append("1");
        }
        String maxLenStr = stringBuilder.toString();
        
        try {
            sdb.createUser(maxLenStr, maxLenStr);
        } catch (BaseException e) {
            throw new BaseException(e.getErrorCode(), "Create user fail with max length");
        } finally {
            sdb.removeUser(maxLenStr, maxLenStr);
        }
    }
}
