// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) Sorbonne Université 2007-2023, All Rights Reserved
//
// +-----------------------------------------------------------------+ 
// |                   C O R I O L I S                               |
// |       T r a m o n t a n a  -  Extractor & LVX                   |
// |                                                                 |
// |  Algorithm    :                   Christian MASSON              |
// |  First impl.  :                           Yifei WU              |
// |  Second impl. :                   Jean-Paul CHAPUT              |
// |  E-mail       :           Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module   :  "./Tile.cpp"                                   |
// +-----------------------------------------------------------------+


#include <iomanip>
#include "hurricane/utilities/Path.h"
#include "hurricane/DebugSession.h"
#include "hurricane/UpdateSession.h"
#include "hurricane/Bug.h"
#include "hurricane/Error.h"
#include "hurricane/Warning.h"
#include "hurricane/Breakpoint.h"
#include "hurricane/Timer.h"
#include "hurricane/Layer.h"
#include "hurricane/Net.h"
#include "hurricane/Pad.h"
#include "hurricane/Plug.h"
#include "hurricane/Cell.h"
#include "hurricane/Instance.h"
#include "hurricane/Vertical.h"
#include "hurricane/Horizontal.h"
#include "hurricane/Diagonal.h"
#include "hurricane/Rectilinear.h"
#include "hurricane/Polygon.h"
#include "hurricane/RoutingPad.h"
#include "crlcore/Utilities.h"
#include "tramontana/Tile.h"
#include "tramontana/Equipotential.h"
#include "tramontana/SweepLine.h"


namespace Tramontana {

  using std::cout;
  using std::cerr;
  using std::endl;
  using std::dec;
  using std::setw;
  using std::setfill;
  using std::left;
  using std::string;
  using std::ostream;
  using std::ofstream;
  using std::ostringstream;
  using std::setprecision;
  using std::vector;
  using std::make_pair;
  using Hurricane::dbo_ptr;
  using Hurricane::UpdateSession;
  using Hurricane::DebugSession;
  using Hurricane::tab;
  using Hurricane::Bug;
  using Hurricane::Error;
  using Hurricane::Warning;
  using Hurricane::Breakpoint;
  using Hurricane::Box;
  using Hurricane::Layer;
  using Hurricane::Entity;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;
  using Hurricane::Vertical;
  using Hurricane::Diagonal;
  using Hurricane::Rectilinear;
  using Hurricane::Polygon;
  using Hurricane::Cell;
  using Hurricane::Instance;


// -------------------------------------------------------------------
// Class  :  "Tramontana::Tile".


  uint32_t       Tile::_idCounter = 0;
  uint32_t       Tile::_time      = 0;
  vector<Tile*>  Tile::_allocateds;


  Tile::Tile ( Occurrence occurrence, const BasicLayer* layer, const Box& boundingBox, Tile* parent )
    : _id           (_idCounter++)
    , _occurrence   (occurrence) 
    , _layer        (layer)
    , _boundingBox  (boundingBox)
    , _equipotential(nullptr)
    , _flags        (0)
    , _parent       (parent)
    , _rank         (0)
    , _timeStamp    (0)
  {
    cdebug_log(160,0) << "Tile::Tile() " << this << endl;
    _allocateds.push_back( this );
    if (occurrence.getPath().isEmpty() and not occurrence.getEntity())
      cerr << "Tile with empty occurrence!!" << endl;
  }


