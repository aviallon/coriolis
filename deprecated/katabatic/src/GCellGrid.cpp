// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2008-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |        K a t a b a t i c  -  Routing Toolbox                    |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./GCellGrid.cpp"                          |
// +-----------------------------------------------------------------+


#include  "hurricane/Error.h"
#include  "hurricane/Cell.h"
#include  "knik/KnikEngine.h"
#include  "katabatic/Session.h"
#include  "katabatic/GCellGrid.h"
#include  "katabatic/KatabaticEngine.h"


namespace Katabatic {


  using namespace std;
  using Hurricane::tab;
  using Hurricane::Error;
  using Hurricane::ForEachIterator;
  using Knik::KnikEngine;


  const char* missingKnikEngine =
    "%s :\n\n"
    "    Hey, Banana! You forgot to run the Knik global router on %s.\n";


// -------------------------------------------------------------------
// Class  :  "Katabatic::GCellGrid".


  GCellGrid::GCellGrid ( KatabaticEngine* ktbt )
    : Grid<GCell>   (ktbt->getCell()->getAbutmentBox())
    , _katabatic    (ktbt)
    , _densityMode  (MaxDensity)
    , _hEdgeCapacity(ktbt->getConfiguration()->getHEdgeCapacity())
    , _vEdgeCapacity(ktbt->getConfiguration()->getVEdgeCapacity())
  { }


  void  GCellGrid::_postCreate ()
  {
    KnikEngine* knik;
    knik = KnikEngine::get ( getCell() );
    if ( !knik )
      throw Error ( missingKnikEngine, "GCellGrid::_postCreate()", getString(getCell()).c_str() );

    vector<DbU::Unit>  knikGraduations;

    knik->getHorizontalCutLines ( knikGraduations );
    for ( size_t i=0 ; i<knikGraduations.size() ; i++ )
      _yGraduations.addGraduation ( knikGraduations[i] );
    _yGraduations.sort ();
    _rows = _yGraduations.getSize() - 1;

    knik->getVerticalCutLines ( knikGraduations );
    for ( size_t i=0 ; i<knikGraduations.size() ; i++ )
      _xGraduations.addGraduation ( knikGraduations[i] );
    _xGraduations.sort ();
    _columns = _xGraduations.getSize() - 1;

    _rawSize = _columns * _rows;

    cdebug_log(145,1) << "Katabatic GCell Matrix [" << getColumns() << "x" << getRows() << "]" << endl;
    cdebug_log(145,0)   << "_xGraduations := " << _xGraduations._print() << endl;
    cdebug_log(145,0)   << "_yGraduations := " << _yGraduations._print() << endl;

    for ( unsigned int row=0 ; row < getRows() ; row++ ) {
      for ( unsigned int column=0; column<getColumns() ; column++ ) {
        Box boundingBox = Box ( _xGraduations[column  ]
                              , _yGraduations[row     ]
                              , _xGraduations[column+1]
                              , _yGraduations[row   +1]
                              );

        _gcells.push_back ( GCell::create ( this, _gcells.size(), boundingBox ) );
  
        cdebug_log(145,0) << "GCell [" << column << "," << row << "] "
                   << _gcells[_gcells.size()-1] << endl;
      }
    }

    cmess1 << "  o  Reading GCell grid from Knik ["
           << getColumns() << "x" << getRows() << "] "
           << "(" << (getColumns()*getRows()) << ")." << endl;

    cdebug_tabw(145,-1);
  }


  GCellGrid* GCellGrid::create ( KatabaticEngine* ktbt )
  {
    GCellGrid* grid = new GCellGrid ( ktbt );
    grid->_postCreate ();
    return grid;
  }


  GCellGrid::~GCellGrid ()
  { }


  void  GCellGrid::_preDestroy ()
  {
    cdebug_log(145,1) << "GCellGrid::_preDestroy()" << endl;

    vector<GCell*>::iterator   it = _gcells.begin();
    vector<GCell*>::iterator  end = _gcells.end  ();
    for ( ; it != end ; it++ ) (*it)->destroy ();

    cdebug_tabw(145,-1);
  }


  Cell* GCellGrid::getCell () const
  {
    return _katabatic->getCell();
  }


  Interval  GCellGrid::getUSide ( unsigned int direction ) const
  {
    Interval side;
    switch ( direction ) {
      default:
        cerr << Error( "GCellGrid::getUSide(): Unknown direction flag \"%i\""
                     , direction ) << endl;
      case KbHorizontal: side = Interval(_boundingBox.getXMin(),_boundingBox.getXMax()); break;
      case KbVertical:   side = Interval(_boundingBox.getYMin(),_boundingBox.getYMax()); break;
    }
    return side;
  }


  void  GCellGrid::updateContacts ( unsigned int flags )
  {
    if (flags & KbOpenSession) Session::open( _katabatic );

    forEach ( GCell*, gcell, getGCells() )
      gcell->updateContacts();

    if (flags & KbOpenSession) Session::close();
  }


  size_t  GCellGrid::updateDensity ()
  {
    size_t  saturateds = 0;
    forEach ( GCell*, gcell, getGCells() ) {
      saturateds += gcell->updateDensity ();
    }
    return saturateds;
  }


  size_t  GCellGrid::checkDensity () const
  {
    size_t  saturateds = 0;
    forEach ( GCell*, gcell, const_cast<GCellGrid*>(this)->getGCells() ) {
      saturateds += gcell->checkDensity ();
    }
    return saturateds;
  }


  bool  GCellGrid::checkEdgeOverflow ( size_t hreserved, size_t vreserved) const
  {
    bool overload = false;
    forEach ( GCell*, gcell, const_cast<GCellGrid*>(this)->getGCells() ) {
      overload = gcell->checkEdgeSaturation(hreserved,vreserved) or overload;
    }
    return overload;
  }


  string  GCellGrid::_getTypeName () const
  {
    return _TName("GCellGrid");
  }


  string  GCellGrid::_getString () const
  {
    return "<" + _getTypeName() + " "
               + getString(getRows()) + "x" + getString(getColumns()) + ">";
  }


  void  GCellGrid::_xmlWrite ( ostream& o )
  {
    updateDensity ();

    o << "<GCellGrid Cell=\"" << getCell()->getName() << "\">" << endl; 
    o << "<!--            GRID           Lower Left Corner            Density    -->" << endl;
    o << "<!--        column row             XMin YMin             hor  ver  con -->" << endl;

    unsigned int  row = 0;
    forEach ( GCell*, gcell, getGCells() ) {
      if ( gcell->getRow() > row ) {
        o << endl;
        o << "<!--        column row             XMin YMin             hor  ver  con -->" << endl;
        row = gcell->getRow();
      }

      o << "  "; gcell->_xmlWrite ( o ); o << endl;
    }

    o << "</GCellGrid>" << endl;
  }


  Record* GCellGrid::_getRecord () const
  {
    Record* record = Grid<GCell>::_getRecord ();
    record->add ( getSlot ( "_katabatic", _katabatic ) );
    return record;
  }



} // End of Katabatic namespace.
