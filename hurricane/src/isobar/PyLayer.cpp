// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2006-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+ 
// |                   C O R I O L I S                               |
// |    I s o b a r  -  Hurricane / Python Interface                 |
// |                                                                 |
// |  Author      :                    Jean-Paul Chaput              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :  "./PyLayer.cpp"                                 |
// +-----------------------------------------------------------------+


#include "hurricane/isobar/PyTechnology.h"
#include "hurricane/isobar/PyLayer.h"
#include "hurricane/isobar/PyLayerMask.h"
#include "hurricane/isobar/PyBasicLayer.h"
#include "hurricane/isobar/PyRegularLayer.h"
#include "hurricane/isobar/PyContactLayer.h"
#include "hurricane/isobar/PyViaLayer.h"
#include "hurricane/isobar/PyDiffusionLayer.h"
#include "hurricane/isobar/PyTransistorLayer.h"
#include "hurricane/isobar/PyBasicLayerCollection.h"


namespace  Isobar {

using namespace Hurricane;

extern "C" {


#define  METHOD_HEAD(function)   GENERIC_METHOD_HEAD(Layer,layer,function)


// +=================================================================+
// |               "PyLayer" Python Module Code Part                 |
// +=================================================================+

#if defined(__PYTHON_MODULE__)


  // +-------------------------------------------------------------+
  // |                "PyLayer" Attribute Methods                  |
  // +-------------------------------------------------------------+


# define  accessorDbuFromOptBasicLayer(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )    \
  {                                                                                     \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                      \
                                                                                        \
    DbU::Unit  rvalue = 0;                                                              \
                                                                                        \
    HTRY                                                                                \
    GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                  \
                                                                                        \
    PyObject* arg0 = NULL;                                                              \
                                                                                        \
    __cs.init (#SELF_TYPE"."#FUNC_NAME"()");                                            \
    if (PyArg_ParseTuple( args, "|O&:"#SELF_TYPE"."#FUNC_NAME"()", Converter, &arg0)) { \
      if ( __cs.getObjectIds() == ":basiclayer" )                                       \
        rvalue = cobject->FUNC_NAME( PYBASICLAYER_O(arg0) );                            \
      else if  ( __cs.getObjectIds() == "" )                                            \
        rvalue = cobject->FUNC_NAME();                                                  \
      else {                                                                            \
        PyErr_SetString ( ConstructorError                                              \
                        , "invalid parameter type for "#SELF_TYPE"."#FUNC_NAME"()." );  \
        return NULL;                                                                    \
      }                                                                                 \
    } else {                                                                            \
      PyErr_SetString ( ConstructorError                                                \
                      , "Invalid number of parameters passed to "#SELF_TYPE"."#FUNC_NAME"()." ); \
      return NULL;                                                                      \
    }                                                                                   \
    HCATCH                                                                              \
                                                                                        \
    return PyLong_FromLong(rvalue);                                                     \
  }


# define  accessorLayerFromLayer(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                  \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )     \
  {                                                                                      \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                         \
                                                                                         \
    Layer* rlayer = 0;                                                                   \
                                                                                         \
    HTRY                                                                                 \
    GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                   \
                                                                                         \
    PyObject* arg0 = NULL;                                                               \
                                                                                         \
    if (PyArg_ParseTuple( args, "O:"#SELF_TYPE"."#FUNC_NAME"()", &arg0)) {               \
      Layer* argLayer = PYLAYER_O(arg0);                                                 \
      if (argLayer == NULL) {                                                            \
        PyErr_SetString ( ConstructorError                                               \
                        , #SELF_TYPE"."#FUNC_NAME"(): Parameter is not of Layer type" ); \
        return NULL;                                                                     \
      }                                                                                  \
      rlayer = const_cast<Layer*>(cobject->FUNC_NAME( argLayer ));                       \
    } else {                                                                             \
      PyErr_SetString ( ConstructorError                                                 \
                      , #SELF_TYPE"."#FUNC_NAME"(): Parameter is not of Layer type" );   \
      return NULL;                                                                       \
    }                                                                                    \
    HCATCH                                                                               \
                                                                                         \
    return PyLayer_LinkDerived(rlayer);                                                  \
  }


# define  accessorMaskFromVoid(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)  \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self )     \
  {                                                                      \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;       \
                                                                         \
    Layer::Mask  mask = Layer::Mask( 0 );                                \
                                                                         \
    HTRY                                                                 \
    GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")   \
    mask = cobject->FUNC_NAME();                                         \
    HCATCH                                                               \
                                                                         \
    return PyLayerMask_Link(mask);                                       \
  }


# define  accessorLayerFromOptBool(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                    \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )    \
  {                                                                                     \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                      \
    Layer* rlayer = NULL;                                                               \
                                                                                        \
    HTRY                                                                                \
      GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                \
      PyObject* arg0        = NULL;                                                     \
      bool      useSymbolic = true;                                                     \
                                                                                        \
      if (PyArg_ParseTuple( args, "|O:"#SELF_TYPE"."#FUNC_NAME"()", &arg0)) {           \
        if (arg0) useSymbolic = PyObject_IsTrue(arg0);                                  \
      } else {                                                                          \
        PyErr_SetString( ConstructorError                                               \
                       , #SELF_TYPE"."#FUNC_NAME"(): Takes exactly one boolean parameter." );  \
        return NULL;                                                                    \
      }                                                                                 \
      rlayer = const_cast<SELF_TYPE*>( cobject->FUNC_NAME(useSymbolic) );               \
    HCATCH                                                                              \
                                                                                        \
    if (not rlayer) Py_RETURN_NONE;                                                     \
    return PyLayer_LinkDerived(rlayer);                                                 \
  }


# define  accessorLayerFromUInt(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                       \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )    \
  {                                                                                     \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                      \
    Layer* rlayer = NULL;                                                               \
                                                                                        \
    HTRY                                                                                \
      GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                \
      unsigned int  flags = 0;                                                          \
                                                                                        \
      if (not PyArg_ParseTuple( args, "I:"#SELF_TYPE"."#FUNC_NAME"()", &flags)) {       \
        PyErr_SetString( ConstructorError                                               \
                       , #SELF_TYPE"."#FUNC_NAME"(): Takes exactly one integer parameter." );  \
        return NULL;                                                                    \
      }                                                                                 \
      rlayer = const_cast<SELF_TYPE*>( cobject->FUNC_NAME(uint32_t(flags)) );           \
    HCATCH                                                                              \
                                                                                        \
    if (not rlayer) Py_RETURN_NONE;                                                     \
    return PyLayer_LinkDerived(rlayer);                                                 \
  }


# define  accessorDbuFromUInt(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                         \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )    \
  {                                                                                     \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                      \
    DbU::Unit u = 0;                                                                    \
                                                                                        \
    HTRY                                                                                \
      GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                \
      unsigned int  flags = 0;                                                          \
                                                                                        \
      if (not PyArg_ParseTuple( args, "I:"#SELF_TYPE"."#FUNC_NAME"()", &flags)) {       \
        PyErr_SetString( ConstructorError                                               \
                       , #SELF_TYPE"."#FUNC_NAME"(): Takes exactly one integer parameter." );  \
        return NULL;                                                                    \
      }                                                                                 \
      u = cobject->FUNC_NAME( uint32_t(flags) );                                        \
    HCATCH                                                                              \
                                                                                        \
    return PyDbU_FromLong(u);                                                           \
  }


# define  predicateFromVoid(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)          \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self )   \
  {                                                                    \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;     \
                                                                       \
    HTRY                                                               \
    GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()") \
    if (cobject->FUNC_NAME()) Py_RETURN_TRUE;                          \
    HCATCH                                                             \
                                                                       \
    Py_RETURN_FALSE;                                                   \
  }


# define  updatorFromDbu(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                               \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )     \
  {                                                                                      \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                       \
                                                                                         \
    HTRY                                                                                 \
    GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                   \
                                                                                         \
    PyObject* pyDimension = NULL;                                                        \
                                                                                         \
    if (PyArg_ParseTuple( args, "O:"#SELF_TYPE"."#FUNC_NAME"()", &pyDimension)) {        \
      cobject->FUNC_NAME( PyAny_AsLong(pyDimension) );                                   \
    } else {                                                                             \
      PyErr_SetString ( ConstructorError                                                 \
                      , #SELF_TYPE"."#FUNC_NAME"(): Parameter is not of long (DbU) type" ); \
      return NULL;                                                                       \
    }                                                                                    \
    HCATCH                                                                               \
                                                                                         \
    Py_RETURN_NONE;                                                                      \
  }


# define  updatorFromBasicLayerDbu(FUNC_NAME,PY_SELF_TYPE,SELF_TYPE)                     \
  static PyObject* PY_SELF_TYPE##_##FUNC_NAME ( PY_SELF_TYPE* self, PyObject* args )     \
  {                                                                                      \
    cdebug_log(20,0) << #PY_SELF_TYPE "_" #FUNC_NAME "()" << endl;                       \
                                                                                         \
    HTRY                                                                                 \
    GENERIC_METHOD_HEAD(SELF_TYPE,cobject,#SELF_TYPE"."#FUNC_NAME"()")                   \
                                                                                         \
    PyObject* pyBasicLayer = NULL;                                                       \
    PyObject* pyDimension  = NULL;                                                       \
                                                                                         \
    if (PyArg_ParseTuple( args, "OO:"#SELF_TYPE"."#FUNC_NAME"()", &pyBasicLayer, &pyDimension)) { \
      BasicLayer* layer = PYBASICLAYER_O(pyBasicLayer);                                  \
      if (layer == NULL) {                                                               \
        PyErr_SetString ( ConstructorError                                               \
                        , #SELF_TYPE"."#FUNC_NAME"(): First parameter is not of BasicLayer type" ); \
        return NULL;                                                                     \
      }                                                                                  \
      cobject->FUNC_NAME( layer, PyAny_AsLong(pyDimension) );                            \
    } else {                                                                             \
      PyErr_SetString ( ConstructorError                                                 \
                      , #SELF_TYPE"."#FUNC_NAME"(): Bad parameters types or numbers." ); \
      return NULL;                                                                       \
    }                                                                                    \
    HCATCH                                                                               \
                                                                                         \
    Py_RETURN_NONE;                                                                      \
  }


  static PyObject* PyLayer_getTechnology ( PyLayer *self ) {
    cdebug_log(20,0) << "PyLayer_getTechnology ()" << endl;

    Technology* techno = NULL;

    HTRY
    METHOD_HEAD ( "Layer.getTechnology()" )
    techno = layer->getTechnology ();
    HCATCH
    
    return PyTechnology_Link(techno);
  }


  static PyObject* PyLayer_getEnclosure ( PyLayer* self, PyObject* args )
  {
    cdebug_log(20,0) << "PyLayer_getEnclosure()" << endl;

    DbU::Unit  rvalue = 0;

    HTRY
      METHOD_HEAD("Layer.getEnclosure()")
  
      PyObject* arg0 = NULL;
      PyObject* arg1 = NULL;
  
      __cs.init( "Layer.getEnclosure()" );
      if (PyArg_ParseTuple( args, "O&|O&:Layer.getEnclosure()"
                          , Converter, &arg0
                          , Converter, &arg1 )) {
        if ( (__cs.getObjectIds() == ":layer:int") and (PYBASICLAYER_O(arg0)) )
          rvalue = layer->getEnclosure( PYBASICLAYER_O(arg0), PyAny_AsLong(arg1) );
        else if  ( __cs.getObjectIds() == ":int" )
          rvalue = layer->getEnclosure( PyAny_AsLong(arg0) );
        else {
          string message = "invalid parameter type for Layer.getEnclosure() (\"" 
                         + __cs.getObjectIds() + "\")";
          PyErr_SetString ( ConstructorError, message.c_str()  );
          return NULL;
        }
      } else {
        PyErr_SetString ( ConstructorError
                        , "Invalid number of parameters passed to Layer.getEnclosure()." );
        return NULL;
      }
    HCATCH

    return PyLong_FromLong(rvalue);
  }


  static PyObject* PyLayer_setEnclosure ( PyLayer* self, PyObject* args )
  {
    cdebug_log(20,0) << "PyLayer_setEnclosure()" << endl;

    HTRY
    METHOD_HEAD("PyLayer.setEnclosure()")

    PyObject* pyBasicLayer = NULL;
    PyObject* pyDimension  = NULL;
    uint32_t  flags        = 0;

    if (PyArg_ParseTuple( args, "OOi:Layer.setEnclosure()"
                        , &pyBasicLayer, &pyDimension, &flags )) {
      BasicLayer* basicLayer = PYBASICLAYER_O(pyBasicLayer);
      if (basicLayer == NULL) {
        PyErr_SetString( ConstructorError
                       , "Layer.setEnclosure(): First parameter is not of BasicLayer type" );
        return NULL;
      }
      layer->setEnclosure( basicLayer, PyAny_AsLong(pyDimension), flags );
    } else {
      PyErr_SetString( ConstructorError
                     , "Layer.setEnclosure(): Bad parameters types or numbers." );
      return NULL;
    }
    HCATCH

    Py_RETURN_NONE;
  }


  GetNameMethod               (Layer, layer)
  predicateFromLayer          (                          above             ,PyLayer,Layer)
  predicateFromLayer          (                          below             ,PyLayer,Layer)
  predicateFromLayer          (                          contains          ,PyLayer,Layer)
  predicateFromLayer          (                          intersect         ,PyLayer,Layer)
  predicateFromVoid           (                          isSymbolic        ,PyLayer,Layer)
  predicateFromVoid           (                          isBlockage        ,PyLayer,Layer)
  accessorDbuFromOptBasicLayer(                          getExtentionCap   ,PyLayer,Layer)
  accessorDbuFromOptBasicLayer(                          getExtentionWidth ,PyLayer,Layer)
  accessorCollectionFromVoid  (                          getBasicLayers    ,PyLayer,Layer,BasicLayer)
  accessorMaskFromVoid        (                          getMask           ,PyLayer,Layer)
  accessorMaskFromVoid        (                          getExtractMask    ,PyLayer,Layer)
  accessorLayerFromVoid       (                          getBlockageLayer  ,PyLayer,Layer)
  accessorLayerFromVoid       (                          getCut            ,PyLayer,Layer)
  accessorLayerFromVoid       (                          getTop            ,PyLayer,Layer)
  accessorLayerFromVoid       (                          getBottom         ,PyLayer,Layer)
  accessorLayerFromLayer      (                          getOpposite       ,PyLayer,Layer)
  accessorLayerFromOptBool    (                          getMetalAbove     ,PyLayer,Layer)
  accessorLayerFromOptBool    (                          getMetalBelow     ,PyLayer,Layer)
  accessorLayerFromOptBool    (                          getCutAbove       ,PyLayer,Layer)
  accessorLayerFromOptBool    (                          getCutBelow       ,PyLayer,Layer)
  accessorDbuFromUInt         (                          getTopEnclosure   ,PyLayer,Layer)
  accessorDbuFromUInt         (                          getBottomEnclosure,PyLayer,Layer)
  DirectGetLongAttribute      (PyLayer_getMinimalSize   ,getMinimalSize    ,PyLayer,Layer)
  DirectGetLongAttribute      (PyLayer_getMinimalSpacing,getMinimalSpacing ,PyLayer,Layer)
  DirectGetDoubleAttribute    (PyLayer_getMinimalArea   ,getMinimalArea    ,PyLayer,Layer)

  SetNameMethod(Layer, layer)
  updatorFromDbu          (setMinimalSize   ,PyLayer,Layer)
  updatorFromDbu          (setMinimalSpacing,PyLayer,Layer)
  updatorFromBasicLayerDbu(setExtentionCap  ,PyLayer,Layer)
  updatorFromBasicLayerDbu(setExtentionWidth,PyLayer,Layer)
  DirectSetBoolAttribute  (PyLayer_setSymbolic   ,setSymbolic   ,PyLayer,Layer)
  DirectSetBoolAttribute  (PyLayer_setBlockage   ,setBlockage   ,PyLayer,Layer)
  DirectSetDoubleAttribute(PyLayer_setMinimalArea,setMinimalArea,PyLayer,Layer)

  // Standart destroy (Attribute).
  DBoDestroyAttribute(PyLayer_destroy, PyLayer)


  PyMethodDef PyLayer_Methods[] =
    { { "getTechnology"       , (PyCFunction)PyLayer_getTechnology       , METH_NOARGS
                              , "Returns the technology owning the layer." }
    , { "getName"             , (PyCFunction)PyLayer_getName             , METH_NOARGS
                              , "Returns the name of the layer." }
    , { "getMask"             , (PyCFunction)PyLayer_getMask             , METH_NOARGS
                              , "Returns the mask (bits) of the layer." }
    , { "getExtractMask"      , (PyCFunction)PyLayer_getExtractMask      , METH_NOARGS
                              , "Returns the extract mask of the layer (for GDSII)." }
    , { "getMinimalSize"      , (PyCFunction)PyLayer_getMinimalSize      , METH_NOARGS
                              , "Returns the minimum width allowed for the layer." }
    , { "getMinimalArea"      , (PyCFunction)PyLayer_getMinimalArea      , METH_NOARGS
                              , "Returns the minimum area allowed for the layer." }
    , { "getMinimalSpacing"   , (PyCFunction)PyLayer_getMinimalSpacing   , METH_NOARGS
                              , "Returns the spacing allowed for the layer (edge to edge)." }
    , { "getBasicLayers"      , (PyCFunction)PyLayer_getBasicLayers      , METH_NOARGS
                              , "Returns the collection of BasicLayer the Layer is built upon." }
    , { "getBlockageLayer"    , (PyCFunction)PyLayer_getBlockageLayer    , METH_NOARGS
                              , "Returns the associated connector layer." }
    , { "getCut"              , (PyCFunction)PyLayer_getCut              , METH_NOARGS
                              , "Returns the cut layer (in case of a muti-layer Contact)." }
    , { "getTop"              , (PyCFunction)PyLayer_getTop              , METH_NOARGS
                              , "Returns the top layer (in case of a muti-layer)." }
    , { "getBottom"           , (PyCFunction)PyLayer_getBottom           , METH_NOARGS
                              , "Returns the bottom layer (in case of a muti-layer)." }
    , { "getOpposite"         , (PyCFunction)PyLayer_getOpposite         , METH_VARARGS
                              , "Returns the layer opposite the one given." }
    , { "getMetalAbove"       , (PyCFunction)PyLayer_getMetalAbove       , METH_VARARGS
                              , "Returns the metal layer above this one." }
    , { "getMetalBelow"       , (PyCFunction)PyLayer_getMetalBelow       , METH_VARARGS
                              , "Returns the metal layer below this one." }
    , { "getCutAbove"         , (PyCFunction)PyLayer_getCutAbove         , METH_VARARGS
                              , "Returns the cut layer above this one." }
    , { "getCutBelow"         , (PyCFunction)PyLayer_getCutBelow         , METH_VARARGS
                              , "Returns the cut layer below this one." }
    , { "getEnclosure"        , (PyCFunction)PyLayer_getEnclosure        , METH_VARARGS
                              , "Returns the enclosure (global or for one BasicLayer)." }
    , { "getTopEnclosure"     , (PyCFunction)PyLayer_getTopEnclosure     , METH_VARARGS
                              , "Returns the top layer enclosure." }
    , { "getBottomEnclosure"  , (PyCFunction)PyLayer_getBottomEnclosure  , METH_VARARGS
                              , "Returns the bottom layer enclosure." }
    , { "getExtentionCap"     , (PyCFunction)PyLayer_getExtentionCap     , METH_VARARGS
                              , "Returns the extention cap (global or for one BasicLayer)." }
    , { "getExtentionWidth"   , (PyCFunction)PyLayer_getExtentionWidth   , METH_VARARGS
                              , "Returns the extention width (global or for one BasicLayer)." }
    , { "above"               , (PyCFunction)PyLayer_above               , METH_VARARGS
                              , "Tells if the layer is above the one passed as argument." }
    , { "below"               , (PyCFunction)PyLayer_below               , METH_VARARGS
                              , "Tells if the layer is below the one passed as argument." }
    , { "contains"            , (PyCFunction)PyLayer_contains            , METH_VARARGS
                              , "Tells if the layer fully contains the one passed as argument." }
    , { "intersect"           , (PyCFunction)PyLayer_intersect           , METH_VARARGS
                              , "Tells if the layer share some BasicLayer with the one passed as argument." }
    , { "isSymbolic"          , (PyCFunction)PyLayer_isSymbolic          , METH_NOARGS
                              , "Tells if the layer is the symbolic one for this BasicLayer." }
    , { "isBlockage"          , (PyCFunction)PyLayer_isBlockage          , METH_NOARGS
                              , "Tells if the layer represent blockage." }
    , { "setName"             , (PyCFunction)PyLayer_setName             , METH_VARARGS
                              , "Allows to change the layer name." }
    , { "setSymbolic"         , (PyCFunction)PyLayer_setSymbolic         , METH_VARARGS
                              , "Sets the layer as the symbolic one." }
    , { "setBlockage"         , (PyCFunction)PyLayer_setBlockage         , METH_VARARGS
                              , "Sets the layer as blockage." }
    , { "setMinimalSize"      , (PyCFunction)PyLayer_setMinimalSize      , METH_VARARGS
                              , "Sets the layer minimal size (width)." }
    , { "setMinimalSpacing"   , (PyCFunction)PyLayer_setMinimalSpacing   , METH_VARARGS
                              , "Sets the layer minimal spacing (edge to edge)." }
    , { "setEnclosure"        , (PyCFunction)PyLayer_setEnclosure        , METH_VARARGS
                              , "Sets the enclosure for the given BasicLayer sub-component." }
    , { "setExtentionCap"     , (PyCFunction)PyLayer_setExtentionCap     , METH_VARARGS
                              , "Sets the extention cap for the given BasiLayer sub-component." }
    , { "setExtentionWidth"   , (PyCFunction)PyLayer_setExtentionWidth   , METH_VARARGS
                              , "Sets the extention width for the given BasiLayer sub-component." }
    , { "setMinimalArea"      , (PyCFunction)PyLayer_setMinimalArea      , METH_VARARGS
                              , "Sets the minimum area allowed for the layer." }
    , { "destroy"             , (PyCFunction)PyLayer_destroy             , METH_NOARGS
                              , "Destroy associated hurricane object The python object remains." }
    , {NULL, NULL, 0, NULL}   /* sentinel */
    };


  // ---------------------------------------------------------------
  // PyLayer Type Methods.


  DBoDeleteMethod(Layer)
  PyTypeObjectLinkPyType(Layer)


#else  // End of Python Module Code Part.


// +=================================================================+
// |              "PyLayer" Shared Library Code Part                 |
// +=================================================================+

  // Link/Creation Method.
  DBoLinkCreateMethod(Layer)

  PyTypeRootObjectDefinitions(Layer)


  extern  void  PyLayer_postModuleInit ()
  {
    PyDict_SetItemString ( PyTypeLayer.tp_dict, "Mask", (PyObject*)&PyTypeLayerMask );

    PyObject* constant;

    LoadObjectConstant(PyTypeLayer.tp_dict,Layer::NoFlags       ,"NoFlags"       );
    LoadObjectConstant(PyTypeLayer.tp_dict,Layer::EnclosureH    ,"EnclosureH"    );
    LoadObjectConstant(PyTypeLayer.tp_dict,Layer::EnclosureV    ,"EnclosureV"    );
    LoadObjectConstant(PyTypeLayer.tp_dict,Layer::EnclosureMax  ,"EnclosureMax"  );
    LoadObjectConstant(PyTypeLayer.tp_dict,Layer::ExtensionCap  ,"ExtensionCap"  );
    LoadObjectConstant(PyTypeLayer.tp_dict,Layer::ExtensionWidth,"ExtensionWidth");
  }


  extern PyObject* PyLayer_LinkDerived ( Layer* object )
  {
    if (object == NULL) Py_RETURN_NONE;

    BasicLayer* basicLayer = dynamic_cast<BasicLayer*>(object);
    if (basicLayer) return PyBasicLayer_Link(basicLayer);

    ContactLayer* contactLayer = dynamic_cast<ContactLayer*>(object);
    if (contactLayer) return PyContactLayer_Link(contactLayer);

    ViaLayer* viaLayer = dynamic_cast<ViaLayer*>(object);
    if (viaLayer) return PyViaLayer_Link(viaLayer);

    DiffusionLayer* diffusionLayer = dynamic_cast<DiffusionLayer*>(object);
    if (diffusionLayer) return PyDiffusionLayer_Link(diffusionLayer);

    RegularLayer* regularLayer = dynamic_cast<RegularLayer*>(object);
    if (regularLayer) return PyRegularLayer_Link(regularLayer);

    TransistorLayer* transistorLayer = dynamic_cast<TransistorLayer*>(object);
    if (transistorLayer) return PyTransistorLayer_Link(transistorLayer);

    Py_RETURN_NONE;
  }


  extern Layer* PYDERIVEDLAYER_O ( PyObject* pyObject )
  {
    Layer* layer = NULL;
    if (IsPyLayer(pyObject))                         layer = PYLAYER_O(pyObject);
    if (not layer and IsPyBasicLayer(pyObject)     ) layer = PYBASICLAYER_O(pyObject);
    if (not layer and IsPyContactLayer(pyObject)   ) layer = PYCONTACTLAYER_O(pyObject);
    if (not layer and IsPyViaLayer(pyObject)       ) layer = PYVIALAYER_O(pyObject);
    if (not layer and IsPyDiffusionLayer(pyObject) ) layer = PYDIFFUSIONLAYER_O(pyObject);
    if (not layer and IsPyRegularLayer(pyObject)   ) layer = PYREGULARLAYER_O(pyObject);
    if (not layer and IsPyTransistorLayer(pyObject)) layer = PYTRANSISTORLAYER_O(pyObject);
    return layer;
  }


  extern bool IsPyDerivedLayer ( PyObject* pyObject )
  {
    if (IsPyLayer(pyObject)          ) return true;
    if (IsPyBasicLayer(pyObject)     ) return true;
    if (IsPyContactLayer(pyObject)   ) return true;
    if (IsPyViaLayer(pyObject)       ) return true;
    if (IsPyDiffusionLayer(pyObject) ) return true;
    if (IsPyRegularLayer(pyObject)   ) return true;
    if (IsPyTransistorLayer(pyObject)) return true;
    return false;
  }


#endif  // End of Shared Library Code Part.

}  // extern "C".

}  // Isobar namespace. 
