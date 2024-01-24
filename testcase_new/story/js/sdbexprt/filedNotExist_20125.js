/*******************************************************************
* @Description : test export with --fields 
*                seqDB-20125: --fields (--fileds有足够多的字段) 不在记录中
* @author      : csq
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt20125";
var exprtfileds = "a,b,c,d,dddddddddddddddddddddd,SEQ_NO,INTERNAL_KEY,TRAN_DATE,SOURCE_TYPE,TERMINAL_ID,BRANCH,OFFICER_ID,TRAN_TYPE,EFFECT_DATE,POST_DATE,REVERSAL_TRAN_TYPE,REVERSAL_DATE,PBK_UPD_FLAG,STMT_DATE,TRAN_AMT,REFERENCE_TYPE,REFERENCE,REFERENCE_BANK,FLOAT_DAYS,PREVIOUS_BAL_AMT,ACTUAL_BAL_AMT,TFR_INTERNAL_KEY,TFR_SEQ_NO,TRACE_ID,CR_DR_MAINT_IND,OVERRIDE_OFFICER,REFERENCE_BRANCH,TRAN_DESC,TIME_STAMP,BR_SEQ_NO,CD_CASH_FLOW_FLAG,VALUE_DATE,CCY,PROFIT_CENTRE,SOURCE_MODULE,BT_OFFICER_ID,BT_OVERRIDE_ID,RECEIPT_NO,PRIMARY_SEQ_NO,RATE_TYPE,PURPOSE_CODE,SPEC_CODE,OTH_BANK_CODE,OTH_ACCT_NO,OTH_ACCT_DESC,OTH_ACCT_DESC2,OTH_REFERENCE,OTH_SPEC_CODE,OTH_VALUE_DATE,MSG_CLIENT,TRAN_REF_NO,CLIENT_CROSS_REF,TRN,SOURCE_REF_NO,OTH_AMT,SOURCE_REF_TYPE,SERV_CHARGE,CONS_SEQ_NO,SORT_PRIORITY,APPROVAL_DATE,FORMAT,LAST_PROCESS_DATE,LAST_PROCESS_ERR,MSG_BANK,ORIG_SETTLEMENT_DATE,ORIG_TRAN_REF_NO,STATUS_INFO,NARRATIVE,TRAN_DATE_TIME,FH_SEQ_NO,TRAN_ENTRY_DATE,INWD_SVC_SEQ,SESSION_ID,APPROVING_OFFICER,OVERRIDE_APPROVING_OFFICER,PREFIX,SUB_STATUS,OUR_CORRESPONDENT,ACCOUNT_BANK_CODE,OTH_CORRESPONDENT,OTH_ACCT_BANK_CODE,ANSWERING_BANK_NO,ANSWERING_BANK_CORRESPONDENT,INITIAL_TRAN_REF_NO,ACCOUNT_BRANCH,NOSTRO_BRANCH,BORROWING_LENDING_RATE,TENOR,MSG_CODE_OUR,MSG_CODE_OTHER,VERIFICATION_OFFICER,VERIFICATION_DATE,GL_TYPE,ORIG_CMT_CODE,BANK_CODE,ACCT_NO,ACCT_DESC,TRAN_CATEGORY,MESSAGE_FOR_INTRA_BANK,ORIG_ACCT_DESC,BANK_SEQ_NO,D2_REFERENCE,OTH_ACCT_TYPE,BAL_TYPE,BASE_ACCT_NO,OTH_BASE_ACCT_NO,OTH_ACCT_CCY,AUTO_DR_OTH_BAL_TYPE,BUSINESS_UNIT,CHEQUE_TYPE,CASH_ITEM,TRAN_NOTE,PRINT_CNT,PAYER,OTH_BANK_NAME,TRAN_STATUS,ENTER_FLAG,SUB_INTERNAL_KEY,SUB_ACCT_NO,CARD_NO,TOTAL_AUTH_OD";

main( test );

function test ()
{
   commDropCL( db, csname, clname );
   var cl = commCreateCL( db, csname, clname );
   testExprt20125_1( cl );
   testExprt20125_2( cl );
   testExprt20125_3( cl );
   commDropCL( db, csname, clname );
}

function testExprt20125_1 ( cl )
{
   var docs = [{ fff: 123 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt20125.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields " + exprtfileds;
   testRunCommand( command );

   var content = exprtfileds + "\n,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
}

function testExprt20125_2 ( cl )
{
   var docs = [{ a: 1, b: 2, c: 3, d: 4, dddddddddddddddddddddd: 5, SEQ_NO: 6, INTERNAL_KEY: 7, TRAN_DATE: 8, SOURCE_TYPE: 9, TERMINAL_ID: 10, BRANCH: 10, OFFICER_ID: 10, TRAN_TYPE: 10, EFFECT_DATE: 10, POST_DATE: 10, REVERSAL_TRAN_TYPE: 10, REVERSAL_DATE: 10, PBK_UPD_FLAG: 10, STMT_DATE: 10, TRAN_AMT: 10, REFERENCE_TYPE: 10, REFERENCE: 10, REFERENCE_BANK: 10, FLOAT_DAYS: 10, PREVIOUS_BAL_AMT: 10, ACTUAL_BAL_AMT: 10, TFR_INTERNAL_KEY: 10, TFR_SEQ_NO: 10, TRACE_ID: 10, CR_DR_MAINT_IND: 10, OVERRIDE_OFFICER: 10, REFERENCE_BRANCH: 10, TRAN_DESC: 10, TIME_STAMP: 10, BR_SEQ_NO: 10, CD_CASH_FLOW_FLAG: 10, VALUE_DATE: 10, CCY: 10, PROFIT_CENTRE: 10, SOURCE_MODULE: 10, BT_OFFICER_ID: 10, BT_OVERRIDE_ID: 10, RECEIPT_NO: 10, PRIMARY_SEQ_NO: 10, RATE_TYPE: 10, PURPOSE_CODE: 10, SPEC_CODE: 10, OTH_BANK_CODE: 10, OTH_ACCT_NO: 10, OTH_ACCT_DESC: 10, OTH_ACCT_DESC2: 10, OTH_REFERENCE: 10, OTH_SPEC_CODE: 10, OTH_VALUE_DATE: 10, MSG_CLIENT: 10, TRAN_REF_NO: 10, CLIENT_CROSS_REF: 10, TRN: 10, SOURCE_REF_NO: 10, OTH_AMT: 10, SOURCE_REF_TYPE: 10, SERV_CHARGE: 10, CONS_SEQ_NO: 10, SORT_PRIORITY: 10, APPROVAL_DATE: 10, FORMAT: 10, LAST_PROCESS_DATE: 10, LAST_PROCESS_ERR: 10, MSG_BANK: 10, ORIG_SETTLEMENT_DATE: 10, ORIG_TRAN_REF_NO: 10, STATUS_INFO: 10, NARRATIVE: 10, TRAN_DATE_TIME: 10, FH_SEQ_NO: 10, TRAN_ENTRY_DATE: 10, INWD_SVC_SEQ: 10, SESSION_ID: 10, APPROVING_OFFICER: 10, OVERRIDE_APPROVING_OFFICER: 10, PREFIX: 10, SUB_STATUS: 10, OUR_CORRESPONDENT: 10, ACCOUNT_BANK_CODE: 10, OTH_CORRESPONDENT: 10, OTH_ACCT_BANK_CODE: 10, ANSWERING_BANK_NO: 10, ANSWERING_BANK_CORRESPONDENT: 10, INITIAL_TRAN_REF_NO: 10, ACCOUNT_BRANCH: 10, NOSTRO_BRANCH: 10, BORROWING_LENDING_RATE: 10, TENOR: 10, MSG_CODE_OUR: 10, MSG_CODE_OTHER: 10, VERIFICATION_OFFICER: 10, VERIFICATION_DATE: 10, GL_TYPE: 10, ORIG_CMT_CODE: 10, BANK_CODE: 10, ACCT_NO: 10, ACCT_DESC: 10, TRAN_CATEGORY: 10, MESSAGE_FOR_INTRA_BANK: 10, ORIG_ACCT_DESC: 10, BANK_SEQ_NO: 10, D2_REFERENCE: 10, OTH_ACCT_TYPE: 10, BAL_TYPE: 10, BASE_ACCT_NO: 10, OTH_BASE_ACCT_NO: 10, OTH_ACCT_CCY: 10, AUTO_DR_OTH_BAL_TYPE: 10, BUSINESS_UNIT: 10, CHEQUE_TYPE: 10, CASH_ITEM: 10, TRAN_NOTE: 10, PRINT_CNT: 10, PAYER: 10, OTH_BANK_NAME: 10, TRAN_STATUS: 10, ENTER_FLAG: 10, SUB_INTERNAL_KEY: 10, SUB_ACCT_NO: 10, CARD_NO: 10, TOTAL_AUTH_OD: 10 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt20125.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields " + exprtfileds;
   testRunCommand( command );

   var content = exprtfileds + "\n1,2,3,4,5,6,7,8,9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
}

function testExprt20125_3 ( cl )
{
   var docs = [{ a: 1, b: 2, c: 3 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt20125.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields " + exprtfileds;
   testRunCommand( command );

   var content = exprtfileds + "\n1,2,3,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
}