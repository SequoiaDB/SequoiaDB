package com.sequoiadb.core;

import java.io.Serializable;

import org.bson.BSONObject;

import backtype.storm.tuple.Tuple;

public abstract class StormSequoiaObjectGrabber implements Serializable {

	/**
	 * 
	 */
	private static final long serialVersionUID = -6771356408498025247L;
	
	
	public abstract BSONObject map(BSONObject object, Tuple tuple);
}
