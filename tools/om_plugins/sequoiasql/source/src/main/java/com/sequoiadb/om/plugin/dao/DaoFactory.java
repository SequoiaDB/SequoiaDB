package com.sequoiadb.om.plugin.dao;

import org.springframework.stereotype.Component;

import java.util.HashMap;
import java.util.Map;

@Component
public class DaoFactory {
    private Map<String, SequoiaSQLOperations> daoMap = new HashMap<String, SequoiaSQLOperations>();

    DaoFactory() throws ClassNotFoundException {
        daoMap.put("postgresql", new PostgreSQLOperations());
        daoMap.put("mysql", new MySQLOperations());
        daoMap.put("mariadb", new MariaDBOperations());
    }

    public SequoiaSQLOperations getDao(String name) {
        SequoiaSQLOperations dao = daoMap.get(name);

        if (dao == null) {
            dao = daoMap.get("postgresql");
        }

        return dao;
    }
}
