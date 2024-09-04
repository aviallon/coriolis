// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2006-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+ 
// |                   C O R I O L I S                               |
// |    I s o b a r  -  Hurricane / Python Interface                 |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :  "./PyCell.cpp"                                  |
// +-----------------------------------------------------------------+


#include "hurricane/isobar/PyCell.h"
#include "hurricane/isobar/PyBox.h"
#include "hurricane/isobar/PyLibrary.h"
#include "hurricane/isobar/PyInstance.h"
#include "hurricane/isobar/PyOccurrence.h"
#include "hurricane/isobar/ProxyProperty.h"
#include "hurricane/isobar/PyNet.h"
#include "hurricane/isobar/PyNetCollection.h"
#include "hurricane/isobar/PyReferenceCollection.h"
#include "hurricane/isobar/PyInstanceCollection.h"
#include "hurricane/isobar/PyComponentCollection.h"
#include "hurricane/isobar/PyOccurrenceCollection.h"

namespace  Isobar {

  
  bool  pyListToStringSet ( PyObject* list, set<string>& v )
  {
    if (not PyList_Check(list)) return false;

    int length = PyList_Size( list );
    for ( int i=0 ; i<length ; ++i ) {
      PyObject* item = PyList_GetItem( list, i );
      if (not PyString_Check(item)) {
        string message =  "pyListToStringSet: Item at position " + getString(i) + " is not a string.";
        PyErr_SetString( ConstructorError, message.c_str() );
        return false;
      }
      v.insert( PyString_AsString(item) );
    }
    return true;
  }


using namespace Hurricane;

extern "C" {


#undef  ACCESS_OBJECT
#undef  ACCESS_CLASS
#define ACCESS_OBJECT           _baseObject._object
#define ACCESS_CLASS(_pyObject)  &(_pyObject->_baseObject)
#define METHOD_HEAD(function)   GENERIC_METHOD_HEAD(Cell,cell,function)


// x=================================================================x
// |                "PyCell" Python Module Code Part                 |
// x=================================================================x

#if defined(__PYTHON_MODULE__)


  // x-------------------------------------------------------------x
  // |                "PyCell" Attribute Methods                   |
  // x-------------------------------------------------------------x
  

  // Standart Accessors (Attributes).


  // Standart Delete (Attribute).
  DBoDestroyAttribute(PyCell_destroy,PyCell)


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_create ()"

  PyObject* PyCell_create ( PyObject*, PyObject* args )
  {
    cdebug_log(20,0) << "PyCell_create()" << endl;

    char*      name      = NULL;
    PyLibrary* pyLibrary = NULL;
    Cell*      cell      = NULL;

    HTRY
      if (PyArg_ParseTuple(args,"O!s:Cell.create", &PyTypeLibrary, &pyLibrary, &name)) {
        cell = Cell::create( PYLIBRARY_O(pyLibrary), Name(name) );
      } else {
        string message = "Cell::create(): Invalid number of parameters for Cell constructor, name=";
        if (name) message += "\"" + getString(name) + "\".";
        else      message += "(NULL).";

        PyErr_SetString( ConstructorError, message.c_str() );
        return NULL;
      }
    HCATCH

    return PyCell_Link(cell);
  }

  
  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getLibrary ()"

  static PyObject* PyCell_getLibrary ( PyCell *self ) {
    cdebug_log(20,0) << "PyCell_getLibrary ()" << endl;

    Library* library = NULL;

    HTRY
    METHOD_HEAD ( "Cell.getLibrary()" )
    library = cell->getLibrary ();
    HCATCH
    
    return PyLibrary_Link(library);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getName ()"

  GetNameMethod(Cell, cell)

  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getInstance ()"

  static PyObject* PyCell_getInstance ( PyCell *self, PyObject* args ) {
    cdebug_log(20,0) << "PyCell_getInstance ()" << endl;
    METHOD_HEAD("Cell.getInstance()")

    Instance* instance = NULL;
    
    HTRY
    char* name;
    if (PyArg_ParseTuple(args,"s:Cell.getInstance", &name)) {
        instance = cell->getInstance(Name(name));
    } else {
        PyErr_SetString(ConstructorError, "invalid number of parameters for Cell.getInstance." );
        return NULL;
    }
    HCATCH
    
    return PyInstance_Link(instance);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getInstances()"

  static PyObject* PyCell_getInstances( PyCell *self ) {
    cdebug_log(20,0) << "PyCell_getInstances()" << endl;

    METHOD_HEAD ( "Cell.getInstances()" )

    PyInstanceCollection* pyInstanceCollection = NULL;

    HTRY
    Instances* instances = new Instances(cell->getInstances());

    pyInstanceCollection = PyObject_NEW(PyInstanceCollection, &PyTypeInstanceCollection);
    if (pyInstanceCollection == NULL) { 
        return NULL;
    }

    pyInstanceCollection->_object = instances;
    HCATCH
    
    return (PyObject*)pyInstanceCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getInstancesUnder()"

  static PyObject* PyCell_getInstancesUnder(PyCell *self, PyObject* args) {
    cdebug_log(20,0) << "PyCell_getInstancesUnder()" << endl;

    METHOD_HEAD("Cell.getInstancesUnder()")

    PyBox* pyBox;
    if (!PyArg_ParseTuple(args,"O!:Cell.getInstancesUnder", &PyTypeBox, &pyBox)) {
        return NULL;
    }

    PyInstanceCollection* pyInstanceCollection = NULL;

    HTRY
    Instances* instances = new Instances(cell->getInstancesUnder(*PYBOX_O(pyBox)));

    pyInstanceCollection = PyObject_NEW(PyInstanceCollection, &PyTypeInstanceCollection);
    if (pyInstanceCollection == NULL) { 
        return NULL;
    }

    pyInstanceCollection->_object = instances;
    HCATCH
    
    return (PyObject*)pyInstanceCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getSlaveInstances()"

  static PyObject* PyCell_getSlaveInstances(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getSlaveInstances()" << endl;

    METHOD_HEAD("Cell.getSlaveInstances()")

    PyInstanceCollection* pyInstanceCollection = NULL;

    HTRY
    Instances* instances = new Instances(cell->getSlaveInstances());

    pyInstanceCollection = PyObject_NEW(PyInstanceCollection, &PyTypeInstanceCollection);
    if (pyInstanceCollection == NULL) { 
        return NULL;
    }

    pyInstanceCollection->_object = instances;
    HCATCH
    
    return (PyObject*)pyInstanceCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getComponents()"

  static PyObject* PyCell_getComponents(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getComponents()" << endl;

    METHOD_HEAD("Cell.getComponents()")

    PyComponentCollection* pyComponentCollection = NULL;

    HTRY
    Components* components = new Components(cell->getComponents());

    pyComponentCollection = PyObject_NEW(PyComponentCollection, &PyTypeComponentCollection);
    if (pyComponentCollection == NULL) { 
        return NULL;
    }

    pyComponentCollection->_object = components;
    HCATCH
    
    return (PyObject*)pyComponentCollection;
  }



  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getComponentsUnder()"

  static PyObject* PyCell_getComponentsUnder(PyCell *self, PyObject* args) {
    cdebug_log(20,0) << "PyCell_getComponentsUnder()" << endl;

    METHOD_HEAD("Cell.getComponentsUnder()")

    PyBox* pyBox;
    if (!PyArg_ParseTuple(args,"O!:Cell.getComponentsUnder", &PyTypeBox, &pyBox)) {
        return NULL;
    }
    PyComponentCollection* pyComponentCollection = NULL;

    HTRY
    Components* components = new Components(cell->getComponentsUnder(*PYBOX_O(pyBox)));

    pyComponentCollection = PyObject_NEW(PyComponentCollection, &PyTypeComponentCollection);
    if (pyComponentCollection == NULL) { 
        return NULL;
    }

    pyComponentCollection->_object = components;
    HCATCH
    
    return (PyObject*)pyComponentCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getOccurrences()"

  static PyObject* PyCell_getOccurrences(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getOccurrences()" << endl;

    METHOD_HEAD("Cell.getOccurrences()")

    PyOccurrenceCollection* pyOccurrenceCollection = NULL;

    HTRY
    Occurrences* occurrences = new Occurrences(cell->getOccurrences());

    pyOccurrenceCollection = PyObject_NEW(PyOccurrenceCollection, &PyTypeOccurrenceCollection);
    if (pyOccurrenceCollection == NULL) { 
        return NULL;
    }

    pyOccurrenceCollection->_object = occurrences;
    HCATCH
    
    return (PyObject*)pyOccurrenceCollection;
  }



  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getOccurrencesUnder()"

  static PyObject* PyCell_getOccurrencesUnder(PyCell *self, PyObject* args) {
    cdebug_log(20,0) << "PyCell_getOccurrencesUnder()" << endl;

    METHOD_HEAD("Cell.getOccurrencesUnder()")

    PyBox* pyBox;
    if (!PyArg_ParseTuple(args,"O!:Cell.getInstancesUnder", &PyTypeBox, &pyBox)) {
        return NULL;
    }
    PyOccurrenceCollection* pyOccurrenceCollection = NULL;

    HTRY
    Occurrences* occurrences = new Occurrences(cell->getOccurrencesUnder(*PYBOX_O(pyBox)));

    pyOccurrenceCollection = PyObject_NEW(PyOccurrenceCollection, &PyTypeOccurrenceCollection);
    if (pyOccurrenceCollection == NULL) { 
        return NULL;
    }

    pyOccurrenceCollection->_object = occurrences;
    HCATCH
    
    return (PyObject*)pyOccurrenceCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getTerminalNetlistInstanceOccurrences()"

  static PyObject* PyCell_getTerminalNetlistInstanceOccurrences ( PyCell* self)
  {
    cdebug_log(20,0) << "PyCell_getTerminalNetlistInstanceOccurrences()" << endl;

    METHOD_HEAD ( "Cell.getTerminalNetlistInstanceOccurrences()" )

    PyOccurrenceCollection* pyOccurrenceCollection = NULL;
    HTRY
      Occurrences* occurrences = new Occurrences(cell->getTerminalNetlistInstanceOccurrences());
  
      pyOccurrenceCollection = PyObject_NEW(PyOccurrenceCollection, &PyTypeOccurrenceCollection);
      if (pyOccurrenceCollection == NULL) return NULL;
  
      pyOccurrenceCollection->_object = occurrences;
    HCATCH
    return (PyObject*)pyOccurrenceCollection;
  }


  static PyObject* PyCell_getNonTerminalNetlistInstanceOccurrences ( PyCell* self)
  {
    cdebug_log(20,0) << "PyCell_getNonTerminalNetlistInstanceOccurrences()" << endl;

    METHOD_HEAD ( "Cell.getTerminalNetlistNonInstanceOccurrences()" )

    PyOccurrenceCollection* pyOccurrenceCollection = NULL;
    HTRY
      Occurrences* occurrences = new Occurrences(cell->getNonTerminalNetlistInstanceOccurrences());
  
      pyOccurrenceCollection = PyObject_NEW(PyOccurrenceCollection, &PyTypeOccurrenceCollection);
      if (pyOccurrenceCollection == NULL) return NULL;
  
      pyOccurrenceCollection->_object = occurrences;
    HCATCH
    return (PyObject*)pyOccurrenceCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getTerminalNetlistInstanceOccurrencesUnder()"

  static PyObject* PyCell_getTerminalNetlistInstanceOccurrencesUnder(PyCell *self, PyObject* args) {
    cdebug_log(20,0) << "PyCell_getTerminalNetlistInstanceOccurrencesUnder()" << endl;

    METHOD_HEAD ( "Cell.getTerminalNetlistInstanceOccurrencesUnder()" )

    PyBox* pyBox;
    if (!PyArg_ParseTuple(args,"O!:Cell.getInstancesUnder", &PyTypeBox, &pyBox)) {
        return NULL;
    }

    PyOccurrenceCollection* pyOccurrenceCollection = NULL;

    HTRY
    Occurrences* occurrences = new Occurrences(cell->getTerminalNetlistInstanceOccurrencesUnder(*PYBOX_O(pyBox)));

    pyOccurrenceCollection = PyObject_NEW(PyOccurrenceCollection, &PyTypeOccurrenceCollection);
    if (pyOccurrenceCollection == NULL) { 
        return NULL;
    }

    pyOccurrenceCollection->_object = occurrences;
    HCATCH
    
    return (PyObject*)pyOccurrenceCollection;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getReferences()"

  static PyObject* PyCell_getReferences(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getReferences()" << endl;

    METHOD_HEAD("Cell.getReferences()")

    PyReferenceCollection* pyReferenceCollection = NULL;

    HTRY
    References* references = new References(cell->getReferences());

    pyReferenceCollection = PyObject_NEW(PyReferenceCollection, &PyTypeReferenceCollection);
    if (pyReferenceCollection == NULL) { 
        return NULL;
    }

    pyReferenceCollection->_object = references;
    HCATCH
    
    return (PyObject*)pyReferenceCollection;
  }

  
  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getHyperNets()"

  static PyObject* PyCell_getHyperNets(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getHyperNets()" << endl;

    METHOD_HEAD ( "Cell.getHyperNets()" )

    PyOccurrenceCollection* pyOccurrenceCollection = NULL;

    HTRY
    Occurrences* occurrences = new Occurrences(cell->getHyperNetRootNetOccurrences());

    pyOccurrenceCollection = PyObject_NEW(PyOccurrenceCollection, &PyTypeOccurrenceCollection);
    if (pyOccurrenceCollection == NULL) { 
        return NULL;
    }

    pyOccurrenceCollection->_object = occurrences;
    HCATCH
    
    return ( (PyObject*)pyOccurrenceCollection);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getNet ()"

  static PyObject* PyCell_getNet(PyCell *self, PyObject* args) {
      cdebug_log(20,0) << "PyCell_getNet()" << endl;
      METHOD_HEAD ( "Cell.getNet()" )
      Net* net = NULL;
      HTRY
        char* name = NULL;
        if (not PyArg_ParseTuple(args,"s:Cell.getNet()", &name)) {
          PyErr_SetString( ProxyError, "Cell.getNet(): Net name argument is not a string." );
          return NULL;
        } 
        net = cell->getNet( Name(name) );
      HCATCH
      return PyNet_Link(net);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getNets ()"

  static PyObject* PyCell_getNets ( PyCell *self ) {
    cdebug_log(20,0) << "PyCell_getNets()" << endl;

    METHOD_HEAD("Cell.getNets()")

    PyNetCollection* pyNetCollection = NULL;

    HTRY
    Nets* nets = new Nets(cell->getNets());

    pyNetCollection = PyObject_NEW(PyNetCollection, &PyTypeNetCollection);
    if (pyNetCollection == NULL) { 
        return NULL;
    }

    pyNetCollection->_object = nets;
    HCATCH
    
    return ( (PyObject*)pyNetCollection);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getExternalNets()"

  static PyObject* PyCell_getExternalNets(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getExternalNets()" << endl;

    METHOD_HEAD("Cell.getExternalNets()")

    PyNetCollection* pyNetCollection = NULL;

    HTRY
    Nets* nets = new Nets(cell->getExternalNets());

    pyNetCollection = PyObject_NEW(PyNetCollection, &PyTypeNetCollection);
    if (pyNetCollection == NULL) { 
        return NULL;
    }
    pyNetCollection->_object = nets;

    HCATCH

    return ((PyObject*)pyNetCollection);
  }  

  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getClockNets()"

  static PyObject* PyCell_getClockNets(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getClockNets()" << endl;

    METHOD_HEAD("Cell.getClockNets")

    PyNetCollection* pyNetCollection = NULL;

    HTRY
    Nets* nets = new Nets(cell->getClockNets());

    pyNetCollection = PyObject_NEW(PyNetCollection, &PyTypeNetCollection);
    if (pyNetCollection == NULL) { 
        return NULL;
    }
    pyNetCollection->_object = nets;

    HCATCH

    return ((PyObject*)pyNetCollection);
  }

  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getSupplyNets()"

  static PyObject* PyCell_getSupplyNets(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getSupplyNets()" << endl;

    METHOD_HEAD ( "Cell.getSupplyNets()" )

    PyNetCollection* pyNetCollection = NULL;

    HTRY
    Nets* nets = new Nets(cell->getSupplyNets());

    pyNetCollection = PyObject_NEW(PyNetCollection, &PyTypeNetCollection);
    if (pyNetCollection == NULL) { 
        return NULL;
    }
    pyNetCollection->_object = nets;

    HCATCH

    return ((PyObject*)pyNetCollection);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getPowerNets()"

  static PyObject* PyCell_getPowerNets(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getPowerNets()" << endl;

    METHOD_HEAD ( "Cell.getPowerNets()" )

    PyNetCollection* pyNetCollection = NULL;

    HTRY
    Nets* nets = new Nets(cell->getPowerNets());

    pyNetCollection = PyObject_NEW(PyNetCollection, &PyTypeNetCollection);
    if (pyNetCollection == NULL) { 
        return NULL;
    }
    pyNetCollection->_object = nets;

    HCATCH

    return ((PyObject*)pyNetCollection);
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getGroundNets()"

  static PyObject* PyCell_getGroundNets(PyCell *self) {
    cdebug_log(20,0) << "PyCell_getGroundNets()" << endl;

    METHOD_HEAD ( "Cell.getGroundNets()" )

    PyNetCollection* pyNetCollection = NULL;

    HTRY
    Nets* nets = new Nets(cell->getGroundNets());

    pyNetCollection = PyObject_NEW(PyNetCollection, &PyTypeNetCollection);
    if (pyNetCollection == NULL) { 
        return NULL;
    }
    pyNetCollection->_object = nets;

    HCATCH

    return ((PyObject*)pyNetCollection);
  }

  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getAbutmentBox ()"

  static PyObject* PyCell_getAbutmentBox ( PyCell *self ) {
    cdebug_log(20,0) << "PyCell_getAbutmentBox()" << endl;
    
    METHOD_HEAD ( "Cell.getAbutmentBox()" )

    PyBox* abutmentBox = PyObject_NEW ( PyBox, &PyTypeBox );
    if (abutmentBox == NULL) { return NULL; }

    HTRY
    abutmentBox->_object = new Box ( cell->getAbutmentBox() );
    HCATCH

    return ( (PyObject*)abutmentBox );
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_setName ()"

  SetNameMethod(Cell, cell)

  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_setAbutmentBox ()"

  static PyObject* PyCell_setAbutmentBox ( PyCell *self, PyObject* args ) {
    cdebug_log(20,0) << "Cell.setAbutmentBox()" << endl;

    HTRY
    METHOD_HEAD ( "Cell.setAbutmentBox()" )

    PyBox* pyBox;
    if (!PyArg_ParseTuple(args,"O!:Cell.setAbutmentBox", &PyTypeBox, &pyBox)) {
        return NULL;
    }
    cell->setAbutmentBox ( *PYBOX_O(pyBox) );
    HCATCH

    Py_RETURN_NONE;
  }
  
  
  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_setTerminalNetlist ()"

  static PyObject* PyCell_setTerminalNetlist ( PyCell *self, PyObject* args ) {
    cdebug_log(20,0) << "PyCell_setTerminalNetlist ()" << endl;

    HTRY
    METHOD_HEAD ( "Cell.setTerminalNetlist()" )
    PyObject* arg0;
    if (!PyArg_ParseTuple(args,"O:Cell.setTerminalNetlist", &arg0) && PyBool_Check(arg0)) {
      return NULL;
    }
    PyObject_IsTrue(arg0)?cell->setTerminalNetlist(true):cell->setTerminalNetlist(false);
    HCATCH
    Py_RETURN_NONE;
  }
  
  
  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_setAbstractedSupply ()"

  static PyObject* PyCell_setAbstractedSupply ( PyCell *self, PyObject* args ) {
    cdebug_log(20,0) << "PyCell_setAbstractedSupply ()" << endl;
    HTRY
      METHOD_HEAD( "Cell.setAbstractedSupply()" )
      PyObject* arg0 = NULL;
      if (not PyArg_ParseTuple(args,"O:Cell.setAbstractedSupply", &arg0)
         and PyBool_Check(arg0)) {
        return NULL;
      }
      PyObject_IsTrue(arg0) ? cell->setAbstractedSupply(true) : cell->setAbstractedSupply(false);
    HCATCH
    Py_RETURN_NONE;
  }
  
  
  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_uniquify ()"

  static PyObject* PyCell_uniquify ( PyCell *self, PyObject* args ) {
    cdebug_log(20,0) << "PyCell_uniquify ()" << endl;

    HTRY
      METHOD_HEAD ( "Cell.uniquify()" )
      unsigned int  depth;
      if (not PyArg_ParseTuple(args,"I:Cell.uniquify", &depth)) {
        PyErr_SetString(ConstructorError, "Cell.uniquify(): Invalid number/bad type of parameter.");
        return NULL;
      }
      cell->uniquify( depth );
    HCATCH
    Py_RETURN_NONE;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_getClone ()"

  static PyObject* PyCell_getClone ( PyCell *self ) {
    cdebug_log(20,0) << "PyCell_getClone ()" << endl;

    Cell* cloneCell = NULL;
    HTRY
      METHOD_HEAD( "Cell.getClone()" )
      cloneCell = cell->getClone();
    HCATCH

    return PyCell_Link( cloneCell );
  }


  static PyObject* PyCell_flattenNets ( PyCell* self, PyObject *args )
  {
    cdebug_log(20,0) << "PyCell_flattenNets()" << endl;
    
    uint64_t     flags    = 0;
    set<string>  excludeds;
    Instance*    instance = NULL;
    PyObject*    arg0     = NULL;
    PyObject*    arg1     = NULL;
    PyObject*    arg2     = NULL;

    HTRY
      METHOD_HEAD ( "Cell.flattenNets()" )
      __cs.init( "Cell.flattenNets" );
      if (not PyArg_ParseTuple(args,"OO|O:Cell.flattenNets", &arg0, &arg1, &arg2 )) {
        PyErr_SetString( ConstructorError, "Cell.flattenNets(): Takes between two and three parameters." );
        return NULL;
      }
  
      if (arg0 != Py_None) {
        if (not IsPyInstance(arg0)) {
          PyErr_SetString( ConstructorError, "Cell.flattenNets(): First argument must be None or an Instance." );
          return NULL;
        }
        instance = PYINSTANCE_O( arg0 );
      }
      if (arg2) {
        pyListToStringSet( arg1, excludeds );
        flags = PyInt_AsLong( arg2 );
      } else {
        flags = PyInt_AsLong( arg1 );
      }

      cell->flattenNets( instance, excludeds, flags );
    HCATCH
    Py_RETURN_NONE;
  }


  // ---------------------------------------------------------------
  // Attribute Method  :  "PyCell_destroyPhysical ()"

  static PyObject* PyCell_destroyPhysical ( PyCell *self )
  {
    cdebug_log(20,0) << "PyCell_destroyPhysical ()" << endl;
    HTRY
      METHOD_HEAD( "Cell.destroyPhysical()" )
      cell->destroyPhysical();
    HCATCH
    Py_RETURN_NONE;
  }


  // Standart Predicates (Attributes).
  DirectGetBoolAttribute(PyCell_isTerminal         , isTerminal         ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isTerminalNetlist  , isTerminalNetlist  ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isUnique           , isUnique           ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isUniquified       , isUniquified       ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isUniquifyMaster   , isUniquifyMaster   ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isExtractConsistent, isExtractConsistent,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isRouted           , isRouted           ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isPad              , isPad              ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isFeed             , isFeed             ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isDiode            , isDiode            ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_isPowerFeed        , isPowerFeed        ,PyCell,Cell)
  DirectGetBoolAttribute(PyCell_updatePlacedFlag   , updatePlacedFlag   ,PyCell,Cell)
//DirectGetLongAttribute(PyCell_getFlags           , getFlags           ,PyCell,Cell)
  DirectSetBoolAttribute(PyCell_setRouted          , setRouted          ,PyCell,Cell)
  DirectSetBoolAttribute(PyCell_setPad             , setPad             ,PyCell,Cell)
  DirectSetBoolAttribute(PyCell_setFeed            , setFeed            ,PyCell,Cell)
  DirectSetBoolAttribute(PyCell_setDiode           , setDiode           ,PyCell,Cell)
  DirectSetBoolAttribute(PyCell_setPowerFeed       , setPowerFeed       ,PyCell,Cell)
  DirectSetLongAttribute(PyCell_setFlags           , setFlags           ,PyCell,Cell)
  DirectSetLongAttribute(PyCell_resetFlags         , resetFlags         ,PyCell,Cell)
                                                   
  GetBoundStateAttribute(PyCell_isPyBound          ,PyCell,Cell)

  // ---------------------------------------------------------------
  // PyCell Attribute Method table.

  PyMethodDef PyCell_Methods[] =
    { { "create"              , (PyCFunction)PyCell_create               , METH_VARARGS|METH_STATIC
                              , "Create a new cell." }
  //, { "getFlags"            , (PyCFunction)PyCell_getFlags             , METH_NOARGS , "Returns state flags." }
    , { "getLibrary"          , (PyCFunction)PyCell_getLibrary           , METH_NOARGS , "Returns the library owning the cell." }
    , { "getName"             , (PyCFunction)PyCell_getName              , METH_NOARGS , "Returns the name of the cell." }
    , { "getInstance"         , (PyCFunction)PyCell_getInstance          , METH_VARARGS, "Returns the instance of name <name> if it exists, else NULL." }
    , { "getInstances"        , (PyCFunction)PyCell_getInstances         , METH_NOARGS , "Returns the locator of the collection of all instances called by the cell." } // getInstances
    , { "getInstancesUnder"   , (PyCFunction)PyCell_getInstancesUnder    , METH_VARARGS, "Returns the locator of the collection of all instances of the cell intersecting the given rectangular area." } // getInstancesUnder
    , { "getSlaveInstances"   , (PyCFunction)PyCell_getSlaveInstances    , METH_NOARGS , "Returns the locator of the collection of instances whose master is this cell." } // getSlaveInstances
    , { "getComponents"       , (PyCFunction)PyCell_getComponents        , METH_NOARGS , "Returns the collection of all components belonging to the cell." }
    , { "getComponentsUnder"  , (PyCFunction)PyCell_getComponentsUnder   , METH_VARARGS, "Returns the collection of all components belonging to this cell and intersecting the given rectangular area." }
    , { "getOccurrences"      , (PyCFunction)PyCell_getOccurrences       , METH_NOARGS , "Returns the collection of all occurrences belonging to the cell." }
    , { "getOccurrencesUnder" , (PyCFunction)PyCell_getOccurrencesUnder  , METH_VARARGS, "Returns the collection of all occurrences belonging to this cell and intersecting the given rectangular area." }
    , { "getTerminalNetlistInstanceOccurrences"     , (PyCFunction)PyCell_getTerminalNetlistInstanceOccurrences     , METH_NOARGS
                                                    , "Returns the collection all terminal instances occurrences." }
    , { "getNonTerminalNetlistInstanceOccurrences"  , (PyCFunction)PyCell_getNonTerminalNetlistInstanceOccurrences  , METH_NOARGS
                                                    , "Returns the collection of all non-terminal instances occurrences." }
    , { "getTerminalNetlistInstanceOccurrencesUnder", (PyCFunction)PyCell_getTerminalNetlistInstanceOccurrencesUnder, METH_VARARGS
                                                    , "Returns the collection of all occurrences belonging to this cell and intersecting the given rectangular area." }
    , { "getReferences"       , (PyCFunction)PyCell_getReferences       , METH_VARARGS, "Returns the collection of all references belonging to the cell." }
    , { "getHyperNets"        , (PyCFunction)PyCell_getHyperNets        , METH_VARARGS, "Returns the collection of all hyperNets belonging to the cell." }
    , { "getNet"              , (PyCFunction)PyCell_getNet              , METH_VARARGS, "Returns the net of name <name> if it exists, else NULL." }
    , { "getNets"             , (PyCFunction)PyCell_getNets             , METH_NOARGS , "Returns the collection of all nets of the cell." }
    , { "getExternalNets"     , (PyCFunction)PyCell_getExternalNets     , METH_NOARGS , "Returns the collection of all external nets of the cell." }
    , { "getClockNets"        , (PyCFunction)PyCell_getClockNets        , METH_NOARGS , "Returns the collection of all clock nets of the cell." }
    , { "getSupplyNets"       , (PyCFunction)PyCell_getSupplyNets       , METH_NOARGS , "Returns the collection of all supply nets of the cell." }
    , { "getPowerNets"        , (PyCFunction)PyCell_getPowerNets        , METH_NOARGS , "Returns the collection of all power nets of the cell." }
    , { "getGroundNets"       , (PyCFunction)PyCell_getGroundNets       , METH_NOARGS , "Returns the collection of all ground nets of the cell." }
    , { "getAbutmentBox"      , (PyCFunction)PyCell_getAbutmentBox      , METH_NOARGS , "Returns the abutment box of the cell(which is defined by the designer unlike the bounding box which is managed dynamically)" }
    , { "isTerminal"          , (PyCFunction)PyCell_isTerminal          , METH_NOARGS , "Returns true if the cell is marked as terminal, else false." }
    , { "isTerminalNetlist"   , (PyCFunction)PyCell_isTerminalNetlist   , METH_NOARGS , "Returns true if the cell is a leaf of the hierarchy, else false." }
    , { "isUnique"            , (PyCFunction)PyCell_isUnique            , METH_NOARGS , "Returns true if the cell has one or less instance." }
    , { "isUniquified"        , (PyCFunction)PyCell_isUniquified        , METH_NOARGS , "Returns true if the cell is the result of an uniquification." }
    , { "isUniquifyMaster"    , (PyCFunction)PyCell_isUniquifyMaster    , METH_NOARGS , "Returns true if the cell is the reference for an uniquification." }
    , { "isRouted"            , (PyCFunction)PyCell_isRouted            , METH_NOARGS , "Returns true if the cell is flagged as routed." }
    , { "isPad"               , (PyCFunction)PyCell_isPad               , METH_NOARGS , "Returns true if the cell is flagged as I/O pad." }
    , { "isFeed"              , (PyCFunction)PyCell_isFeed              , METH_NOARGS , "Returns true if the cell is flagged as feed (filler cell)." }
    , { "isDiode"             , (PyCFunction)PyCell_isDiode             , METH_NOARGS , "Returns true if the cell is flagged as diode." }
    , { "isPowerFeed"         , (PyCFunction)PyCell_isPowerFeed         , METH_NOARGS , "Returns true if the cell is flagged as power rail element." }
    , { "isBound"             , (PyCFunction)PyCell_isPyBound           , METH_NOARGS , "Returns true if the cell is bounded to the hurricane cell" }    
    , { "setFlags"            , (PyCFunction)PyCell_setFlags            , METH_VARARGS, "Set state flags." }
    , { "resetFlags"          , (PyCFunction)PyCell_resetFlags          , METH_VARARGS, "Reset state flags." }
    , { "updatePlacedFlag"    , (PyCFunction)PyCell_updatePlacedFlag    , METH_VARARGS, "Recompute the state of the *fully placed* flag." }
    , { "setName"             , (PyCFunction)PyCell_setName             , METH_VARARGS, "Allows to change the cell name." }
    , { "setAbutmentBox"      , (PyCFunction)PyCell_setAbutmentBox      , METH_VARARGS, "Sets the cell abutment box." }
    , { "setTerminalNetlist"  , (PyCFunction)PyCell_setTerminalNetlist  , METH_VARARGS, "Sets the cell terminal netlist status." }
    , { "setAbstractedSupply" , (PyCFunction)PyCell_setAbstractedSupply , METH_VARARGS, "Sets the cell abstracted supply status." }
    , { "setRouted"           , (PyCFunction)PyCell_setRouted           , METH_VARARGS, "Sets the cell routed status." }
    , { "setPad"              , (PyCFunction)PyCell_setPad              , METH_VARARGS, "Sets/reset the cell I/O pad flag." }
    , { "setFeed"             , (PyCFunction)PyCell_setFeed             , METH_VARARGS, "Sets/reset the cell feed (filler cell) flag." }
    , { "setDiode"            , (PyCFunction)PyCell_setDiode            , METH_VARARGS, "Sets/reset the cell diode flag." }
    , { "setPowerFeed"        , (PyCFunction)PyCell_setPowerFeed        , METH_VARARGS, "Sets/reset the cell power rail element flag." }
    , { "uniquify"            , (PyCFunction)PyCell_uniquify            , METH_VARARGS, "Uniquify the Cell and it's instances up to <depth>." }
    , { "getClone"            , (PyCFunction)PyCell_getClone            , METH_NOARGS , "Return a copy of the Cell (placement only)." }
    , { "flattenNets"         , (PyCFunction)PyCell_flattenNets         , METH_VARARGS, "Perform a virtual flatten, possibly limited to one instance." }
    , { "destroyPhysical"     , (PyCFunction)PyCell_destroyPhysical     , METH_NOARGS , "Destroy all physical components, including DeepNets (vflatten)." }
    , { "destroy"             , (PyCFunction)PyCell_destroy             , METH_NOARGS , "Destroy associated hurricane object The python object remains." }
    , {NULL, NULL, 0, NULL}   /* sentinel */
    };


  // x-------------------------------------------------------------x
  // |                  "PyCell" Object Methods                    |
  // x-------------------------------------------------------------x

  DBoDeleteMethod(Cell)
  PyTypeObjectLinkPyType(Cell)


#else  // End of Python Module Code Part.


// x=================================================================x
// |               "PyCell" Shared Library Code Part                 |
// x=================================================================x

  // Link/Creation Method.
  DBoLinkCreateMethod(Cell)
  PyTypeInheritedObjectDefinitions(Cell, Entity)


  extern  void  PyCell_postModuleInit ()
  {
    PyObject* constant;

    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::NoFlags            ,"Flags_NoFlags");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::BuildRings         ,"Flags_BuildRings");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::BuildClockRings    ,"Flags_BuildClockRings");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::BuildSupplyRings   ,"Flags_BuildSupplyRings");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::NoClockFlatten     ,"Flags_NoClockFlatten");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::TerminalNetlist    ,"Flags_TerminalNetlist");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::Pad                ,"Flags_Pad");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::Feed               ,"Flags_Feed");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::Diode              ,"Flags_Diode");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::PowerFeed          ,"Flags_PowerFeed");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::FlattenedNets      ,"Flags_FlattenedNets");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::AbstractedSupply   ,"Flags_AbstractedSupply");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::Placed             ,"Flags_Placed");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::Routed             ,"Flags_Routed");
    LoadObjectConstant(PyTypeCell.tp_dict,Cell::Flags::NoExtractConsistent,"Flags_NoExtractConsistent");
  }


#endif  // End of Shared Library Code Part.


}  // End of extern "C".




}  // End of Isobar namespace.
