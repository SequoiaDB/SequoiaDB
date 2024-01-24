package com.sequoiadb.om.plugin.dao;

public class MySQLOperations extends SequoiaSQLOperations {

    MySQLOperations() throws ClassNotFoundException {
        className = "com.mysql.jdbc.Driver";
        scheme = "jdbc:mysql";
        defaultDBName = "mysql";
        defaultUser = "root";
    }
}
