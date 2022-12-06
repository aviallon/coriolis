// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2008-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |      K i t e  -  D e t a i l e d   R o u t e r                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./NegociateWindow.cpp"                    |
// +-----------------------------------------------------------------+


#include <vector>
#include <algorithm>
#include <iomanip>
#include "hurricane/Warning.h"
#include "hurricane/Bug.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/Net.h"
#include "hurricane/Cell.h"
#include "crlcore/Utilities.h"
#include "crlcore/AllianceFramework.h"
#include "crlcore/Measures.h"
#include "crlcore/Histogram.h"
#include "katabatic/AutoContact.h"
#include "katabatic/GCellGrid.h"
#include "kite/DataNegociate.h"
#include "kite/TrackElement.h"
#include "kite/TrackMarker.h"
#include "kite/TrackCost.h"
#include "kite/Track.h"
#include "kite/TrackSegment.h"
#include "kite/RoutingPlane.h"
#include "kite/RoutingEventQueue.h"
#include "kite/RoutingEventHistory.h"
#include "kite/RoutingEventLoop.h"
#include "kite/NegociateWindow.h"
#include "kite/KiteEngine.h"


namespace {

  using namespace std;
  using namespace Hurricane;
  using namespace CRL;
  using namespace Kite;


  void  NegociateOverlapCost ( const TrackElement* segment, TrackCost& cost )
  {
    cdebug_log(9000,0) << "Deter| NegociateOverlapCost() " << segment << endl;
    Interval intersect = segment->getCanonicalInterval();

    if (not intersect.intersect(cost.getInterval())) return;

    if (segment->isBlockage() or segment->isFixed()) {
      cdebug_log(159,0) << "Infinite cost from: " << segment << endl;
      cost.setInfinite   ();
      cost.setOverlap    ();
      cost.setHardOverlap();
      cost.setBlockage   ();
      return;
    }

    if (cost.getInterval().getVMax() > intersect.getVMax()) cost.setLeftOverlap();
    if (cost.getInterval().getVMin() < intersect.getVMin()) cost.setRightOverlap();

    if (not intersect.contains(cost.getInterval()))
      intersect.intersection( cost.getInterval() );
    else {
      cost.setLonguestOverlap( intersect.getSize() );
      cost.setGlobalEnclosed();
    }

    DataNegociate* data = segment->getDataNegociate();
    if (not data) return;

    cost.mergeRipupCount( data->getRipupCount() );
    if ( segment->isLocal() ) {
      cost.mergeDataState( data->getState() );
      if (data->getState() >=  DataNegociate::LocalVsGlobal) {
        cdebug_log(159,0) << "MaximumSlack/LocalVsGlobal for " << segment << endl;
      }
    }

    if (segment->isGlobal()) {
      cost.setOverlapGlobal();
      if (   (cost.getFlags() & TrackCost::LocalAndTopDepth)
         and (data->getState() >= DataNegociate::MoveUp) ) {
        cost.setInfinite   ();
        cost.setOverlap    ();
        cost.setHardOverlap();
        return;
      }
    }

    cost.setOverlap();
    if ( segment->isLocal()
       or (cost.isForGlobal() and (Session::getRoutingGauge()->getLayerDepth(segment->getLayer()) < 3)) ) {
      cdebug_log(9000,0) << "Deter|     incTerminals() " << boolalpha << cost.isForGlobal() << " " << (data->getTerminals()*100) << endl;
      cost.incTerminals( data->getTerminals()*100 );
    } else {
      cdebug_log(9000,0) << "Deter|     isForGlobal() " << boolalpha << cost.isForGlobal() << endl;
    }

    cdebug_log(159,0) << "| Increment Delta: " << DbU::getValueString(intersect.getSize()) << endl;
    cost.incDelta( intersect.getSize() );
  }


