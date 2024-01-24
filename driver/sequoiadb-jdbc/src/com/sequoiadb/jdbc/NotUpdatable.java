package com.sequoiadb.jdbc;

import java.sql.SQLException;

public class NotUpdatable extends SQLException{

	private static final long serialVersionUID = -3255542912991511196L;
   
	public NotUpdatable(){
		super("Result Set not updatable.");
	}

}
