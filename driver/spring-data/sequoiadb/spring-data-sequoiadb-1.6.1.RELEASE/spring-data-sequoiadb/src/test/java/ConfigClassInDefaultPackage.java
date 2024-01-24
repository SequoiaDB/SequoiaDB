/*
 * Copyright 2014 the original author or authors.
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
import org.springframework.context.annotation.Configuration;
import org.springframework.data.sequoiadb.assist.SdbClient;
import org.springframework.data.sequoiadb.config.AbstractSequoiadbConfiguration;

import org.springframework.data.sequoiadb.assist.Sdb;

/**
 * Sample configuration class in default package.
 * 
 * @see DATA_JIRA-877

 */
@Configuration
public class ConfigClassInDefaultPackage extends AbstractSequoiadbConfiguration {

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.config.AbstractSequoiadbConfiguration#getDatabaseName()
	 */
	@Override
	protected String getDatabaseName() {
		return "default";
	}

	/* 
	 * (non-Javadoc)
	 * @see org.springframework.data.sequoiadb.config.AbstractSequoiadbConfiguration#sdb()
	 */
	@Override
	public Sdb sdb() throws Exception {
		return new SdbClient();
	}
}
