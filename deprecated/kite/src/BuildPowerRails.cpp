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
// |  C++ Module  :       "./BuildPowerRails.cpp"                    |
// +-----------------------------------------------------------------+


#include <map>
#include <list>
#include "hurricane/DebugSession.h"
#include "hurricane/Error.h"
#include "hurricane/Warning.h"
#include "hurricane/DataBase.h"
#include "hurricane/Technology.h"
#include "hurricane/BasicLayer.h"
#include "hurricane/RegularLayer.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Vertical.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/NetExternalComponents.h"
#include "hurricane/NetRoutingProperty.h"
#include "hurricane/Instance.h"
#include "hurricane/Plug.h"
#include "hurricane/Path.h"
#include "hurricane/Query.h"
#include "crlcore/AllianceFramework.h"
#include "katabatic/GCell.h"
#include "katabatic/GCellGrid.h"
#include "kite/RoutingPlane.h"
#include "kite/TrackFixedSegment.h"
#include "kite/Track.h"
#include "kite/KiteEngine.h"


namespace {

  using namespace std;
  using Hurricane::tab;
  using Hurricane::DebugSession;
  using Hurricane::ForEachIterator;
  using Hurricane::Warning;
  using Hurricane::Error;
  using Hurricane::DbU;
  using Hurricane::Box;
  using Hurricane::Interval;
  using Hurricane::Net;
  using Hurricane::DeepNet;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;
  using Hurricane::RoutingPad;
  using Hurricane::NetExternalComponents;
  using Hurricane::NetRoutingExtension;
  using Hurricane::NetRoutingState;
  using Hurricane::Instance;
  using Hurricane::Plug;
  using Hurricane::Path;
  using Hurricane::Query;
  using Hurricane::Go;
  using Hurricane::Rubber;
  using Hurricane::Layer;
  using Hurricane::BasicLayer;
  using Hurricane::RegularLayer;
  using Hurricane::Transformation;
  using Hurricane::Technology;
  using Hurricane::DataBase;
  using CRL::AllianceFramework;
  using Katabatic::GCell;
  using Katabatic::ChipTools;
  using Katabatic::NetRoutingExtension;
  using namespace Kite;


// -------------------------------------------------------------------
// Local Functions.

  void  destroyRing ( Net* net )
  {
    for( RoutingPad* rp : net->getRoutingPads() ) {
      bool          allMasters = true;
      vector<Hook*> ring;
      for( Hook* hook : rp->getBodyHook()->getHooks() ) {
        if (not hook->isMaster()) { allMasters = false; break; }
        ring.push_back( hook );
      }
      if (allMasters) {
        for ( auto hook : ring ) {
          hook->_setNextHook( hook );
        }
      }
    }
  }


// -------------------------------------------------------------------
// Class  :  "::GlobalNetTable".

  class GlobalNetTable {
    public:
      enum Flag { ClockIsRouted=0x0001 };
    public:
                   GlobalNetTable       ( KiteEngine* );
             bool  isCoreClockNetRouted ( const Net* ) const;
      inline Cell* getTopCell           () const;
             Net*  getRootNet           ( const Net*, Path ) const;
      inline Net*  getVdde              () const;
      inline Net*  getVddi              () const;
      inline Net*  getVsse              () const;
      inline Net*  getVssi              () const;
      inline Net*  getCk                () const;
      inline Net*  getCki               () const;
      inline Net*  getCko               () const;
      inline Net*  getBlockage          () const;
      inline void  setBlockage          ( Net* );
    private:                            
             bool  guessGlobalNet       ( const Name&, Net* );
    private:
      unsigned int  _flags;
      Name          _vddePadNetName;
      Name          _vddiPadNetName;
      Name          _vssePadNetName;
      Name          _vssiPadNetName;
      Name          _ckPadNetName;
      Name          _ckiPadNetName;
      Name          _ckoPadNetName;
      Net*          _vdde;
      Net*          _vddi;
      Net*          _vsse;
      Net*          _vssi;
      Net*          _ck;    // Clock net on the (external) pad.
      Net*          _cki;   // Clock net in the pad ring.
      Net*          _cko;   // Clock net of the core (design).
      Net*          _blockage;
      Cell*         _topCell;
  };


  inline Cell* GlobalNetTable::getTopCell  () const { return _topCell; }
  inline Net*  GlobalNetTable::getVdde     () const { return _vdde; }
  inline Net*  GlobalNetTable::getVddi     () const { return _vddi; }
  inline Net*  GlobalNetTable::getVsse     () const { return _vsse; }
  inline Net*  GlobalNetTable::getVssi     () const { return _vssi; }
  inline Net*  GlobalNetTable::getCk       () const { return _ck; }
  inline Net*  GlobalNetTable::getCki      () const { return _cki; }
  inline Net*  GlobalNetTable::getCko      () const { return _cko; }
  inline Net*  GlobalNetTable::getBlockage () const { return _blockage; }
  inline void  GlobalNetTable::setBlockage ( Net* net ) { _blockage=net; }

