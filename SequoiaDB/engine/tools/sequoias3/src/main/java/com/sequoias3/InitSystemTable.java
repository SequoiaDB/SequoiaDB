package com.sequoias3;

import com.sequoias3.dao.RegionDao;
import com.sequoias3.dao.sequoiadb.SdbInitSystem;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.ApplicationArguments;
import org.springframework.boot.ApplicationRunner;
import org.springframework.core.annotation.Order;
import org.springframework.stereotype.Component;

@Component
@Order(1)
public class InitSystemTable implements ApplicationRunner {
    @Autowired
    SdbInitSystem sdbInitSystem;

    @Autowired
    RegionDao regionDao;

    @Override
    public void run(ApplicationArguments applicationArguments) throws Exception {
        // is cs exist and create cs
        sdbInitSystem.createSystemCS();

        // is user exist and create user
        sdbInitSystem.createUserCL();

        // is bucketlist exist and create bucketlist
        sdbInitSystem.createBucketCL();

        // is regionlist exist and create regionlist
        sdbInitSystem.createRegionCL();

        // is regionspacelist exist and create regionlist
        sdbInitSystem.createRegionSpaceCL();

        sdbInitSystem.createIDGeneratorCL();

        sdbInitSystem.createTaskCL();

        sdbInitSystem.createUploadCL();

        sdbInitSystem.createPartCL();

        sdbInitSystem.createACLTable();

        regionDao.createMetaCSCL(null, regionDao.getMetaCurCSName(null), regionDao.getMetaCurCLName(null), false);
        regionDao.createMetaCSCL(null, regionDao.getMetaHisCSName(null), regionDao.getMetaHisCLName(null), true);
        regionDao.createDirCSCL(null, regionDao.getMetaCurCSName(null));
    }
}
