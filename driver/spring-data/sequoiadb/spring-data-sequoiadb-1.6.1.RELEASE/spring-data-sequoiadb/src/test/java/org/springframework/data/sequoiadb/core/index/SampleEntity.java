package org.springframework.data.sequoiadb.core.index;

import org.springframework.data.annotation.Id;
import org.springframework.data.sequoiadb.core.mapping.Document;

@Document
public class SampleEntity {

	@Id
	String id;

	@Indexed
	String prop;
}
