package com.sequoiadb.metadata;

import java.util.ArrayList;

import org.bson.BSONObject;
import org.bson.types.BasicBSONList;
import org.testng.Assert;

import com.sequoiadb.base.DBCursor;
import com.sequoiadb.base.Sequoiadb;
import com.sequoiadb.exception.BaseException;

public class CommLib {
    public boolean isStandAlone( Sequoiadb sdb ) {
        try {
            sdb.listReplicaGroups();
        } catch ( BaseException e ) {
            if ( e.getErrorCode() == -159 ) {
                System.out.printf( "run mode is standalone" );
                return true;
            }
        }
        return false;
    }

    public boolean OneGroupMode( Sequoiadb sdb ) {
        if ( getDataGroups( sdb ).size() < 2 ) {
            System.out.printf( "only one group" );
            return true;
        }
        return false;
    }

    public ArrayList< String > getDataGroups( Sequoiadb sdb ) {
        ArrayList< String > dataGroups = new ArrayList< String >();
        try {
            dataGroups = sdb.getReplicaGroupNames();
            dataGroups.remove( "SYSCatalogGroup" );
            dataGroups.remove( "SYSCoord" );
        } catch ( BaseException e ) {
            Assert.fail( "getDataGroups failed" + e.getMessage() );
        }
        return dataGroups;
    }

    public void dropDomainForClearEnv( Sequoiadb sdb, String domainName ) {
        if ( sdb.isDomainExist( domainName ) ) {
            DBCursor csInDomain = sdb.getDomain( domainName ).listCSInDomain();
            while ( csInDomain.hasNext() ) {
                BSONObject csName = csInDomain.getNext();
                sdb.dropCollectionSpace( ( String ) csName.get( "Name" ) );
            }
            csInDomain.close();
            sdb.dropDomain( domainName );
        }
    }

    public void createDomain( Sequoiadb sdb, String domainName,
            BSONObject domainOption ) {
        try {
            sdb.createDomain( domainName, domainOption );
        } catch ( BaseException e ) {
            Assert.fail( "create domain failed, errMsg:" + e.getMessage() );
        }
    }

    public void checkDomainInfo( Sequoiadb sdb, BSONObject matcher,
            BSONObject selector, BSONObject orderBy, BSONObject hint,
            ArrayList< String > expectDomainGroups, String expectDomainName,
            boolean expectAutoSplit ) {
        DBCursor dbCursor = sdb.listDomains( matcher, selector, orderBy, hint );
        while ( dbCursor.hasNext() ) {
            ArrayList< String > domainGroupArrs = new ArrayList< String >();
            BSONObject record = dbCursor.getNext();
            BasicBSONList groupArr = ( BasicBSONList ) record.get( "Groups" );
            for ( int i = 0; i < groupArr.toArray().length; i++ ) {
                BSONObject groupObj = ( BSONObject ) groupArr.toArray()[ i ];
                String groupName = ( String ) groupObj.get( "GroupName" );
                domainGroupArrs.add( groupName );
            }

            // check group name
            Assert.assertEqualsNoOrder( domainGroupArrs.toArray(),
                    expectDomainGroups.toArray() );
            // Assert.assertEquals(domainGroupArrs, expectDomainGroups);

            // check domain name
            String domainName = ( String ) record.get( "Name" );
            Assert.assertEquals( domainName, expectDomainName );

            // check domain autosplit
            boolean autoSplit = ( boolean ) record.get( "AutoSplit" );
            Assert.assertEquals( autoSplit, expectAutoSplit );
        }
        dbCursor.close();
    }

    public void alterDomain( Sequoiadb sdb, String domainName,
            BSONObject domainOption ) {
        try {
            sdb.getDomain( domainName ).alterDomain( domainOption );
        } catch ( BaseException e ) {
            Assert.fail( "alter domain failed, errMsg:" + e.getMessage() );
        }
    }

    public void dropDomain( Sequoiadb sdb, String domainName ) {
        try {
            sdb.dropDomain( domainName );
        } catch ( BaseException e ) {
            Assert.fail( "drop domain failed, errMsg:" + e.getMessage() );
        }
    }
}
