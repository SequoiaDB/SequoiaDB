/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = pmdFTMgr.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/06/2020  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdFTMgr.hpp"
#include "pmdEnv.hpp"
#include "pmd.hpp"
#include "dpsLogWrapper.hpp"
#include "pd.hpp"

namespace engine
{

   /*
      Self Define
   */
   #define PMD_FT_CATCHUP_LSNDIFF_THRESHOLD           ( 4 * 1024 * 1024 )
   #define PMD_FT_SLOWNODE_N_WND_SZ                   ( 40 )

   #define PMD_FT_NOSPC_ADJ_MAX                       ( 30 )

   /*
      _ftSampleWindow implement
   */
   _ftSampleWindow::_ftSampleWindow()
   {
      _curPos = 0 ;

      for ( UINT32 i = 0 ; i < PMD_FT_SAMPLE_WINDOW_SZ ; ++i )
      {
         _window[ i ]._pos = i ;
      }
   }

   _ftSampleWindow::~_ftSampleWindow()
   {
   }

   void _ftSampleWindow::clean()
   {
      for ( UINT32 i = 0 ; i < PMD_FT_SAMPLE_WINDOW_SZ ; ++i )
      {
         _window[ i ].clean() ;
      }
   }

   ftSampleWndItem* _ftSampleWindow::slideForward( UINT64 dbTick )
   {
      ftSampleWndItem *pItem = current() ;
      UINT64 time = pmdGetDBTick() ;
      SDB_ASSERT( 0 != time, "Time can't be zero" ) ;

      if ( 0 == pItem->_time )
      {
         pItem->reset( dbTick ) ;
      }
      else
      {
         UINT32 pos = ( _curPos + 1 ) % PMD_FT_SAMPLE_WINDOW_SZ ;
         pItem = &_window[pos] ;
         pItem->reset( dbTick ) ;
         _curPos = pos ;
      }

      return pItem ;
   }

