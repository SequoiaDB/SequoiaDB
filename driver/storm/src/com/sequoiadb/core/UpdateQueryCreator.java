package com.sequoiadb.core;

import java.io.Serializable;

import org.bson.BSONObject;

import backtype.storm.tuple.Tuple;

public abstract class UpdateQueryCreator implements Serializable {
	public abstract BSONObject createQuery(Tuple tuple);
}