  void  loadRoutingPads ( NegociateWindow* nw )
  {
    AllianceFramework* af = AllianceFramework::get ();
    RoutingGauge*      rg = nw->getKiteEngine()->getRoutingGauge();

    for( Net* net : nw->getCell()->getNets() ) {
      if (net->getType() == Net::Type::POWER ) continue;
      if (net->getType() == Net::Type::GROUND) continue;
      if (net->getType() == Net::Type::CLOCK ) continue;
      if (af->isBLOCKAGE(net->getName())) continue;

      for( RoutingPad* rp : net->getRoutingPads() ) {
        size_t depth = rg->getLayerDepth(rp->getLayer());
        if (depth >  0) continue;
        if (depth == 0) 
          TrackMarker::create( rp, 1 );
      }
    }
  }


} // Anonymous namespace.


namespace Kite {

  using std::cerr;
  using std::endl;
  using std::setw;
  using std::left;
  using std::right;
  using std::setprecision;
  using Hurricane::Warning;
  using Hurricane::Bug;
  using Hurricane::tab;
  using Hurricane::ForEachIterator;
  using CRL::Histogram;
  using CRL::addMeasure;
  using Katabatic::AutoContact;
  using Katabatic::AutoSegmentLut;
  using Katabatic::perpandicularTo;


// -------------------------------------------------------------------
// Class  :  "NegociateWindow".


  NegociateWindow::NegociateWindow ( KiteEngine* kite )
    : _flags       (KtNoFlags)
    , _interrupt   (false)
    , _kite        (kite)
    , _gcells      ()
    , _segments    ()
    , _eventQueue  ()
    , _eventHistory()
    , _eventLoop   (10,50)
  { }


  NegociateWindow* NegociateWindow::create ( KiteEngine* kite )
  {
    NegociateWindow* negociateWindow = new NegociateWindow ( kite );
    return negociateWindow;
  }


  NegociateWindow::~NegociateWindow ()
  { }


  void  NegociateWindow::destroy ()
  { delete this; }


  Cell* NegociateWindow::getCell () const
  { return _kite->getCell(); }


  void  NegociateWindow::setGCells ( const Katabatic::GCellVector& gcells )
  {
    _gcells = gcells;

    loadRoutingPads( this );
    Session::revalidate();

    TrackElement*            segment;
    AutoSegmentLut           lut = Session::getKiteEngine()->_getAutoSegmentLut();
    AutoSegmentLut::iterator it  = lut.begin ();
    for ( ; it != lut.end() ; it++ ) {
      segment = Session::lookup( it->second );
      if (segment) segment->getDataNegociate()->update();
    }

    _statistics.setGCellsCount( _gcells.size() );
  }


  void  NegociateWindow::addRoutingEvent ( TrackElement* segment, unsigned int level )
  {
    DataNegociate* data = segment->getDataNegociate();
    if (not data or not data->hasRoutingEvent())
      _eventQueue.add( segment, level );
    else
      cerr << Bug( "NegociateWidow::addRoutingEvent(): Try to adds twice the same TrackElement event."
                   "\n       %p:%s."
                 , (void*)segment->base()->base()
                 , getString(segment).c_str()
                 ) << endl;
  }