  GlobalNetTable::GlobalNetTable ( KiteEngine* kite )
    : _flags         (0)
    , _vddePadNetName("vdde")
    , _vddiPadNetName("vddi")
    , _vssePadNetName("vsse")
    , _vssiPadNetName("vssi")
    , _ckPadNetName  ("pad" )
    , _ckiPadNetName ("ck"  )
    , _ckoPadNetName ("cko" )
    , _vdde    (NULL)
    , _vddi    (NULL)
    , _vsse    (NULL)
    , _vssi    (NULL)
    , _ck      (NULL)
    , _cki     (NULL)
    , _cko     (NULL)
    , _blockage(NULL)
    , _topCell (kite->getCell())
  {
    if (_topCell == NULL) return;

    cmess1 << "  o  Looking for powers/grounds & clocks." << endl;

    AllianceFramework* af = AllianceFramework::get();

    bool hasPad = false;
    for( Instance* instance : _topCell->getInstances() ) {
      if (af->isPad(instance->getMasterCell())) {
        if (not hasPad) {
          cmess1 << "  o  Design has pads, assuming complete chip top structure." << endl;
          hasPad = true;
        }

        string padName = getString( instance->getMasterCell()->getName() );
        if (padName.substr(0,8) == "pvddeck_") {
          cmess1 << "     o  Reference power pad: " << instance->getName()
                 << "(model:" << instance->getMasterCell()->getName() << ")." << endl;

        // Guessing the power, ground and clock nets from *this* pad connexions.
          for( Plug* plug : instance->getPlugs() ) {
            Net*      masterNet = plug->getMasterNet();
            Net::Type netType   = masterNet->getType();
            if (   (netType != Net::Type::POWER )
               and (netType != Net::Type::GROUND)
               and (netType != Net::Type::CLOCK ) ) continue;

            Net* net = plug->getNet();
            if (not net) {
              net = _topCell->getNet( masterNet->getName() );
              if (not net) {
                cerr << Error("Missing global net <%s> at chip level.",getString(masterNet->getName()).c_str()) << endl;
                continue;
              }
            }

            guessGlobalNet( masterNet->getName(), net );
          }
        }

        padName = getString( instance->getMasterCell()->getName() );
        if (padName.substr(0,4) == "pck_") {
          cmess1 << "     o  Reference clock pad: " << instance->getName()
                 << "(model:" << instance->getMasterCell()->getName() << ")." << endl;

        // Guessing external clock net *only* from *this* pad connexions.
          for( Plug* plug : instance->getPlugs() ) {
            Net* masterNet = plug->getMasterNet();
            Net* net       = plug->getNet();
            if (not net) {
              net = _topCell->getNet( masterNet->getName() );
              if (not net) {
                cerr << Error("Missing global net <%s> at chip level.",getString(masterNet->getName()).c_str()) << endl;
                continue;
              }
            }

            if (masterNet->getName() == _ckPadNetName) {
              cmess1 << "        - Using <" << net->getName() << "> as external chip clock net." << endl;
              _ck = net;
            }
          }
        }
      }
    }

    if (hasPad) {
      if (_vdde == NULL) cerr << Error("Missing <vdde> net (for pads) at chip level." ) << endl;
      else destroyRing( _vdde );
      if (_vsse == NULL) cerr << Error("Missing <vsse> net (for pads) at chip level." ) << endl;
      else destroyRing( _vsse );
      if (_vddi == NULL) cerr << Error("Missing <vddi>/<vdd> net (for pads) at top level." ) << endl;
      else destroyRing( _vddi );
      if (_vssi == NULL) cerr << Error("Missing <vssi>/<vss> net (for pads) at top level." ) << endl;
      else destroyRing( _vssi );
      if (_ck   == NULL) cerr << Warning("No <ck> net at (for pads) chip level." ) << endl;
      if (_cki  == NULL) cerr << Warning("No <cki> net at (for pads) chip level." ) << endl;
      else destroyRing( _cki );
    } else {
      _vddiPadNetName = "";
      _vssiPadNetName = "";
      _ckoPadNetName  = "";

      for( Net* net : _topCell->getNets() ) {
        Net::Type netType = net->getType();

        if (netType == Net::Type::CLOCK) {
          if (not net->isExternal()) continue;

          if (_ckoPadNetName.isEmpty()) {
            cmess1 << "     - Using <" << net->getName() << "> as internal (core) clock net." << endl;
            _ckoPadNetName = net->getName();
            _cko           = net;
            if (NetRoutingExtension::isMixedPreRoute(net)) {
              cmess1 << "       (core clock net is already routed)" << endl;
              _flags |= ClockIsRouted;
            } else {
              cmess1 << "       (core clock net will be routed as an ordinary signal)" << endl;
            }
          } else {
            cerr << Error("Second clock net <%s> net at top block level will be ignored.\n"
                          "        (will consider only <%s>)"
                         , getString(net ->getName()).c_str()
                         , getString(_cko->getName()).c_str()
                         ) << endl;
          }
        }

        if (NetRoutingExtension::isManualGlobalRoute(net)) continue;
        if (NetRoutingExtension::isManualDetailRoute(net)) continue;

        if (netType == Net::Type::POWER) {
          if (_vddiPadNetName.isEmpty()) {
            _vddiPadNetName = net->getName();
            _vddi           = net;
          } else {
            cerr << Error("Second power supply net <%s> net at top block level will be ignored.\n"
                          "        (will consider only <%s>)"
                         , getString(net  ->getName()).c_str()
                         , getString(_vddi->getName()).c_str()
                         ) << endl;
          }
        }

        if (netType == Net::Type::GROUND) {
          if (_vssiPadNetName.isEmpty()) {
            _vssiPadNetName = net->getName();
            _vssi           = net;
          } else {
            cerr << Error("Second power ground net <%s> net at top block level will be ignored.\n"
                          "        (will consider only <%s>)"
                         , getString(net  ->getName()).c_str()
                         , getString(_vssi->getName()).c_str()
                         ) << endl;
          }
        }
      }
      
      if (_vddi == NULL) cerr << Error("Missing <vdd> net at top block level." ) << endl;
      else destroyRing( _vddi );
      if (_vssi == NULL) cerr << Error("Missing <vss> net at top block level." ) << endl;
      else destroyRing( _vssi );
    }

    if (_cko  == NULL) cparanoid << Warning("No clock net at top level." ) << endl;
  }


  bool  GlobalNetTable::guessGlobalNet ( const Name& name, Net* net )
  {
    if (name == _vddePadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as corona (external:vdde) power net." << endl;
      _vdde = net;
      return true;
    }

    if (name == _vddiPadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as core (internal:vddi) power net." << endl;
      _vddi = net;
      return true;
    }

    if (name == _vssePadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as corona (external:vsse) ground net." << endl;
      _vsse = net;
      return true;
    }

    if (name == _vssiPadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as core (internal:vssi) ground net." << endl;
      _vssi = net;
      return true;
    }

    if (name == _ckiPadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as corona (external:cki) clock net." << endl;
      _cki = net;
      return true;
    }

    if (name == _ckoPadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as core (internal:cko) clock net." << endl;
      _cko = net;

      if (NetRoutingExtension::isMixedPreRoute(_cko)) {
        cmess1 << "          (core clock net is already routed)" << endl;
        _flags |= ClockIsRouted;
      } else {
        cmess1 << "          (core clock net will be routed as an ordinary signal)" << endl;
      }
      return true;
    }

    if (name == _ckPadNetName) {
      cmess1 << "        - Using <" << net->getName() << "> as external chip clock net." << endl;
      _ck = net;
      return true;
    }

    return false;
  }


