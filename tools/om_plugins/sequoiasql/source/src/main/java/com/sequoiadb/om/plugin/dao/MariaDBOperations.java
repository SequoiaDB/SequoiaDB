package com.sequoiadb.om.plugin.dao;

public class MariaDBOperations extends SequoiaSQLOperations {
    MariaDBOperations() throws ClassNotFoundException {
        className = "com.mysql.jdbc.Driver";
        scheme = "jdbc:mysql";
        defaultDBName = "mysql";
        defaultUser = "root";
    }
}
