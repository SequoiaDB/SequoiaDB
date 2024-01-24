package com.sequoiadb.samples;

/******************************************************************************
 *
 * Name: Sql.java
 * Description: This program demonstrates how to use the Java Driver to
 *				manipulate DB by SQL 
 *				Get more details in API document
 * 
 * ****************************************************************************/

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class Sql {
	public static void main(String[] args) {
		if (args.length != 1) {
			if (args.length != 1) {
				System.out
						.println("Please give the database server address <IP:Port>");
				System.exit(0);
			}
		}
		// the database server address
		String connString = args[0];
		Sequoiadb sdb = null;

		try {
			sdb = new Sequoiadb(connString, "", "");
		} catch (BaseException e) {
			System.out.println("Failed to connect to database: " + connString
					+ ", error description" + e.getErrorType());
			e.printStackTrace();
			System.exit(1);
		}
		try {
			String csFullName = Constants.SQL_CS_NAME + "." + Constants.SQL_CL_NAME;
			String sql = "";
			// create collectionspace
			sql = "create collectionspace " + Constants.SQL_CS_NAME;
			sdb.exec(sql);
			// create table
			sql = "create collection " + csFullName;
			sdb.execUpdate(sql);
			// insert data into table
			sql = "insert into " + csFullName + " (a,b,c)" + " values(1,\"John\",20)";
			sdb.execUpdate(sql);
			// select from table
			sql = "select * from " + csFullName;
			DBCursor cursor = cursor = sdb.exec(sql);
			try {
				while(cursor.hasNext())
					System.out.println(cursor.getNext());
			} finally {
				cursor.close();
			}
		} catch (BaseException e) {
			e.printStackTrace();
		}
	}

}