  Net* GlobalNetTable::getRootNet ( const Net* net, Path path ) const
  {
    cdebug_log(159,0) << "    getRootNet:" << path << ":" << net << endl;

    if (net == _blockage) return _blockage;

    if (_vdde and (net->getName() == _vdde->getName())) return _vdde;
    if (_vsse and (net->getName() == _vsse->getName())) return _vsse;

    if (net->getType() == Net::Type::POWER ) return _vddi;
    if (net->getType() == Net::Type::GROUND) return _vssi;
    if (net->getType() != Net::Type::CLOCK ) {
      return NULL;
    }

  // Track up, *only* for clocks.
    const Net* upNet = net;

    if (not path.isEmpty()) {
      DeepNet* deepClockNet = getTopCell()->getDeepNet( path, net );
      if (deepClockNet) {
        cdebug_log(159,0) << "    Deep Clock Net:" << deepClockNet
                    << " state:" << NetRoutingExtension::getFlags(deepClockNet) << endl;

        return NetRoutingExtension::isFixed(deepClockNet) ? _blockage : NULL;
      } else {
        cdebug_log(159,0) << "    Top Clock Net:" << net
                    << " state:" << NetRoutingExtension::getFlags(net) << endl;
      }

      Path       upPath   = path;
      Instance*  instance = NULL;
      Plug*      plug     = NULL;

      while ( true ) {
        cdebug_log(159,0) << "      " << path << "+" << upNet << endl;

        if (path.isEmpty()) break;
        if ((upNet == NULL) or not upNet->isExternal()) return _blockage;

        instance = path.getTailInstance();
        plug     = instance->getPlug(upNet);
        if (plug == NULL) return NULL;

        upNet = plug->getNet();
        path  = path.getHeadPath();
      }
    }

    cdebug_log(159,0) << "      Check againts top clocks ck:" << ((_ck) ? _ck->getName() : "NULL")
                << " cki:" << ((_cki) ? _cki->getName() : "NULL")
                << " cko:" << ((_cko) ? _cko->getName() : "NULL")
                << endl;

    if (_ck  and (upNet->getName() == _ck->getName() )) return _ck;
    if (_cki and (upNet->getName() == _cki->getName())) return _cki;

    if (_cko) {
      if (upNet->getName() == _cko->getName()) {
        if (isCoreClockNetRouted(upNet)) return _cko;
      }
    }

    return NetRoutingExtension::isFixed(upNet) ? _blockage : NULL;
  }


  bool  GlobalNetTable::isCoreClockNetRouted ( const Net* net ) const
  { return (net == _cko) and (_flags & ClockIsRouted); }


// -------------------------------------------------------------------
// Class  :  "::PowerRailsPlanes".

  class PowerRailsPlanes {
    public:
      class Rails;
      class Plane;

      class Rail {
        public:
                               Rail              ( Rails*, DbU::Unit axis, DbU::Unit width );
          inline DbU::Unit     getAxis           () const;
          inline DbU::Unit     getWidth          () const;
          inline Rails*        getRails          () const;
          inline RoutingPlane* getRoutingPlane   () const;
          inline unsigned int  getDirection      () const;
          inline Net*          getNet            () const;
                 void          merge             ( DbU::Unit source, DbU::Unit target );
                 void          doLayout          ( const Layer* );
                 string        _getString        () const;
        private:
          Rails*         _rails;
          DbU::Unit      _axis;
          DbU::Unit      _width;
          list<Interval> _chunks;
      };

    private:
      class RailCompare {
        public:
          bool operator() ( const Rail* lhs, const Rail* rhs );
      };

      class RailMatch : public unary_function<Rail*,bool> {
        public:
          inline      RailMatch  ( DbU::Unit axis, DbU::Unit width );
          inline bool operator() ( const Rail* );
        private:
          DbU::Unit  _axis;
          DbU::Unit  _width;
      };

    public:
      class Rails {
        public:
                               Rails             ( Plane*, unsigned int direction, Net* );
                              ~Rails             ();
          inline Plane*        getPlane          ();
          inline RoutingPlane* getRoutingPlane   ();
          inline unsigned int  getDirection      () const;
          inline Net*          getNet            () const;
                 void          merge             ( const Box& );
                 void          doLayout          ( const Layer* );
        private:
          Plane*         _plane;
          unsigned int   _direction;
          Net*           _net;
          vector<Rail*>  _rails;
      };

    public:
      class Plane {
        public:
          typedef  map<Net*,Rails*,Net::CompareById>  RailsMap;
        public:
                               Plane             ( const Layer*, RoutingPlane* );
                              ~Plane             ();
          inline const Layer*  getLayer          () const;
          inline RoutingPlane* getRoutingPlane   ();
          inline unsigned int  getDirection      () const;
          inline unsigned int  getPowerDirection () const;
                 void          merge             ( const Box&, Net* );
                 void          doLayout          ();
        private:
          const Layer*         _layer;
          RoutingPlane*        _routingPlane;
          RailsMap             _horizontalRails;
          RailsMap             _verticalRails;
          unsigned int         _powerDirection;
      };

