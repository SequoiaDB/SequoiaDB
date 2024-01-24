package com.sequoiadb.jdbc.test;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Timestamp;
import java.util.Date;

public class jdbcDemo {
public static void main(String[] args) throws SQLException {
	Connection conn = null;
	try {
		Class.forName("com.sequoiadb.jdbc.Driver");
		System.out.println("成功加载sequoiadb驱动程序");
		String url = "jdbc:sequoiadb://192.168.20.45:11810";
		conn = DriverManager.getConnection(url);
		Statement stmt = conn.createStatement();
		ResultSet rs = stmt.executeQuery("select * from foo.bar where T9 = null");
		while(rs.next()){	
//		Double str1 = rs.getDouble("T1");
		Double str2 = rs.getDouble(1);
		int str3 = rs.getInt(3);
		System.out.println(str2+" "+str3);
//		Double str4 = rs.getDouble("T4");
//		Date str15 = rs.getDate("T15");
//		String str16 = rs.getString("T16");
//		String str18 = rs.getString("T18");
//		String str19 = rs.getString("T19");
//		int str20 = rs.getInt("T20");
//		String str21 = rs.getString("T21");
//		System.out.println(" "+str2+" "+str3+" "+str4+" "+str16+"  "+str15+"  "+str18+" "+str21+"  "+str19+"  "+str20);
		}
	} catch (Exception e) {
		e.printStackTrace();
	}
}
}
