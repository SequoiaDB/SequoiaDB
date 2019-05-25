package org.springframework.data.sequoiadb.core;

import org.bson.BSONObject;
import org.bson.types.ObjectId;

import org.springframework.core.convert.converter.Converter;



public class PersonReadConverter implements Converter<BSONObject, Person> {

	public Person convert(BSONObject source) {
		Person p = new Person((ObjectId) source.get("_id"), (String) source.get("name"));
		p.setAge((Integer) source.get("age"));
		return p;
	}

}
