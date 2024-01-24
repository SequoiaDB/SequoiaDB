package com.sequoiadb.datasource;

import org.testng.annotations.DataProvider;


public class SdbTestOptionFactory {
    @DataProvider(name = "option-provider")
    public static Object[][] create() {
        return new Object[][] {
                { createSequoiadbOption( ConnectStrategy.LOCAL ) },
                { createSequoiadbOption( ConnectStrategy.SERIAL ) },
                { createSequoiadbOption( ConnectStrategy.RANDOM ) },
                { setDisablesyncCoord(
                        createSequoiadbOption( ConnectStrategy.BALANCE ) ) },
                { setsyncCoordInterval(
                        createSequoiadbOption( ConnectStrategy.BALANCE ) ) } };
    }

    private static DatasourceOptions setsyncCoordInterval(
            DatasourceOptions option ) {
        option.setSyncCoordInterval( 100 );
        return option;
    }

    private static DatasourceOptions setDisablesyncCoord(
            DatasourceOptions option ) {
        option.setSyncCoordInterval( 0 );
        return option;
    }

    private static DatasourceOptions setValidateConnection(
            DatasourceOptions option ) {
        option.setValidateConnection( true );
        return option;
    }

    private static DatasourceOptions createSequoiadbOption(
            ConnectStrategy strategy ) {
        DatasourceOptions option = new DatasourceOptions();
        option.setConnectStrategy( strategy );
        return option;
    }

}