   BOOLEAN _ftSampleWindow::isEmpty( UINT32 curPos ) const
   {
      if ( 0 == curPos && 0 == _window[ curPos ]._time )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _ftSampleWindow::isEmpty() const
   {
      return isEmpty( _curPos ) ;
   }

   BOOLEAN _ftSampleWindow::isFull( UINT32 curPos ) const
   {
      UINT32 nextPos = ( curPos + 1 ) % PMD_FT_SAMPLE_WINDOW_SZ ;

      if ( 0 != _window[ curPos ]._time &&
           0 != _window[ nextPos ]._time )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   BOOLEAN _ftSampleWindow::isFull() const
   {
      return isFull( _curPos ) ;
   }

   UINT32 _ftSampleWindow::getCount() const
   {
      UINT32 curPos = _curPos ;

      if ( isEmpty( curPos ) )
      {
         return 0 ;
      }
      else if ( isFull( curPos ) )
      {
         return PMD_FT_SAMPLE_WINDOW_SZ ;
      }
      return curPos % PMD_FT_SAMPLE_WINDOW_SZ ;
   }

   ftSampleWndItem* _ftSampleWindow::current()
   {
      return &_window[ _curPos ] ;
   }

   ftSampleWndItem* _ftSampleWindow::first()
   {
      UINT32 curPos = _curPos ;
      UINT32 nextPos = ( curPos + 1 ) % PMD_FT_SAMPLE_WINDOW_SZ ;

      if ( 0 == _window[ curPos ]._time )
      {
         return &_window[ curPos ] ;
      }
      else if ( 0 == _window[ nextPos ]._time )
      {
         return &_window[ 0 ] ;
      }
      return &_window[ nextPos ] ;      
   }

   ftSampleWndItem* _ftSampleWindow::next( UINT32 curPos, UINT32 step )
   {
      UINT32 nextPos = ( curPos + step ) % PMD_FT_SAMPLE_WINDOW_SZ ;
      return &_window[ nextPos ] ;
   }

   ftSampleWndItem* _ftSampleWindow::prev( UINT32 curPos, UINT32 step )
   {
      step = step % PMD_FT_SAMPLE_WINDOW_SZ ;
      UINT32 prevPos = ( curPos + PMD_FT_SAMPLE_WINDOW_SZ - step ) % 
                       PMD_FT_SAMPLE_WINDOW_SZ ;
      return &_window[ prevPos ] ;
   }

   BOOLEAN _ftSampleWindow::reportErr( PMD_FT_ERR_TYPE err )
   {
      ftSampleWndItem *item = current() ;

      if ( item && err >= FT_ERR_NONE && err < FT_ERR_MAX )
      {
         item->_err[ err ].inc() ;
         return TRUE ;
      }

      return FALSE ;
   }

   BOOLEAN _ftSampleWindow::reportRisk( PMD_FT_RISK_TYPE risk )
   {
      ftSampleWndItem *item = current() ;

      if ( item && risk >= FT_RISK_NONE && risk <= FT_RISK_MAX )
      {
         item->_risk[ risk ].inc() ;
         return TRUE ;
      }

      return FALSE ;
   }

   /*
      _pmdFTMgr implement
   */
   _pmdFTMgr::_pmdFTMgr()
   {
      _lastSampleTick = 0 ;
      _lastSucCount = 0 ;
      _confirmPeriod = PMD_FT_CACL_INTERVAL_DFT ;
      _ftMask = 0 ;
      _confirmRatio = PMD_FT_CACL_RATIO_DFT ;
      _isCatchup = TRUE ;
      _confirmedStat = 0 ;
      _indoubtErr = SDB_OK ;
      _ftLevel = FT_LEVEL_SEMI ;

      _slowNodeThreshold = 0 ;
      _slowNodeIncrement = 0 ;
   }

   _pmdFTMgr::~_pmdFTMgr()
   {
   }

   INT32 _pmdFTMgr::init( UINT32 ftmask,
                          UINT32 confirmPeriod,
                          UINT32 confirmRatio,
                          INT32 ftLevel )
   {
      setMask( ftmask ) ;
      setConfirmPeriod( confirmPeriod ) ;
      setConfrimRatio( confirmRatio ) ;
      setFTLevel( ftLevel ) ;

      _isCatchup = TRUE ;
      _confirmedStat = 0 ;

      return SDB_OK ;
   }

   void _pmdFTMgr::fini()
   {
   }

   void _pmdFTMgr::setMask( UINT32 ftmask )
   {
      if ( _ftMask != ( _ftMask & ftmask )  )
      {
         /// has removed some mask
         _confirmedStat = 0 ;
         _indoubtErr = SDB_OK ;
         _sampleWnd.clean() ;
      }

      _ftMask = ftmask ;
   }

   void _pmdFTMgr::setConfirmPeriod( UINT32 confirmPeriod )
   {
      if ( confirmPeriod < PMD_FT_CACL_INTERVAL_MIN )
      {
         _confirmPeriod = PMD_FT_CACL_INTERVAL_MIN ;
      }
      else if ( confirmPeriod > PMD_FT_CACL_INTERVAL_MAX )
      {
         _confirmPeriod = PMD_FT_CACL_INTERVAL_MAX ;
      }
      else
      {
         _confirmPeriod = confirmPeriod ;
      }
   }

   void _pmdFTMgr::setConfrimRatio( UINT32 confirmRatio )
   {
      if ( confirmRatio < PMD_FT_CACL_RATIO_MIN )
      {
         _confirmRatio = PMD_FT_CACL_RATIO_MIN ;
      }
      else if ( confirmRatio > PMD_FT_CACL_RATIO_MAX )
      {
         _confirmRatio = PMD_FT_CACL_RATIO_MAX ;
      }
      else
      {
         _confirmRatio = confirmRatio ;
      }
   }

   void _pmdFTMgr::setFTLevel( INT32 ftLevel )
   {
      if ( ftLevel < FT_LEVEL_FUSING )
      {
         _ftLevel = FT_LEVEL_FUSING ;
      }
      else if ( ftLevel > FT_LEVEL_WHOLE )
      {
         _ftLevel = FT_LEVEL_WHOLE ;
      }
      else
      {
         _ftLevel = ftLevel ;
      }
   }

   void _pmdFTMgr::setSlowNodeInfo( UINT32 threshold, UINT32 increment )
   {
      _slowNodeThreshold = (UINT64)threshold * 1024 * 1024 ;
      _slowNodeIncrement = (UINT64)increment * 1024 * 1024 ;
   }

   UINT64 _pmdFTMgr::_sumPrevnLsnDiff( ftSampleWndItem *pItem, UINT32 count )
   {
      UINT64 totalLsnDiff = 0 ;
      UINT32 i = 0 ;

      do
      {
         totalLsnDiff += pItem->_sys._lsnDiff ;
         pItem = _sampleWnd.prev( pItem->getPos() ) ;
      } while ( ++i < count ) ;

      return totalLsnDiff ;
   }

   ftSampleWndItem* _pmdFTMgr::_sample( UINT64 dbTick )
   {
#if defined ( SDB_ENGINE )
      ftSampleWndItem *pItem = NULL ;
      ftSampleWndItem *pPrevItem = NULL ;
      pmdKRCB *krcb = pmdGetKRCB() ;
      ICluster *pCluster = krcb->getCluster() ;
      SDB_DPSCB *pDpsCB = krcb->getDPSCB() ;

      DPS_LSN expectLsn ;

      UINT64 completeLsn = DPS_INVALID_LSN_OFFSET ;
      UINT32 lsnQueSize = 0 ;
      UINT64 primaryLsn = DPS_INVALID_LSN_OFFSET ;
      UINT64 lsnDiff = 0 ;
      UINT64 sucCount = 0 ;

      UINT32 countInc = 0 ;
      monDBCB *pMonDB = krcb->getMonDBCB() ;

      if ( !pDpsCB )
      {
         /// not data node
         goto done ;
      }

      sucCount = pMonDB->totalInsert +
                 pMonDB->totalUpdate +
                 pMonDB->totalDelete +
                 pMonDB->totalLobWrite ;
      if ( sucCount < _lastSucCount )
      {
         countInc = sucCount ;
      }
      else
      {
         countInc = sucCount - _lastSucCount ;
      }
      _lastSucCount = sucCount ;

      pItem = _sampleWnd.slideForward( dbTick ) ;
      pPrevItem = _sampleWnd.prev( pItem->getPos() ) ;

      /// first get primary info, then get local lsn info to avoid
      /// false inspection
      if ( pCluster )
      {
         completeLsn = pCluster->completeLsn( TRUE ) ;
         lsnQueSize = pCluster->lsnQueSize() ;

         if ( !pCluster->primaryLsn( primaryLsn ) )
         {
            /// nout found primary
            primaryLsn = 0 ;
         }
      }

      expectLsn = pDpsCB->expectLsn() ;

      if ( DPS_INVALID_LSN_OFFSET == completeLsn )
      {
         completeLsn = expectLsn.offset ;
      }
      if ( DPS_INVALID_LSN_OFFSET == primaryLsn )
      {
         primaryLsn = expectLsn.offset ;
      }

      if ( 0 != primaryLsn )
      {
         if ( primaryLsn >= completeLsn )
         {
            lsnDiff = primaryLsn - completeLsn ;
         }
         else
         {
            lsnDiff = 0 ;
         }

         if ( _isCatchup && lsnDiff < PMD_FT_CATCHUP_LSNDIFF_THRESHOLD )
         {
            _isCatchup = FALSE ;
            PD_LOG( PDEVENT, "Disable catchup node, Expr: "
                    "LsnDiff(%llu) < Threshold(%u)",
                    lsnDiff, PMD_FT_CATCHUP_LSNDIFF_THRESHOLD ) ;
         }
      }

      pItem->_sys._expectLsn = expectLsn.offset ;
      pItem->_sys._completeLsn = completeLsn ;
      pItem->_sys._lsnQueSize = lsnQueSize ;
      pItem->_sys._primaryLsn = primaryLsn ;
      pItem->_sys._lsnDiff = lsnDiff ;

      /// calc risk
      /// slow node
      if ( OSS_BIT_TEST( _ftMask, PMD_FT_MASK_SLOWNODE ) )
      {
         if ( lsnDiff >= _slowNodeThreshold )
         {
            /// When is fullsync, keep the same with last
            if ( SDB_DB_FULLSYNC == PMD_DB_STATUS() )
            {
               if ( pPrevItem->_risk[ FT_RISK_SLOW_NODE ]._count > 0 )
               {
                  _sampleWnd.reportRisk( FT_RISK_SLOW_NODE ) ;
                  PD_LOG( PDINFO, "Report risk( FT_RISK_SLOW_NODE ), Expr: "
                          "LsnDiff(%llu) >= Threshold(%llu) && "
                          "Database is in FullSync && "
                          "PrevItem is FT_RISK_SLOW_NODE",
                          lsnDiff, _slowNodeThreshold ) ;
               }
            }
            else if ( 0 == _slowNodeIncrement )
            {
               _sampleWnd.reportRisk( FT_RISK_SLOW_NODE ) ;
               PD_LOG( PDINFO, "Report risk( FT_RISK_SLOW_NODE ), Expr: "
                       "LsnDiff(%llu) >= Threshold(%llu) ",
                       lsnDiff, _slowNodeThreshold ) ;
            }
            else if ( _sampleWnd.getCount() > PMD_FT_SLOWNODE_N_WND_SZ )
            {
               UINT64 lsnDiffN = _sumPrevnLsnDiff( pItem,
                                                   PMD_FT_SLOWNODE_N_WND_SZ ) ;
               UINT64 prevLsnDiffN = _sumPrevnLsnDiff( pPrevItem,
                                                       PMD_FT_SLOWNODE_N_WND_SZ ) ;
               UINT64 avgN = lsnDiffN / PMD_FT_SLOWNODE_N_WND_SZ ;
               UINT64 prevAvgN = prevLsnDiffN / PMD_FT_SLOWNODE_N_WND_SZ ;
               if ( avgN >= prevAvgN + _slowNodeIncrement )
               {
                  _sampleWnd.reportRisk( FT_RISK_SLOW_NODE ) ;
                  PD_LOG( PDINFO, "Report risk( FT_RISK_SLOW_NODE ), Expr: "
                          "LsnDiff(%llu) >= Threshold(%llu) && "
                          "AvgN(%llu) >= PrevAvgN(%llu) + Increment(%llu)",
                          lsnDiff, _slowNodeThreshold,
                          avgN, prevAvgN,
                          _slowNodeIncrement ) ;
               }
               else if ( avgN > prevAvgN &&
                         pPrevItem->_risk[ FT_RISK_SLOW_NODE ]._count > 0 )
               {
                  _sampleWnd.reportRisk( FT_RISK_SLOW_NODE ) ;
                  PD_LOG( PDINFO, "Report risk( FT_RISK_SLOW_NODE ), Expr: "
                          "LsnDiff(%llu) >= Threshold(%llu) && "
                          "AvgN(%llu) > PrevAvgN(%llu) && "
                          "PrevItem is FT_RISK_SLOW_NODE",
                          lsnDiff, _slowNodeThreshold,
                          avgN, prevAvgN ) ;
               }
            }
         }
      }

      /// dead sync
      if ( OSS_BIT_TEST( _ftMask, PMD_FT_MASK_DEADSYNC ) )
      {
         if ( 0 != pPrevItem->_time )
         {
            if ( completeLsn == pPrevItem->_sys._completeLsn &&
                 ( lsnDiff > 0 ||
                   completeLsn < expectLsn.offset ) )
            {
               /// When is FullSync and countInc is no-zero, and no error
               /// means is not deadsync
               if ( SDB_DB_FULLSYNC == PMD_DB_STATUS() &&
                    countInc > 0 &&
                    pPrevItem->allErr() == 0 )
               {
                  /// means is not deadsync
               }
               else
               {
                  _sampleWnd.reportRisk( FT_RISK_DEADSYNC ) ;
                  PD_LOG( PDINFO, "Report risk( FT_RISK_DEADSYNC ), Expr: "
                          "CompleteLsn(%llu) == PreCompleteLsn(%llu) && "
                          "( LsnDiff(%llu) > 0 || "
                          "CompleteLsn < ExpectLsn(%llu) )",
                          completeLsn, pPrevItem->_sys._completeLsn,
                          lsnDiff, expectLsn.offset ) ;
               }
            }
         }
      }

      /// nospc
      if ( OSS_BIT_TEST( _ftMask, PMD_FT_MASK_NOSPC ) )
      {
         if ( 0 != pPrevItem->_time )
         {
            if ( completeLsn > pPrevItem->_sys._completeLsn )
            {
               pPrevItem->_err[ FT_ERR_NONE ]._count = countInc ;
            }
            /// when countInc is zero
            else if ( 0 == countInc &&
                      pPrevItem->_err[ FT_ERR_NOSPC ]._count > 0 )
            {
               if ( pPrevItem->_err[ FT_ERR_NOSPC ]._count >
                    PMD_FT_NOSPC_ADJ_MAX )
               {
                  pItem->_err[ FT_ERR_NOSPC ]._count = PMD_FT_NOSPC_ADJ_MAX ;
               }
               else
               {
                  pItem->_err[ FT_ERR_NOSPC ]._count =
                     pPrevItem->_err[ FT_ERR_NOSPC ]._count - 1 ;
               }
            }
         }
      }

   done:
      return pItem ;
#else
      return NULL ;
#endif // SDB_ENGINE
   }

   UINT32 _pmdFTMgr::_confirm( ftSampleWndItem *current )
   {
      UINT32 confirmStat = 0 ;
      UINT32 wndSize = _sampleWnd.getCount() ;
      UINT32 confirmWndSize = 0 ;
      ftSampleWndItem statItem ;
      UINT32 count = 0 ;
      FLOAT64 ratio = 0.0 ;
      FLOAT64 confirmRatio = (FLOAT64)_confirmRatio / 100 ;

      confirmWndSize = ( _confirmPeriod + PMD_FT_SAMPLE_INTERVAL - 1 ) /
                       PMD_FT_SAMPLE_INTERVAL ;

      /// nospc need atlest 2 sample wnd sz
      if ( confirmWndSize < 2 &&
           OSS_BIT_TEST( _ftMask, PMD_FT_MASK_SLOWNODE ) )
      {
         confirmWndSize = 2 ;
      }
      else if ( confirmWndSize > PMD_FT_SAMPLE_WINDOW_SZ )
      {
         confirmWndSize = PMD_FT_SAMPLE_WINDOW_SZ ;
      }
      else if ( confirmWndSize < 1 )
      {
         confirmWndSize = 1 ;
      }

      if ( confirmWndSize > wndSize )
      {
         /// Can not confirm others
         goto done ;
      }

      do
      {
         statItem += *current ;
         current = _sampleWnd.prev( current->getPos() ) ;
      } while ( ++count < confirmWndSize ) ;

      /// Analyse
      /// Risk: FT_RISK_SLOW_NODE
      if ( OSS_BIT_TEST( _ftMask, PMD_FT_MASK_SLOWNODE ) )
      {
         ratio = (FLOAT64)statItem._risk[ FT_RISK_SLOW_NODE ]._count /
                 confirmWndSize ;
         if ( ratio >= confirmRatio )
         {
            OSS_BIT_SET( confirmStat, PMD_FT_MASK_SLOWNODE ) ;
            PD_LOG( ( ( _confirmedStat & PMD_FT_MASK_SLOWNODE ) ?
                    PDINFO : PDWARNING ),
                    "Confirm Risk( FT_RISK_SLOW_NODE ), Expr: "
                    "Ratio(%.2f) >= ConfirmRatio(%.2f)",
                    ratio, confirmRatio ) ;
         }
         else if ( _confirmedStat & PMD_FT_MASK_SLOWNODE )
         {
            PD_LOG( PDEVENT,
                    "Disable Risk( FT_RISK_SLOW_NODE ), Expr: "
                    "Ratio(%.2f) < ConfirmRatio(%.2f)",
                    ratio, confirmRatio ) ;
         }
      }

      /// Risk: FT_RISK_DEADSYNC
      if ( OSS_BIT_TEST( _ftMask, PMD_FT_MASK_DEADSYNC ) )
      {
         ratio = (FLOAT64)statItem._risk[ FT_RISK_DEADSYNC ]._count /
                 confirmWndSize ;
         if ( ratio >= confirmRatio )
         {
            OSS_BIT_SET( confirmStat, PMD_FT_MASK_DEADSYNC ) ;
            PD_LOG( ( ( _confirmedStat & PMD_FT_MASK_DEADSYNC ) ?
                    PDINFO : PDWARNING ),
                    "Confirm Risk( FT_RISK_DEADSYNC ), Expr: "
                    "Ratio(%.2f) >= ConfirmRatio(%.2f)",
                    ratio, confirmRatio ) ;
         }
         else if ( _confirmedStat & PMD_FT_MASK_DEADSYNC )
         {
            PD_LOG( PDEVENT,
                    "Disable Risk( FT_RISK_DEADSYNC ), Expr: "
                    "Ratio(%.2f) < ConfirmRatio(%.2f)",
                    ratio, confirmRatio ) ;
         }
      }

      /// Err: FT_ERR_NOSPC
      if ( OSS_BIT_TEST( _ftMask, PMD_FT_MASK_NOSPC ) )
      {
         UINT32 total = statItem._err[ FT_ERR_NOSPC ]._count +
                        statItem._err[ FT_ERR_NONE ]._count ;
         if ( total > 0 )
         {
            ratio = ( FLOAT64 )statItem._err[ FT_ERR_NOSPC ]._count / total ;
         }
         else
         {
            ratio = 0 ;
         }

         if ( ratio >= confirmRatio )
         {
            OSS_BIT_SET( confirmStat, PMD_FT_MASK_NOSPC ) ;
            PD_LOG( ( ( _confirmedStat & PMD_FT_MASK_NOSPC ) ?
                    PDINFO : PDWARNING ),
                    "Confirm Err( FT_ERR_NOSPC ), Expr: "
                    "Ratio(%.2f) >= ConfirmRatio(%.2f)",
                    ratio, confirmRatio ) ;
         }
         else if ( _confirmedStat & PMD_FT_MASK_NOSPC )
         {
            PD_LOG( PDEVENT,
                    "Disable Err( FT_ERR_NOSPC ), Expr: "
                    "Ratio(%.2f) < ConfirmRatio(%.2f)",
                    ratio, confirmRatio ) ;
         }
      }

   done:
      return confirmStat ;
   }

   void _pmdFTMgr::run()
   {
      ftSampleWndItem *pSampleItem = NULL ;
      UINT64 dbTick = pmdGetDBTick() ;

      if ( pmdDBTickSpan2Time( dbTick - _lastSampleTick ) <
           PMD_FT_SAMPLE_INTERVAL * OSS_ONE_SEC )
      {
         goto done ;
      }

      _lastSampleTick = dbTick ;

      /// First sample
      pSampleItem = _sample( dbTick ) ;

      /// Then confirm
      if ( pSampleItem )
      {
         _confirmedStat = _confirm( pSampleItem ) ;
      }
      else
      {
         _confirmedStat = 0 ;
      }

      if ( 0 == _confirmedStat )
      {
         _indoubtErr = SDB_OK ;
      }

   done:
      return ;
   }

   BOOLEAN _pmdFTMgr::isStop() const
   {
      pmdKRCB *krcb = pmdGetKRCB() ;
      return krcb->isShutdown() ;
   }

   static BOOLEAN _ftIsErrorIn( INT32 err, const INT32 *pArray, UINT32 size )
   {
      for ( UINT32 i = 0 ; i < size ; ++i )
      {
         if ( pArray[i] == err )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   void _pmdFTMgr::reportErr( INT32 err, BOOLEAN isWrite )
   {
      const static INT32 _ignored[] = {
         SDB_DMS_EOC, SDB_INVALIDARG, SDB_EOF, SDB_IXM_EOC,
         SDB_CLS_NO_CATALOG_INFO, SDB_CLS_NODE_NOT_ENOUGH,
         SDB_CLS_NOT_PRIMARY, SDB_UNKNOWN_MESSAGE,
         SDB_APP_INTERRUPT
         } ;

      const static INT32 _prefered[] = {
         SDB_IO, SDB_OOM, SDB_PERM, SDB_FNE, SDB_FE, SDB_NOSPC,
         SDB_IXM_DUP_KEY
         } ;

      /// ignore
      if ( _ftIsErrorIn( err, _ignored, sizeof( _ignored ) / sizeof( INT32 ) ) )
      {
         /// ignore
      }
      else if ( _ftIsErrorIn( err, _prefered,
                              sizeof( _prefered ) / sizeof( INT32 ) ) )
      {
         _indoubtErr = err ;
      }
      else if ( isWrite &&
                !_ftIsErrorIn( _indoubtErr, _prefered,
                               sizeof( _prefered ) / sizeof( INT32 ) ) )
      {
         _indoubtErr = err ;
      }

      if ( SDB_NOSPC == err )
      {
         _sampleWnd.reportErr( FT_ERR_NOSPC ) ;
      }
   }

   /*
      Tool function
   */
   void ftReportErr( INT32 err, BOOLEAN isWrite )
   {
      if ( err )
      {
         pmdFTMgr *pMgr = pmdGetKRCB()->getFTMgr() ;
         if ( pMgr )
         {
            pMgr->reportErr( err, isWrite ) ;
         }

         pmdIncErrNum( err ) ;
      }
   }

}