  TrackElement* NegociateWindow::createTrackSegment ( AutoSegment* autoSegment, unsigned int flags )
  {
    cdebug_log(159,1) << "NegociateWindow::createTrackSegment() - " << autoSegment << endl;

  // Special case: fixed AutoSegments must not interfere with blockages.
  // Ugly: uses of getExtensionCap().
    if (autoSegment->isFixed()) {
      RoutingPlane* plane = Session::getKiteEngine()->getRoutingPlaneByLayer(autoSegment->getLayer());
      Track*        track = plane->getTrackByPosition( autoSegment->getAxis() );
      size_t        begin;
      size_t        end;
      Interval      fixedSpan;
      Interval      blockageSpan;

      autoSegment->getCanonical( fixedSpan );
      fixedSpan.inflate( Session::getExtensionCap(autoSegment->getLayer())-1 );

      track->getOverlapBounds( fixedSpan, begin, end );
      for ( ; (begin < end) ; begin++ ) {

        TrackElement* other = track->getSegment(begin);
        cdebug_log(159,0) << "| overlap: " << other << endl;

        if (not other->isBlockage()) continue;

        other->getCanonical( blockageSpan );
        blockageSpan.inflate( Session::getExtensionCap(autoSegment->getLayer()) );

        cdebug_log(159,0) << "  fixed:" << fixedSpan << " vs. blockage:" << blockageSpan << endl;

        if (not fixedSpan.intersect(blockageSpan)) continue;

      // Overlap between fixed & blockage.
        cdebug_log(159,0) << "* Blockage overlap: " << autoSegment << endl;
        Session::destroyRequest( autoSegment );

        cerr << Warning( "Overlap between fixed %s and blockage at %s."
                       , getString(autoSegment).c_str()
                       , getString(blockageSpan).c_str() ) << endl;
        cdebug_tabw(159,-1);
        return NULL;
      }
    }

    Interval span;
    autoSegment = autoSegment->getCanonical( span );

    bool           created;
    TrackElement*  trackSegment  = TrackSegment::create( autoSegment, NULL, created );

    if (not (flags & KtLoadingStage))
      cdebug_log(159,0) << "* lookup: " << autoSegment << endl;

    if (created) {
      cdebug_log(159,0) << "* " << trackSegment << endl;

      RoutingPlane* plane = Session::getKiteEngine()->getRoutingPlaneByLayer(autoSegment->getLayer());
      Track*        track = plane->getTrackByPosition ( autoSegment->getAxis() );
      Interval      uside = autoSegment->getAutoSource()->getGCell()->getSide( perpandicularTo(autoSegment->getDirection()) );

      Interval      constraints;
      autoSegment->getConstraints( constraints );
      uside.intersection( constraints );

      if (track->getAxis() > uside.getVMax()) track = track->getPreviousTrack();
      if (track->getAxis() < uside.getVMin()) track = track->getNextTrack();

      cdebug_log(159,0) << "* GCell U-side " << uside << endl;
      cdebug_log(159,0) << "* " << plane << endl;
      cdebug_log(159,0) << "* " << track << endl;

      trackSegment->setAxis( track->getAxis(), Katabatic::SegAxisSet );
      trackSegment->invalidate();

      if (trackSegment->isFixed()) {
        Session::addInsertEvent( trackSegment, track );
      } else {
        _segments.push_back( trackSegment );
      }
    }

    if (not created and not (flags & KtLoadingStage)) {
      cdebug_log(159,0) << "TrackSegment already exists (and not in loading stage)." << endl;
    }

    cdebug_tabw(159,-1);

    return trackSegment;
  }


  double  NegociateWindow::computeWirelength ()
  {
    set<TrackElement*> accounteds;
    double totalWL = 0.0;

    for ( size_t igcell=0 ; igcell<_gcells.size() ; ++igcell ) {
      double        gcellWL = 0.0;
      Segment*      segment;
      TrackElement* trackSegment;

      const vector<AutoContact*>& contacts = _gcells[igcell]->getContacts();
      for ( size_t i=0 ; i<contacts.size() ; i++ ) {
        for( Hook* hook : contacts[i]->getBodyHook()->getSlaveHooks() ) {
          Hook* sourceHook = dynamic_cast<Segment::SourceHook*>(hook);
          if (not sourceHook) continue;
          
          segment       = dynamic_cast<Segment*>(sourceHook->getComponent());
          trackSegment  = Session::lookup( segment );
          if (trackSegment) {
            if (accounteds.find(trackSegment) != accounteds.end()) continue;

            accounteds.insert( trackSegment );
            gcellWL += DbU::getLambda( trackSegment->getLength() );
          }
        }
      }

    // Partial sum to limit rounding errors.
      totalWL += gcellWL;
    }

    return totalWL;
  }


