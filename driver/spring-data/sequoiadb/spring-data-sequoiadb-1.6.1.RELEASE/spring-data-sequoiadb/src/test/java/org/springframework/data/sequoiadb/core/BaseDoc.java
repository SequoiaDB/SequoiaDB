package org.springframework.data.sequoiadb.core;

import org.springframework.data.annotation.Id;

public class BaseDoc {
	@Id String id;
	String value;
}
