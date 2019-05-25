package com.sequoiadb.core;

import java.io.Serializable;
import java.util.List;

import org.bson.BSONObject;

public abstract class SequoiaObjectGrabber implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1384658504027624352L;

	public abstract List<Object> map(BSONObject object);
	
	public abstract String[] fields();
}