    public:
      typedef  map<const BasicLayer*,Plane*,BasicLayer::CompareByMask>  PlanesMap;
    public:
                    PowerRailsPlanes       ( KiteEngine* );
                   ~PowerRailsPlanes       ();
      inline Net*   getRootNet             ( Net*, Path );
      inline bool   isCoreClockNetRouted   ( const Net* ) const;
             bool   hasPlane               ( const BasicLayer* );
             bool   setActivePlane         ( const BasicLayer* );
      inline Plane* getActivePlane         () const;
      inline Plane* getActiveBlockagePlane () const;
             void   merge                  ( const Box&, Net* );
             void   doLayout               ();
    private:
      KiteEngine*     _kite;
      GlobalNetTable  _globalNets;
      PlanesMap       _planes;
      Plane*          _activePlane;
      Plane*          _activeBlockagePlane;
  };


  PowerRailsPlanes::Rail::Rail ( Rails* rails, DbU::Unit axis, DbU::Unit width )
    : _rails (rails)
    , _axis  (axis)
    , _width (width)
    , _chunks()
  {
    cdebug_log(159,0) << "    new Rail "
                << " @" << DbU::getValueString(axis)
                << " " << getRoutingPlane()->getLayer()->getName()
                << " " << getRails()->getNet()
                << " " << ((getDirection()==KbHorizontal) ? "Horizontal" : "Vertical")<< endl;
  }

  inline DbU::Unit                PowerRailsPlanes::Rail::getAxis          () const { return _axis; }
  inline DbU::Unit                PowerRailsPlanes::Rail::getWidth         () const { return _width; }
  inline PowerRailsPlanes::Rails* PowerRailsPlanes::Rail::getRails         () const { return _rails; }
  inline RoutingPlane*            PowerRailsPlanes::Rail::getRoutingPlane  () const { return _rails->getRoutingPlane(); }
  inline unsigned int             PowerRailsPlanes::Rail::getDirection     () const { return _rails->getDirection(); }
  inline Net*                     PowerRailsPlanes::Rail::getNet           () const { return _rails->getNet(); }


  void  PowerRailsPlanes::Rail::merge ( DbU::Unit source, DbU::Unit target )
  {
    Interval chunkToMerge ( source, target );
    cdebug_log(159,0) << "    Rail::merge() "
                << ((getDirection()==KbHorizontal) ? "Horizontal" : "Vertical")
                << " " << chunkToMerge << endl;
    cdebug_log(159,0) << "    | " << _getString() << endl;

    list<Interval>::iterator imerge = _chunks.end();
    list<Interval>::iterator ichunk = _chunks.begin();

    while ( ichunk != _chunks.end() ) {
      if (imerge == _chunks.end()) {
        if (chunkToMerge.getVMax() < (*ichunk).getVMin()) {
          cdebug_log(159,0) << "    | Insert before " << *ichunk << endl;
          imerge = _chunks.insert( ichunk, chunkToMerge );
          break;
        }

        if (chunkToMerge.intersect(*ichunk)) {
          cdebug_log(159,0) << "    | Merge with " << *ichunk << endl;
          imerge = ichunk;
          (*imerge).merge( chunkToMerge );
        }
      } else {
        if (chunkToMerge.getVMax() >= (*ichunk).getVMin()) {
          (*imerge).merge( *ichunk );
          cdebug_log(159,0) << "    | Absorb (erase) " << *ichunk << endl;
          ichunk = _chunks.erase( ichunk );
          continue;
        } else
          break;
      }

      // if (chunkToMerge.intersect(*ichunk)) {
      //   if (imerge == _chunks.end()) {
      //     cdebug_log(159,0) << "    | Merge with " << *ichunk << endl;
      //     imerge = ichunk;
      //     (*imerge).merge( chunkToMerge );
      //   } else {
      //     (*imerge).merge( *ichunk );
      //     cdebug_log(159,0) << "    | Absorb (erase) " << *ichunk << endl;
      //     ichunk = _chunks.erase( ichunk );
      //     continue;
      //   }
      // }
      ++ichunk;
    }

    if (imerge == _chunks.end()) {
      _chunks.insert( ichunk, chunkToMerge );
      cdebug_log(159,0) << "    | Insert at end " << DbU::getValueString(_axis) << " " << chunkToMerge << endl;
      cdebug_log(159,0) << "    | " << _getString() << endl;
    }
  }