  void  NegociateWindow::_createRouting  ( Katabatic::GCell* gcell )
  {
    cdebug_log(159,1) << "NegociateWindow::_createRouting() - " << gcell << endl;

    Segment*     segment;
    AutoSegment* autoSegment;

    cdebug_log(159,0) << "AutoSegments from AutoContacts" << endl;
    const vector<AutoContact*>& contacts = gcell->getContacts();
    for ( size_t i=0 ; i<contacts.size() ; i++ ) {
      for( Component* component : contacts[i]->getSlaveComponents() ) {
        segment      = dynamic_cast<Segment*>(component);
        autoSegment  = Session::base()->lookup( segment );
        cdebug_log(159,0) << autoSegment << endl;
        if (autoSegment and autoSegment->isCanonical()) {
          createTrackSegment( autoSegment, KtLoadingStage );
        }
      }
    }

    cdebug_log(159,0) << "_segments.size():" << _segments.size() << endl;
    cdebug_tabw(159,-1);
  }


  void  NegociateWindow::_pack ( size_t& count, bool last )
  {
    unsigned long limit     = _kite->getEventsLimit();
    unsigned int  pushStage = RoutingEvent::getStage();
    RoutingEvent::setStage( RoutingEvent::Pack );

    RoutingEventQueue  packQueue;
  //for ( size_t i = (count > 600) ? count-600 : 0
  //    ; (i<_eventHistory.size()-(last ? 0 : 100)) and not isInterrupted() ; i++ ) {
    for ( size_t i=0 ; i<_eventHistory.size() ; ++i ) {
      RoutingEvent* event = _eventHistory.getNth(i);

      if ( event and not event->isCloned() ) {
        cerr << "Cloned:" << event->isCloned()
             << " UTurn:" << event->getSegment()->isUTurn() << " " << event->getSegment() << endl;
      }
          
      if ( event and not event->isCloned() and event->getSegment()->isUTurn() ) {
        event->reschedule( packQueue, 0 );
      }
    }
    packQueue.commit();

    while ( not packQueue.empty() and not isInterrupted() ) {
      RoutingEvent* event = packQueue.pop();

      if (tty::enabled()) {
        cmess2 << "        <pack.event:" << tty::bold << setw(8) << setfill('0')
               << RoutingEvent::getProcesseds() << tty::reset
               << " remains:" << right << setw(8) << setfill('0')
               << packQueue.size() << ">"
               << setfill(' ') << tty::reset << tty::cr;
        cmess2.flush();
      } else {
        cmess2 << "        <pack.event:" << setw(8) << setfill('0')
               << RoutingEvent::getProcesseds() << setfill(' ') << " "
               << event->getEventLevel() << ":" << event->getPriority() << "> "
               << event->getSegment()
               << endl;
        cmess2.flush();
      }

      event->process( packQueue, _eventHistory, _eventLoop );

      if (RoutingEvent::getProcesseds() >= limit) setInterrupt( true );
    }
  // Count will be wrong!

    RoutingEvent::setStage( pushStage );
  }


