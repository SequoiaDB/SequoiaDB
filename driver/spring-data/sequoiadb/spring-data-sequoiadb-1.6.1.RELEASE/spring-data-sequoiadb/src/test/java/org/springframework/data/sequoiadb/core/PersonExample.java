/*
 * Copyright 2002-2013 the original author or authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.springframework.data.sequoiadb.core;

import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;
import org.springframework.context.support.AbstractApplicationContext;

/**


 */
public class PersonExample {

	private static final Logger LOGGER = LoggerFactory.getLogger(PersonExample.class);

	@Autowired private SequoiadbOperations sdbOps;

	public static void main(String[] args) {
		AbstractApplicationContext applicationContext = new AnnotationConfigApplicationContext(PersonExampleAppConfig.class);
		PersonExample example = applicationContext.getBean(PersonExample.class);
		example.doWork();
		applicationContext.close();
	}

	public void doWork() {
		sdbOps.dropCollection("personexample");

		PersonWithIdPropertyOfTypeString p = new PersonWithIdPropertyOfTypeString();
		p.setFirstName("Sven");
		p.setAge(22);

		sdbOps.save(p);

		PersonWithIdPropertyOfTypeString p2 = new PersonWithIdPropertyOfTypeString();
		p2.setFirstName("Jon");
		p2.setAge(23);

		sdbOps.save(p2);

		LOGGER.debug("Saved: " + p);

		p = sdbOps.findById(p.getId(), PersonWithIdPropertyOfTypeString.class);

		LOGGER.debug("Found: " + p);



		p = sdbOps.findById(p.getId(), PersonWithIdPropertyOfTypeString.class);
		LOGGER.debug("Updated: " + p);

		List<PersonWithIdPropertyOfTypeString> folks = sdbOps.findAll(PersonWithIdPropertyOfTypeString.class);
		LOGGER.debug("Querying for all people...");
		for (PersonWithIdPropertyOfTypeString element : folks) {
			LOGGER.debug(element.toString());
		}


		sdbOps.remove(p);

		List<PersonWithIdPropertyOfTypeString> people = sdbOps.findAll(PersonWithIdPropertyOfTypeString.class);

		LOGGER.debug("Number of people = : " + people.size());

	}

	public void doWork2() {
		sdbOps.dropCollection("personexample");

		PersonWithIdPropertyOfTypeString p = new PersonWithIdPropertyOfTypeString();
		p.setFirstName("Sven");
		p.setAge(22);

	}

}