  void  PowerRailsPlanes::Rail::doLayout ( const Layer* layer )
  {
    cdebug_log(159,0) << "Doing layout of rail: "
                << " " << layer->getName()
                << " " << ((getDirection()==KbHorizontal) ? "Horizontal" : "Vertical")
                << " @" << DbU::getValueString(_axis) << endl;
    cdebug_log(159,0) << _getString() << endl;

    Net*          net       = getNet();
    RoutingPlane* plane     = getRoutingPlane();
    Segment*      segment   = NULL;
  //DbU::Unit     delta     =   plane->getLayerGauge()->getPitch()
  //                          - plane->getLayerGauge()->getHalfWireWidth()
  //                          - DbU::fromLambda(0.1);
    DbU::Unit     delta     =   plane->getLayerGauge()->getObstacleDw() - DbU::fromLambda(0.1);
    DbU::Unit     extension = layer->getExtentionCap();
  //DbU::Unit     extension = Session::getExtentionCap();
  //unsigned int  type      = plane->getLayerGauge()->getType();
    const Box&    coronaBb  = plane->getKiteEngine()->getChipTools().getCoronaBb();
    DbU::Unit     axisMin   = 0;
    DbU::Unit     axisMax   = 0;

    cdebug_log(159,0) << "  delta:" << DbU::getValueString(delta)
                << " (pitch:" << DbU::getValueString(plane->getLayerGauge()->getPitch())
                << " , ww/2:" << DbU::getValueString(plane->getLayerGauge()->getHalfWireWidth())
                << ")" << endl;

    // if ( type == Constant::PinOnly ) {
    //   cdebug_log(159,0) << "  Layer is PinOnly." << endl;
    //   return;
    // }

    if ( getDirection() == KbHorizontal ) {
      list<Interval>::iterator ichunk     = _chunks.begin();
      list<Interval>::iterator ichunknext = ichunk;
      ++ichunknext;

      for ( ; ichunk != _chunks.end() ; ++ichunk, ++ichunknext ) {

        if (ichunknext != _chunks.end()) {
          if ((*ichunk).intersect(*ichunknext))
            cerr << Error( "Overlaping consecutive chunks in %s %s Rail @%s:\n"
                           "  %s"
                         , getString(layer->getName()).c_str()
                         , ((getDirection()==KbHorizontal) ? "Horizontal" : "Vertical")
                         , DbU::getValueString(_axis).c_str()
                         , _getString().c_str()
                         ) << endl;
        }
        
        cdebug_log(159,0) << "  chunk: [" << DbU::getValueString((*ichunk).getVMin())
                    << ":" << DbU::getValueString((*ichunk).getVMax()) << "]" << endl;

        segment = Horizontal::create ( net
                                     , layer
                                     , _axis
                                     , _width
                                     , (*ichunk).getVMin()+extension
                                     , (*ichunk).getVMax()-extension
                                     );
        if ( segment and net->isExternal() )
          NetExternalComponents::setExternal ( segment );

        axisMin = _axis - _width/2;
        axisMax = _axis + _width/2;
        if (coronaBb.contains(segment->getBoundingBox())) {
          axisMin -= delta;
          axisMax += delta;
        }

        Track* track = plane->getTrackByPosition ( axisMin, Constant::Superior );
        for ( ; track and (track->getAxis() <= axisMax) ; track = track->getNextTrack() ) {
          TrackElement* element = TrackFixedSegment::create ( track, segment );
          cdebug_log(159,0) << "  Insert in " << track << "+" << element << endl;
        }
      }
    } else {
      list<Interval>::iterator ichunk = _chunks.begin();
      for ( ; ichunk != _chunks.end() ; ichunk++ ) {
        cdebug_log(159,0) << "  chunk: [" << DbU::getValueString((*ichunk).getVMin())
                    << ":" << DbU::getValueString((*ichunk).getVMax()) << "]" << endl;

        segment = Vertical::create ( net
                                   , layer
                                   , _axis
                                   , _width
                                   , (*ichunk).getVMin()+extension
                                   , (*ichunk).getVMax()-extension
                                   );
        if ( segment and net->isExternal() )
          NetExternalComponents::setExternal ( segment );

        axisMin = _axis - _width/2;
        axisMax = _axis + _width/2;
      //if (coronaBb.contains(segment->getBoundingBox())) {
          axisMin -= delta;
          axisMax += delta;
      //}

        cdebug_log(159,0) << "  axisMin:" << DbU::getValueString(axisMin)
                    <<  " axisMax:" << DbU::getValueString(axisMax) << endl;

        Track* track = plane->getTrackByPosition ( axisMin, Constant::Superior );
        for ( ; track and (track->getAxis() <= axisMax) ; track = track->getNextTrack() ) {
          TrackElement* element = TrackFixedSegment::create ( track, segment );
          cdebug_log(159,0) << "  Insert in " << track
                      << "+" << element
                      << " " << (net->isExternal() ? "external" : "internal")
                      << endl;
        }
      }
    }
  }


  string   PowerRailsPlanes::Rail::_getString () const
  {
    ostringstream os;

    os << "<Rail " << ((getDirection()==KbHorizontal) ? "Horizontal" : "Vertical")
       << " @"  << DbU::getValueString(_axis)  << " "
       << " w:" << DbU::getValueString(_width) << " ";
    list<Interval>::const_iterator ichunk = _chunks.begin();
    for ( ; ichunk != _chunks.end() ; ++ichunk ) {
      if (ichunk != _chunks.begin()) os << " ";
      os << "[" << DbU::getValueString((*ichunk).getVMin())
         << " " << DbU::getValueString((*ichunk).getVMax()) << "]";
    }
    os << ">";
    return os.str();
  }


  inline bool  PowerRailsPlanes::RailCompare::operator() ( const Rail* lhs, const Rail* rhs )
  {
    if ( lhs->getAxis () < rhs->getAxis () ) return true;
    if ( lhs->getWidth() < rhs->getWidth() ) return true;
    return false;
  }


  inline  PowerRailsPlanes::RailMatch::RailMatch ( DbU::Unit axis, DbU::Unit width )
    : _axis(axis)
    , _width(width)
  { }


  inline bool  PowerRailsPlanes::RailMatch::operator() ( const Rail* rail )
  { return (rail->getAxis() == _axis) and (rail->getWidth() == _width); }


  PowerRailsPlanes::Rails::Rails ( PowerRailsPlanes::Plane* plane , unsigned int direction , Net* net )
    : _plane         (plane)
    , _direction     (direction)
    , _net           (net)
    , _rails         ()
  {
    cdebug_log(159,0) << "  new Rails @"
                << " " << getRoutingPlane()->getLayer()->getName()
                << " " << net
                << " " << ((getDirection()==KbHorizontal) ? "Horizontal": "Vertical") << endl;
  }


  PowerRailsPlanes::Rails::~Rails ()
  {
    while ( not _rails.empty() ) {
      delete (*_rails.begin());
      _rails.erase ( _rails.begin() );
    }
  }


  inline PowerRailsPlanes::Plane* PowerRailsPlanes::Rails::getPlane          () { return _plane; }
  inline RoutingPlane*            PowerRailsPlanes::Rails::getRoutingPlane   () { return getPlane()->getRoutingPlane(); }
  inline unsigned int             PowerRailsPlanes::Rails::getDirection      () const { return _direction; }
  inline Net*                     PowerRailsPlanes::Rails::getNet            () const { return _net; }


  void   PowerRailsPlanes::Rails::merge ( const Box& bb )
  {
    DbU::Unit  axis;
    DbU::Unit  width;
    DbU::Unit  sourceU;
    DbU::Unit  targetU;

    if (getDirection() == KbHorizontal) {
      axis    = bb.getYCenter();
      width   = bb.getHeight();
      sourceU = bb.getXMin();
      targetU = bb.getXMax();
    } else {
      axis    = bb.getXCenter();
      width   = bb.getWidth();
      sourceU = bb.getYMin();
      targetU = bb.getYMax();
    }

    vector<Rail*>::iterator irail = find_if ( _rails.begin(), _rails.end(), RailMatch(axis,width) );

    Rail* rail = NULL;
    if ( irail == _rails.end() ) {
      rail = new Rail(this,axis,width);
      _rails.push_back ( rail );
      stable_sort ( _rails.begin(), _rails.end(), RailCompare() );
    } else {
      rail = *irail;
    }

    rail->merge ( sourceU, targetU );
  }