  Tile* Tile::create (       Occurrence  occurrence
                     , const BasicLayer* layer
                     ,       Tile*       rootTile
                     ,       SweepLine*  sweepLine
                     ,       uint32_t    flags )
  {
    Component* component = dynamic_cast<Component*>( occurrence.getEntity() );
    if (not component) {
      cerr << Error( "Tile::create(): Must be build over an occurrence of *Component*.\n"
                     "        (%s)"
                   , getString(occurrence).c_str()
                   ) << endl;
      return nullptr;
    }
    if (not (flags & ForceLayer) and not component->getLayer()->contains(layer)) {
      cerr << "Intersect:" << component->getLayer()->getMask().intersect(layer->getMask()) << endl;
      cerr << Error( "Tile::create(): Component layer \"%s\" does not contains \"%s\".\n"
                     "        (%s)\n"
                     "        component :%s\n"
                     "        basicLayer:%s"
                   , getString(component->getLayer()->getName()).c_str()
                   , getString(layer->getName()).c_str()
                   , getString(occurrence).c_str()
                   , getString(component->getLayer()->getMask()).c_str()
                   , getString(layer->getMask()).c_str()
                   ) << endl;
      return nullptr;
    }
    if (dynamic_cast<Polygon*>(component)) {
      cerr << Error( "Tile::create(): Polygon are not supported for extraction.\n"
                     "        (%s)"
                   , getString(occurrence).c_str()
                   ) << endl;
      return nullptr;
    }
    if (dynamic_cast<Diagonal*>(component)) {
      cerr << Error( "Tile::create(): Diagonal are not supported for extraction.\n"
                     "        (%s)"
                   , getString(occurrence).c_str()
                   ) << endl;
      return nullptr;
    }

    Occurrence childEqui = occurrence;
    if (not childEqui.getPath().isEmpty())
      childEqui = Equipotential::getChildEqui( occurrence );
    
    Rectilinear* rectilinear = dynamic_cast<Rectilinear*>( component );
    if (rectilinear) {
      if (not rectilinear->isRectilinear()) {
        cerr << Error( "Tile::create(): Rectilinear with 45/135 edges are not supported for extraction.\n"
                       "        (%s)"
                     , getString(occurrence).c_str()
                     ) << endl;
        return nullptr;
      }
      if (rectilinear->getId() == 3367) {
        DebugSession::open( 160, 169 );
        cdebug_log(160,0) << "Tiling: " << rectilinear << endl;
      }
      vector<Box> boxes;
      rectilinear->getAsRectangles( boxes );
      for ( Box bb : boxes ) {
        occurrence.getPath().getTransformation().applyOn( bb );
        Tile* tile = new Tile ( childEqui, layer, bb, rootTile );
        sweepLine->add( tile );
        cdebug_log(160,0) << "| " << tile << endl;
        if (not rootTile) rootTile = tile;
      }
      if (rectilinear->getId() == 3367) {
        DebugSession::close();
      }
      return rootTile;
    }

    Box bb = component->getBoundingBox( layer );
    occurrence.getPath().getTransformation().applyOn( bb );

    Tile* tile = new Tile ( childEqui, layer, bb, rootTile );
    sweepLine->add( tile );
    return tile;
  }


  void  Tile::destroy ()
  {
    cdebug_log(160,1) << "Tile::destroy() " << this << endl;
    cdebug_tabw(160,-1);
  }


  Tile::~Tile ()
  { }


  void  Tile::deleteAllTiles ()
  {
    for ( Tile* tile : _allocateds) delete tile;
    _allocateds.clear();
    _idCounter = 0;
  }

  // Net* Tile::getNet () const
  // { return dynamic_cast<Component*>( _occurrence.getEntity() )->getNet(); }


