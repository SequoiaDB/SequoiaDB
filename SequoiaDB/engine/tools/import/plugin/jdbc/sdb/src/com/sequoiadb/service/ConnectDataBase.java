package com.sequoiadb.service;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;

import org.apache.log4j.Logger;

public class ConnectDataBase {

	
	String driver = null;

	String url = null;

	String user = null;

	String password = null;

	Connection conn = null;

	Logger logger = Logger.getLogger(ConnectDataBase.class);

	public void dbDriver(String dbType, String url, String user, String password) {

		if (dbType != null && dbType == "mysql") {
			driver = "com.mysql.jdbc.Driver";
			this.url = url;
			this.user = user;
			this.password = password;
		}
		if (dbType != null && dbType == "sqlserver") {
			driver = "com.microsoft.jdbc.sqlserver.SQLServerDriver";
			this.url = url;
			this.user = user;
			this.password = password;
		}
		if (dbType != null && dbType == "db2") {
			driver = "com.ibm.db2.jcc.DB2Driver";
			this.url = url;
			this.user = user;
			this.password = password;
		}
		if (dbType != null && dbType == "oracle") {
			driver = "oracle.jdbc.driver.OracleDriver";
			this.url = url;
			this.user = user;
			this.password = password;
		}
	}

	public ConnectDataBase(String dbType, String url, String user, String password) {
		try {
			dbDriver(dbType, url, user, password);

			Class.forName(driver); //load driver

			conn = DriverManager.getConnection(url, user, password); // connect db

		} catch (ClassNotFoundException e) {
			logger.error(e.getMessage());
			e.printStackTrace();
		} catch (SQLException e) {
			logger.error(e.getMessage());
			e.printStackTrace();
		}
	}

	public Connection getConnection() {
		return this.conn;
	}
}