  void  PowerRailsPlanes::Rails::doLayout ( const Layer* layer )
  {
    cdebug_log(159,0) << "Doing layout of rails: " << layer->getName()
                << " " << ((_direction==KbHorizontal) ? "Horizontal" : "Vertical")
                << " " << _net->getName() << endl;

    for ( size_t irail=0 ; irail<_rails.size() ; irail++ )
      _rails[irail]->doLayout ( layer );
  }


  PowerRailsPlanes::Plane::Plane ( const Layer* layer, RoutingPlane* routingPlane )
    : _layer                (layer)
    , _routingPlane         (routingPlane)
    , _horizontalRails      ()
    , _verticalRails        ()
    , _powerDirection       (routingPlane->getDirection())
  {
    cdebug_log(159,0) << "New Plane " << _layer->getName() << " " << _routingPlane << endl;

  // Hard-coded SxLib gauge.
    if (_routingPlane->getDepth() == 0) _powerDirection = KbHorizontal;
  }


  PowerRailsPlanes::Plane::~Plane ()
  {
    RailsMap::iterator irail = _horizontalRails.begin();
    for ( ; irail != _horizontalRails.end() ; ++irail ) {
      delete (*irail).second;
    }
    irail = _verticalRails.begin();
    for ( ; irail != _verticalRails.end() ; ++irail ) {
      delete (*irail).second;
    }
  }


  inline const Layer*  PowerRailsPlanes::Plane::getLayer          () const { return _layer; }
  inline RoutingPlane* PowerRailsPlanes::Plane::getRoutingPlane   () { return _routingPlane; }
  inline unsigned int  PowerRailsPlanes::Plane::getDirection      () const { return _routingPlane->getDirection(); }
  inline unsigned int  PowerRailsPlanes::Plane::getPowerDirection () const { return _powerDirection; }


  void  PowerRailsPlanes::Plane::merge ( const Box& bb, Net* net )
  {
    Rails* rails = NULL;

    cdebug_log(159,0) << "    Plane::merge() " << net->getName() << " " << bb << endl;

    unsigned int direction = getDirection();
    if ( (net->getType() == Net::Type::POWER) or (net->getType() == Net::Type::GROUND) )
      direction = getPowerDirection();

    if (direction == KbHorizontal) {
      RailsMap::iterator irails = _horizontalRails.find(net);
      if ( irails == _horizontalRails.end() ) {
        rails = new Rails(this,KbHorizontal,net);
        _horizontalRails.insert ( make_pair(net,rails) );
      } else
        rails = (*irails).second;

      rails->merge ( bb );
    } else {
      RailsMap::iterator irails = _verticalRails.find(net);
      if ( irails == _verticalRails.end() ) {
        rails = new Rails(this,KbVertical,net);
        _verticalRails.insert ( make_pair(net,rails) );
      } else
        rails = (*irails).second;

      cdebug_log(159,0) << "    Vertical Merging" << endl; 
      rails->merge ( bb );
    }
  }


  void  PowerRailsPlanes::Plane::doLayout ()
  {
    cdebug_log(159,0) << "Doing layout of plane: " << _layer->getName() << endl;

    RailsMap::iterator irails = _horizontalRails.begin();
    for ( ; irails != _horizontalRails.end() ; ++irails ) {
      (*irails).second->doLayout(_layer);
    }
    irails = _verticalRails.begin();
    for ( ; irails != _verticalRails.end() ; ++irails ) {
      (*irails).second->doLayout(_layer);
    }
  }


  PowerRailsPlanes::PowerRailsPlanes ( KiteEngine* kite )
    : _kite               (kite)
    , _globalNets         (kite)
    , _planes             ()
    , _activePlane        (NULL)
    , _activeBlockagePlane(NULL)
  {
    _globalNets.setBlockage( kite->getBlockageNet() );

    Technology*   technology = DataBase::getDB()->getTechnology();
    RoutingGauge* rg         = _kite->getConfiguration()->getRoutingGauge();

    for( Layer* layer : technology->getLayers() ) {
      RegularLayer* regular = dynamic_cast<RegularLayer*>(layer);
      if ( not regular
         or (regular->getBasicLayer()->getMaterial() != BasicLayer::Material::metal) ) continue;

      RoutingLayerGauge* lg = rg->getLayerGauge(regular);
      if ( not lg ) continue;

      cdebug_log(159,0) << "Gauge: [" << lg->getDepth() << "] " << lg << endl;

      RoutingPlane* rp = _kite->getRoutingPlaneByIndex(lg->getDepth());
      cdebug_log(159,0) << "Plane:"  << rp << endl;

      _planes.insert( make_pair(regular->getBasicLayer(),new Plane(regular,rp)) );

      if (lg->getType() == Constant::PinOnly) continue;
      const BasicLayer* blockageLayer = regular->getBasicLayer()->getBlockageLayer();
      if (not blockageLayer) continue;

      _planes.insert( make_pair(blockageLayer,new Plane(blockageLayer,rp)) );
    }
  }


  PowerRailsPlanes::~PowerRailsPlanes ()
  {
    while ( not _planes.empty() ) {
      delete _planes.begin()->second;
      _planes.erase ( _planes.begin() );
    }
  }

  
  inline Net* PowerRailsPlanes::getRootNet ( Net* net, Path path )
  { return _globalNets.getRootNet(net,path); }

  
  inline bool  PowerRailsPlanes::isCoreClockNetRouted ( const Net* net ) const
  { return _globalNets.isCoreClockNetRouted(net); }


  bool  PowerRailsPlanes::hasPlane ( const BasicLayer* layer )
  { return (_planes.find(layer) != _planes.end()); }