  Tile* Tile::getRoot ( uint32_t flags )
  {
    cdebug_log(160,1) << "Tile::getRoot() tid=" << getId() << " " << getOccurrence() << endl;
    cdebug_log(160,0) << "+ " << (getEquipotential() ? getString(getEquipotential()) : "equi=NULL") << endl;
    if (not getParent()) {
      if ((flags & MakeLeafEqui) and not getEquipotential()) {
        newEquipotential();
      }
      cdebug_tabw(160,-1);
      return this;
    }
    
    Tile* root = this;
    while ( root->getParent() ) {
      // if (flags & MergeEqui) {
      //   if (not root->getParent()->getEquipotential() and root->getEquipotential()) {
      //     cdebug_log(160,0) << "| tile has no equi, immediate merge" << endl;
      //     root->getParent()->setEquipotential( root->getEquipotential() );
      //     root->getEquipotential()->add( root->getParent()->getOccurrence ()
      //                                  , root->getParent()->getBoundingBox() );
      //     root->getParent()->setOccMerged( true );
      //   }
      // }
      root = root->getParent();
      cdebug_log(160,0) << "| parent tid=" << root->getId() << " " << root->getOccurrence() << endl;
    }
    cdebug_log(160,0) << "> root tid=" << root->getId() << " "
                      << (root->getEquipotential() ? getString(root->getEquipotential()) : "equi=NULL") << endl;


    if (flags & MergeEqui) {
      Equipotential* rootEqui = root->getEquipotential();
      if (not rootEqui) {
        rootEqui = root->newEquipotential();
      }

      Tile* current = this;
      while ((current != root) and current) {
        if (current->isUpToDate()) {
          cdebug_log(160,0) << "> Up to date current: tid=" << current->getId() << endl;
          break;
        }
        if (not current->isOccMerged()) {
          if (current->getEquipotential()) {
            if (current->getEquipotential() != rootEqui) {
              cdebug_log(160,0) << "| merge tid=" << current->getId() << " => tid=" << root->getId() << endl;
              cdebug_log(160,0) << "|       tid=" << current->getEquipotential() << endl;
              rootEqui->merge( current->getEquipotential() );
            }
          } else {
            cdebug_log(160,0) << "| add " << current->getOccurrence() << endl;
            rootEqui->add( current->getOccurrence(), _boundingBox );
          }
          current->setOccMerged( true );
          current->syncTime();
          cdebug_log(160,0) << "| current up to date: time=" << current->_timeStamp
                            << " " << current->isUpToDate() << endl;
        }
        current = current->getParent();
      }
    }

    if (flags & Compress) {
      Tile* current = this;
      while ( current != root ) {
        Tile* curParent = current->getParent();
        current->setParent( root );
        current = curParent;
      }
    }

    cdebug_tabw(160,-1);
    return root;
  }


  Tile* Tile::merge ( Tile* other )
  {
    cdebug_log(160,1) << "Tile::merge() this->tid:" << getId()
                      << " + other->tid:" << other->getId() << endl;
    Tile* root1 = getRoot( Compress|MergeEqui );
    Tile* root2 = other->getRoot( Compress|MergeEqui );
    if (root1 and (root1 == root2)) {
      cdebug_log(160,0) << "Already have same root tid:" << root1->getId() << endl;
      cdebug_tabw(160,-1);
      return root1;
    }

    if (root1->getRank() < root2->getRank())
      std::swap( root1, root2 );
    if (root1->getRank() == root2->getRank())
      root1->incRank();
    root2->setParent( root1 );
    cdebug_log(160,0) << "New root tid:" << root1->getId()
                      << " child tid:" << root2->getId() << endl;
  // Fuse root2 into root1 here!

    cdebug_tabw(160,-1);
    return root1;
  }


  Equipotential* Tile::newEquipotential ()
  {
    if (_equipotential) {
      cerr << Error( "Tile::newEquipotential(): Equipotential already created (ignoring).\n"
                     "        (on: %s)"
                   , getString(this).c_str()
                   ) << endl;
      return _equipotential;
    }

    _equipotential = Equipotential::create( _occurrence.getOwnerCell() );
    _equipotential->add( _occurrence, _boundingBox );
    cdebug_log(160,0) << "new " << _equipotential << endl;
    cdebug_log(160,0) << "| " << _occurrence << endl;
    return _equipotential;
  }


  string  Tile::_getTypeName () const
  { return "Tramontana::Tile"; }


  string  Tile::_getString () const
  {
    ostringstream  os;
    os << "<Tile tid:" << _id
       << " " << _boundingBox << " " << _layer->getName() << " " << _occurrence << ">";
    return os.str();
  }


  Record* Tile::_getRecord () const
  {
    Record* record = new Record ( _getString() );
    if (record) {
      record->add( getSlot( "_occurrence" , &_occurrence  ) );
      record->add( getSlot( "_layer"      ,  _layer       ) );
      record->add( getSlot( "_boundingBox", &_boundingBox ) );
      record->add( getSlot( "_flags"      , &_flags       ) );
    }
    return record;
  }


} // Tramontana namespace.
