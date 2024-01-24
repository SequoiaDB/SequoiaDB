package org.springframework.data.mongodb.core;

import org.springframework.core.convert.converter.Converter;

import org.springframework.data.mongodb.assist.BasicDBObject;
import org.springframework.data.mongodb.assist.DBObject;

public class PersonWriteConverter implements Converter<Person, DBObject> {

	public DBObject convert(Person source) {
		DBObject dbo = new BasicDBObject();
		dbo.put("_id", source.getId());
		dbo.put("name", source.getFirstName());
		dbo.put("age", source.getAge());
		return dbo;
	}

}