  bool  PowerRailsPlanes::setActivePlane ( const BasicLayer* layer )
  {
    PlanesMap::iterator iplane = _planes.find(layer);
    if (iplane == _planes.end()) return false;

    _activePlane         = iplane->second;
    _activeBlockagePlane = NULL;
    if (layer->getMaterial() != BasicLayer::Material::blockage) {
      BasicLayer* blockageLayer = layer->getBlockageLayer();
      PlanesMap::iterator ibplane = _planes.find(blockageLayer);
      if (ibplane != _planes.end())
        _activeBlockagePlane = ibplane->second;
    }
    return true;
  }


  inline PowerRailsPlanes::Plane* PowerRailsPlanes::getActivePlane () const
  { return _activePlane; }


  inline PowerRailsPlanes::Plane* PowerRailsPlanes::getActiveBlockagePlane () const
  { return _activeBlockagePlane; }


  void  PowerRailsPlanes::merge ( const Box& bb, Net* net )
  {
    if (not _activePlane) return;

    Net* topGlobalNet = _globalNets.getRootNet( net, Path() );
    if (topGlobalNet == NULL) {
      cdebug_log(159,0) << "Not a global net: " << net << endl;
      return;
    }

    if ( (topGlobalNet == _globalNets.getBlockage()) and (_activeBlockagePlane != NULL) )
      _activeBlockagePlane->merge( bb, topGlobalNet );
    else
      _activePlane->merge( bb, topGlobalNet );
  }


  void  PowerRailsPlanes::doLayout ()
  {
    PlanesMap::iterator iplane = _planes.begin();
    for ( ; iplane != _planes.end() ; iplane++ )
      iplane->second->doLayout ();
  }



// -------------------------------------------------------------------
// Class  :  "::QueryPowerRails".

  class QueryPowerRails : public Query {
    public:
                            QueryPowerRails     ( KiteEngine* );
      virtual bool          hasGoCallback       () const;
      virtual void          setBasicLayer       ( const BasicLayer* );
      virtual bool          hasBasicLayer       ( const BasicLayer* );
      virtual void          goCallback          ( Go*     );
      virtual void          rubberCallback      ( Rubber* );
      virtual void          extensionGoCallback ( Go*     );
      virtual void          masterCellCallback  ();
              void          addToPowerRail      ( const Go*              go
                                                , const BasicLayer*      basicLayer
                                                , const Box&             area
                                                , const Transformation&  transformation
                                                );
              void          ringAddToPowerRails ();
      virtual void          doQuery             ();
      inline  void          doLayout            ();
      inline  unsigned int  getGoMatchCount     () const;
    private:
      AllianceFramework*      _framework;
      KiteEngine*             _kite;
      RoutingGauge*           _routingGauge;
      const ChipTools&        _chipTools;
      PowerRailsPlanes        _powerRailsPlanes;
      bool                    _isBlockagePlane;
      vector<const Segment*>  _hRingSegments;
      vector<const Segment*>  _vRingSegments;
      unsigned int            _goMatchCount;
  };


  QueryPowerRails::QueryPowerRails ( KiteEngine* kite )
    : Query            ()
    , _framework       (AllianceFramework::get())
    , _kite            (kite)
    , _routingGauge    (kite->getConfiguration()->getRoutingGauge())
    , _chipTools       (kite->getChipTools())
    , _powerRailsPlanes(kite)
    , _isBlockagePlane (false)
    , _hRingSegments   ()
    , _vRingSegments   ()
    , _goMatchCount    (0)
  {
    setCell       ( kite->getCell() );
    setArea       ( kite->getCell()->getBoundingBox() );
    setBasicLayer ( NULL );
    setFilter     ( Query::DoTerminalCells|Query::DoComponents );

    cmess1 << "  o  Building power rails." << endl;
  }


  inline  unsigned int  QueryPowerRails::getGoMatchCount () const
  { return _goMatchCount; }


  inline  void  QueryPowerRails::doLayout ()
  { return _powerRailsPlanes.doLayout(); }


  bool  QueryPowerRails::hasBasicLayer ( const BasicLayer* basicLayer )
  { return _powerRailsPlanes.hasPlane ( basicLayer ); }


  void  QueryPowerRails::setBasicLayer ( const BasicLayer* basicLayer )
  {
    _isBlockagePlane = (basicLayer) and (basicLayer->getMaterial() == BasicLayer::Material::blockage);
    _powerRailsPlanes.setActivePlane ( basicLayer );
    Query::setBasicLayer ( basicLayer );
  }


  void  QueryPowerRails::doQuery ()
  {
    PowerRailsPlanes::Plane* activePlane = _powerRailsPlanes.getActivePlane();

    if (not activePlane) return;

    cmess1 << "     - PowerRails in " << activePlane->getLayer()->getName() << " ..." << endl;
    Query::doQuery();
  }


  void  QueryPowerRails::masterCellCallback ()
  { }


  bool  QueryPowerRails::hasGoCallback () const
  { return true; }


  void  QueryPowerRails::goCallback ( Go* go )
  {
  //ltrace(80) << "QueryPowerRails::goCallback() " << go->getId() << ":" << go
  //           << " " << getPath().getName() << endl;
    addToPowerRail ( go, getBasicLayer(), getArea(), getTransformation() );
  }