  size_t  NegociateWindow::_negociate ()
  {
    cdebug_log(9000,0) << "Deter| NegociateWindow::_negociate()" << endl;
    cdebug_log(159,1) << "NegociateWindow::_negociate() - " << _segments.size() << endl;

    cmess1 << "     o  Negociation Stage." << endl;

    unsigned long limit     = _kite->getEventsLimit();
    bool          profiling = _kite->profileEventCosts();
    ofstream      ofprofile;

    if (profiling) ofprofile.open( "kite.profile.txt" );

    _eventHistory.clear();
    _eventQueue.load( _segments );
    cmess2 << "        <queue:" <<  right << setw(8) << setfill('0') << _eventQueue.size() << ">" << endl;
    if (cdebug.enabled(9000)) _eventQueue.dump();

    size_t count = 0;
    RoutingEvent::setStage( RoutingEvent::Negociate );
    while ( not _eventQueue.empty() and not isInterrupted() ) {
      RoutingEvent* event = _eventQueue.pop();

      if (tty::enabled()) {
        cmess2 << "        <event:" << tty::bold << right << setw(8) << setfill('0')
               << RoutingEvent::getProcesseds() << tty::reset
               << " remains:" << right << setw(8) << setfill('0')
               << _eventQueue.size()
               << setfill(' ') << tty::reset << ">" << tty::cr;
        cmess2.flush ();
      } else {
        cmess2 << "        <event:" << right << setw(8) << setfill('0')
               << RoutingEvent::getProcesseds() << setfill(' ') << " "
               << event->getEventLevel() << ":" << event->getPriority() << "> "
               << event->getSegment()
               << endl;
        cmess2.flush();
      }

      if (ofprofile.is_open()) {
        size_t depth = _kite->getConfiguration()->getLayerDepth( event->getSegment()->getLayer() );
        if (depth < 6) {
          ofprofile << setw(10) << right << count << " ";
          for ( size_t i=0 ; i<6 ; ++i ) {
            if (i == depth)
              ofprofile << setw(10) << right << setprecision(2) << event->getPriority  () << " ";
            else
              ofprofile << setw(10) << right << setprecision(2) << 0.0 << " ";
          }

          ofprofile << setw( 2) << right << event->getEventLevel() << endl;
        }
      }

      event->process( _eventQueue, _eventHistory, _eventLoop );
      count++;

    //if (count and not (count % 500)) {
    //  _pack( count, false );
    //} 

      if (RoutingEvent::getProcesseds() >= limit) setInterrupt( true );
    }
  //_pack( count, true );
    if (count and cmess2.enabled() and tty::enabled()) cmess1 << endl;

    cdebug_log(9000,0) << "Deter| Repair Stage" << endl;
    cmess1 << "     o  Repair Stage." << endl;

    cdebug_log(159,0) << "Loadind Repair queue." << endl;
    RoutingEvent::setStage( RoutingEvent::Repair );
    for ( size_t i=0 ; (i<_eventHistory.size()) and not isInterrupted() ; i++ ) {
      RoutingEvent* event = _eventHistory.getNth(i);

      if (not event->isCloned() and event->isUnimplemented()) {
        event->reschedule( _eventQueue, 0 );
      }
    }
    _eventQueue.commit();
    cmess2 << "        <repair.queue:" <<  right << setw(8) << setfill('0')
           << _eventQueue.size() << ">" << endl;

    count = 0;
  //_eventQueue.prepareRepair();
    while ( not _eventQueue.empty() and not isInterrupted() ) {
      RoutingEvent* event = _eventQueue.pop();

      if (tty::enabled()) {
        cmess2 << "        <repair.event:" << tty::bold << setw(8) << setfill('0')
               << RoutingEvent::getProcesseds() << tty::reset
               << " remains:" << right << setw(8) << setfill('0')
               << _eventQueue.size() << ">"
               << setfill(' ') << tty::reset << tty::cr;
        cmess2.flush();
      } else {
        cmess2 << "        <repair.event:" << setw(8) << setfill('0')
               << RoutingEvent::getProcesseds() << setfill(' ') << " "
               << event->getEventLevel() << ":" << event->getPriority() << "> "
               << event->getSegment()
               << endl;
        cmess2.flush();
      }

      event->process( _eventQueue, _eventHistory, _eventLoop );

      count++;
      if (RoutingEvent::getProcesseds() >= limit ) setInterrupt( true );
    }

    if (count and cmess2.enabled() and tty::enabled()) cmess1 << endl;

    size_t eventsCount = _eventHistory.size();

    _eventHistory.clear();
    _eventQueue.clear();

    if (RoutingEvent::getAllocateds() > 0) {
      cerr << Bug( "%d events remains after clear.", RoutingEvent::getAllocateds() ) << endl;
    }

    if (ofprofile.is_open()) ofprofile.close();
    _statistics.setEventsCount( eventsCount );
    cdebug_tabw(159,-1);

    return eventsCount;
  }


