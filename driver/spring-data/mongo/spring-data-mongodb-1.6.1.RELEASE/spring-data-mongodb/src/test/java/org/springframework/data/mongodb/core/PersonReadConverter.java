package org.springframework.data.mongodb.core;

import org.bson.types.ObjectId;

import org.springframework.core.convert.converter.Converter;

import org.springframework.data.mongodb.assist.DBObject;

public class PersonReadConverter implements Converter<DBObject, Person> {

	public Person convert(DBObject source) {
		Person p = new Person((ObjectId) source.get("_id"), (String) source.get("name"));
		p.setAge((Integer) source.get("age"));
		return p;
	}

}
