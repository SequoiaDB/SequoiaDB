/**
 * Copyright (c) 2020, SequoiaDB Ltd.
 * File Name:IChecker.java
 * 类的详细描述
 *
 *  @author 类创建者姓名
 * Date:2020年7月1日下午1:41:13
 *  @version 1.00
 */
package com.sequoiadb.testcommon;

import com.sequoiadb.base.Sequoiadb;

public interface IChecker {
    public boolean check(Sequoiadb db) ;
    public String getName();
}