  void  NegociateWindow::run ( unsigned int flags )
  {
    cdebug_log(159,1) << "NegociateWindow::run()" << endl;

    cmess1 << "  o  Running Negociate Algorithm" << endl;

    TrackElement::setOverlapCostCB( NegociateOverlapCost );
    RoutingEvent::resetProcesseds();

    for ( size_t igcell=0 ; igcell<_gcells.size() ; ++igcell ) {
      _createRouting( _gcells[igcell] );
    }
    Session::revalidate();

    if (not (flags & KtPreRoutedStage)) {
      _kite->preProcess();
      Session::revalidate();
    }

    _kite->setMinimumWL( computeWirelength() );

#if defined(CHECK_DATABASE)
    unsigned int overlaps = 0;
    Session::getKiteEngine()->_check( overlaps, "after _createRouting(GCell*)" );
#endif 

    _flags |= flags;
    _negociate();
    printStatistics();

    if (flags & KtPreRoutedStage) {
      _kite->setFixedPreRouted();
    }

    Session::revalidate();
    Session::get()->isEmpty();

# if defined(CHECK_DATABASE)
    _kite->_check( overlaps, "after negociation" );
# endif

    cdebug_tabw(159,-1);
  }


  void  NegociateWindow::printStatistics () const
  {
    cmess1 << "  o  Computing statistics." << endl;
    cmess1 << Dots::asSizet("     - Processeds Events Total",RoutingEvent::getProcesseds()) << endl;
    cmess1 << Dots::asSizet("     - Unique Events Total"
                           ,(RoutingEvent::getProcesseds() - RoutingEvent::getCloneds())) << endl;
    cmess1 << Dots::asSizet("     - # of GCells",_statistics.getGCellsCount()) << endl;
    _kite->printCompletion();

    _kite->addMeasure<size_t>( "Events" , RoutingEvent::getProcesseds(), 12 );
    _kite->addMeasure<size_t>( "UEvents", RoutingEvent::getProcesseds()-RoutingEvent::getCloneds(), 12 );

    Histogram* densityHistogram = new Histogram ( 1.0, 0.1, 2 );
    _kite->addMeasure<Histogram>( "GCells Density Histogram", densityHistogram );

    densityHistogram->setFileExtension( ".density.histogram" );
    densityHistogram->setMainTitle    ( "GCell Densities" );
    densityHistogram->setTitle        ( "Avg. Density", 0 );
    densityHistogram->setTitle        ( "Peak Density", 1 );
    densityHistogram->setColor        ( "green", 0 );
    densityHistogram->setColor        ( "red"  , 1 );

    const Katabatic::GCellVector* gcells = getKiteEngine()->getGCellGrid()->getGCellVector();

    getKiteEngine()->getGCellGrid()->setDensityMode( Katabatic::GCellGrid::MaxHVDensity );
    for ( size_t igcell=0 ; igcell<(*gcells).size() ; ++igcell ) {
      densityHistogram->addSample( (*gcells)[igcell]->getDensity(), 0 );
    }

    getKiteEngine()->getGCellGrid()->setDensityMode( Katabatic::GCellGrid::MaxDensity );
    for ( size_t igcell=0 ; igcell<(*gcells).size() ; ++igcell ) {
      densityHistogram->addSample( (*gcells)[igcell]->getDensity(), 1 );
    }

    densityHistogram->normalize( 0 );
    densityHistogram->normalize( 1 );
  }


  string  NegociateWindow::_getString () const
  {
    ostringstream  os;

    os << "<" << _getTypeName() << ">";
    return os.str();
  }


  Record* NegociateWindow::_getRecord () const
  {
    Record* record = new Record ( _getString() );
                                     
    record->add( getSlot( "_gcells", _gcells ) );
    return record;
  }


} // Kite namespace.
