package com.sequoiadb.jdbc;

public class NotImplemented extends java.sql.SQLException{

	private static final long serialVersionUID = 9125162314742072927L;

	public NotImplemented(){
		super("Feature not implemented.");
	}
}
