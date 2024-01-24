package com.sequoiadb.core;

import java.io.Serializable;
import java.util.List;

import org.bson.BSONObject;

//The mapper class for maps the bson object to tuple list
public abstract class SequoiaObjectGrabber implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -1384658504027624352L;

	//maps the bson object to List
	public abstract List<Object> map(BSONObject object);
	
	//Get the tuple's fileds name list.
	public abstract String[] fields();
}
