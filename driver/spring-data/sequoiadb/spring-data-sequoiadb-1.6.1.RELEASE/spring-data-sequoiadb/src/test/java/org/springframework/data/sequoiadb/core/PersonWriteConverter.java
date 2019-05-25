package org.springframework.data.sequoiadb.core;

import org.bson.BSONObject;
import org.bson.BasicBSONObject;
import org.springframework.core.convert.converter.Converter;




public class PersonWriteConverter implements Converter<Person, BSONObject> {

	public BSONObject convert(Person source) {
		BSONObject dbo = new BasicBSONObject();
		dbo.put("_id", source.getId());
		dbo.put("name", source.getFirstName());
		dbo.put("age", source.getAge());
		return dbo;
	}

}