  void  QueryPowerRails::addToPowerRail ( const Go*              go
                                        , const BasicLayer*      basicLayer
                                        , const Box&             area
                                        , const Transformation&  transformation
                                        )
  {
    const Component* component = dynamic_cast<const Component*>(go);
    if ( component ) {
      if (    _framework->isPad(getMasterCell())
         and ( (_routingGauge->getLayerDepth(component->getLayer()) < 2)
             or (component->getLayer()->getBasicLayers().getFirst()->getMaterial() != BasicLayer::Material::blockage) ) )
        return;

      Net* rootNet = _kite->getBlockageNet();
      if (not _isBlockagePlane) {
        rootNet = _powerRailsPlanes.getRootNet(component->getNet(),getPath());
      }

#if 0
      Net* rootNet = NULL;
      if ( not _isBlockagePlane )
        rootNet = _powerRailsPlanes.getRootNet(component->getNet(),getPath());
      else {
        rootNet = _kite->getBlockageNet();
      }
#endif

      if ( rootNet == NULL ) {
        cdebug_log(159,0) << "  rootNet is NULL, not taken into account." << endl;
        return;
      }

      cdebug_log(159,0) << "  rootNet " << rootNet << " (" << rootNet->isClock() << ") "
                  << go->getCell() << " (" << go->getCell()->isTerminal() << ")" << endl;

      const Segment* segment = dynamic_cast<const Segment*>(component);
      if ( segment != NULL ) {
        _goMatchCount++;
        cdebug_log(159,0) << "  Merging PowerRail element: " << segment << endl;

        Box bb = segment->getBoundingBox ( basicLayer );

        unsigned int depth = _routingGauge->getLayerDepth ( segment->getLayer() );

        if (    _chipTools.isChip()
           and ((depth == 2) or (depth == 3))
           and (segment->getWidth () == _chipTools.getPadPowerWidth())
           and (segment->getLength() >  _chipTools.getPadWidth())
           and (_kite->getChipTools().getCorona().contains(bb)) ) {
          switch ( depth ) {
            case 2: _vRingSegments.push_back ( segment ); break; // M3 V.
            case 3: _hRingSegments.push_back ( segment ); break; // M4 H.
          }
          return;
        }

        transformation.applyOn ( bb );

        _powerRailsPlanes.merge ( bb, rootNet );
      } else {
        const Contact* contact = dynamic_cast<const Contact*>(component);
        if ( contact != NULL ) {
          _goMatchCount++;

          Box bb = contact->getBoundingBox ( basicLayer );
          transformation.applyOn ( bb );

          cdebug_log(159,0) << "  Merging PowerRail element: " << contact << " bb:" << bb
                      << " " << basicLayer << endl;
          
          _powerRailsPlanes.merge ( bb, rootNet );
        }
      }
    }
  }


  void  QueryPowerRails::ringAddToPowerRails ()
  {
    if ( not _hRingSegments.empty() ) {
      const RegularLayer* layer = dynamic_cast<const RegularLayer*>(_routingGauge->getRoutingLayer(3));
      setBasicLayer ( layer->getBasicLayer() );

      DbU::Unit   xmin = DbU::Max;
      DbU::Unit   xmax = DbU::Min;
      vector<Box> boxes;

      for ( size_t i=0 ; i<_hRingSegments.size() ; ++i ) {
        boxes.push_back ( _hRingSegments[i]->getBoundingBox() );
        xmin = std::min ( xmin, boxes.back().getXMin() );
        xmax = std::max ( xmax, boxes.back().getXMax() );
      }

      for ( size_t i=0 ; i<_hRingSegments.size() ; ++i ) {
        _powerRailsPlanes.merge ( Box(xmin,boxes[i].getYMin(),xmax,boxes[i].getYMax())
                                , _powerRailsPlanes.getRootNet(_hRingSegments[i]->getNet(),Path()) );
      }
    }

    if ( not _vRingSegments.empty() ) {
      const RegularLayer* layer = dynamic_cast<const RegularLayer*>(_routingGauge->getRoutingLayer(2));
      setBasicLayer ( layer->getBasicLayer() );

      DbU::Unit   ymin = DbU::Max;
      DbU::Unit   ymax = DbU::Min;
      vector<Box> boxes;

      for ( size_t i=0 ; i<_vRingSegments.size() ; ++i ) {
        boxes.push_back ( _vRingSegments[i]->getBoundingBox() );
        ymin = std::min ( ymin, boxes.back().getYMin() );
        ymax = std::max ( ymax, boxes.back().getYMax() );
      }

      for ( size_t i=0 ; i<_vRingSegments.size() ; ++i ) {
        _powerRailsPlanes.merge ( Box(boxes[i].getXMin(),ymin,boxes[i].getXMax(),ymax)
                                , _powerRailsPlanes.getRootNet(_vRingSegments[i]->getNet(),Path()) );
      }
    }
  }


  void  QueryPowerRails::rubberCallback ( Rubber* )
  { }


  void  QueryPowerRails::extensionGoCallback ( Go* )
  { }


} // End of anonymous namespace.


namespace Kite {


  using Hurricane::DataBase;
  using Hurricane::Technology;
  using Hurricane::BasicLayer;
  using Hurricane::ForEachIterator;
  using Katabatic::NetRoutingState;


  void  KiteEngine::buildPowerRails ()
  {
  //DebugSession::open( 150, 160 );

    if (not _blockageNet) {
      _blockageNet = getCell()->getNet("blockagenet");
      if (not _blockageNet) {
        _blockageNet = Net::create( getCell(), "blockagenet" );
        _blockageNet->setType( Net::Type::BLOCKAGE );
      }

      NetRoutingState* state = getRoutingState( _blockageNet, Katabatic::KbCreate );
      state->setFlags( NetRoutingState::Fixed );
    }

    QueryPowerRails query ( this );
    Technology*     technology = DataBase::getDB()->getTechnology();

    for( BasicLayer* layer : technology->getBasicLayers() ) {
      if (   (layer->getMaterial() != BasicLayer::Material::metal)
         and (layer->getMaterial() != BasicLayer::Material::blockage) )
        continue;
      if (_configuration->isGMetal(layer)) continue;
      if (not query.hasBasicLayer(layer)) continue;

      query.setBasicLayer( layer );
      query.doQuery      ();
    }
    query.ringAddToPowerRails();
    query.doLayout();
    cmess1 << "     - " << query.getGoMatchCount() << " power rails elements found." << endl;

    vector<GCell*>& gcells = *(getGCellGrid()->getGCellVector());
    for ( auto gcell : gcells ) {
      gcell->truncDensities();
    }

    Session::revalidate ();

  //DebugSession::close();
  }


} // Kite namespace.
