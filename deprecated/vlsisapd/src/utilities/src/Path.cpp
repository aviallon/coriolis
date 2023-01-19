// -*- C++ -*-
//
// This file is part of the VLSI Stand-Alone Software.
// Copyright (c) UPMC 2013-2018, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |      V L S I  Stand - Alone  Parsers / Drivers                  |
// |    M i s c e l l a n e o u s   U t i l i t i e s                |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :  "./Path.cpp"                                    |
// +-----------------------------------------------------------------+


#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include "vlsisapd/utilities/Path.h"


namespace Utilities {

  using namespace std;


  int  Path::_toUnistd ( unsigned int mode )
  {
    int  unimode = 0;
    if (mode & PermRead   ) unimode |= R_OK;
    if (mode & PermWrite  ) unimode |= W_OK;
    if (mode & PermExecute) unimode |= X_OK;
    return unimode;
  }


  int  Path::_toMode_T ( unsigned int mode )
  {
    int  unimode = 0;
    if (mode & s_isuid   ) unimode |= S_ISUID;
    if (mode & s_isgid   ) unimode |= S_ISGID;
    if (mode & s_isvtx   ) unimode |= S_ISVTX;
    if (mode & s_irusr   ) unimode |= S_IRUSR;
    if (mode & s_iwusr   ) unimode |= S_IWUSR;
    if (mode & s_ixusr   ) unimode |= S_IXUSR;
    if (mode & s_irgrp   ) unimode |= S_IRGRP;
    if (mode & s_iwgrp   ) unimode |= S_IWGRP;
    if (mode & s_ixgrp   ) unimode |= S_IXGRP;
    if (mode & s_iroth   ) unimode |= S_IROTH;
    if (mode & s_iwoth   ) unimode |= S_IWOTH;
    if (mode & s_ixoth   ) unimode |= S_IXOTH;
    return unimode;
  }
  
  
  void  Path::_split ( const string& path, vector<string>& elements, unsigned int& flags )
  {
    if (path.empty()) return;

    size_t begin = 0;
    if (path[0] == '/') {
      flags |= Absolute;
      begin = 1;
    }
  
    size_t end = path.find_first_of( '/', begin );
    while ( end != string::npos ) {
      elements.push_back(path.substr(begin,end-begin));
      begin = end+1;
      end   = path.find_first_of( '/', begin );
    }
    elements.push_back(path.substr(begin));
  }
  
  
  void  Path::_normalize ()
  {
    if ( (size() == 1) and (_elements[0] == ".") ) return;

    _flags |= Invalidated;
  
    if (not _elements.empty() and (_elements[0] == "~")) {
      char* home = getenv( "HOME" );
      if (home) {
        vector<string> vhome;
        unsigned int   vflags = 0;
        _split( home, vhome, vflags );
        _flags |= vflags;
        _elements.insert( _elements.begin(), vhome.begin(), vhome.end() );
      }
    }
  
    vector<string>::iterator ie = _elements.begin();
    while ( ie != _elements.end() ) {
      if ( ((*ie) == ".") or (*ie).empty()) {
        _elements.erase(ie);
      } else {
        ++ie;
      }
    }
  
    ie = _elements.begin();
    while ( ie != _elements.end() ) {
      if ( ((*ie) == "..") and (ie != _elements.begin()) ) {
        --ie;
        _elements.erase(ie,ie+2);
      } else {
        ++ie;
      }
    }
  
    if (not _elements.empty() and (_elements.back().find_last_of('.') != string::npos))
      _flags |= Extension;
  
    // cout << "_normalize(" << path << ")" << endl;
    // cout << "  Absolute: " << boolalpha << (bool)(_flags & Absolute) << endl;
    // for ( size_t i=0 ; i<_elements.size() ; ++i ) {
    //   cout << "  " << i << ": <" << _elements[i] << '>' << endl;
    // }
  }
  
  
  void  Path::_normalize ( const string& path )
  {
    _split( path, _elements, _flags );
    _normalize();
  }
  
  
  string  Path::ext () const
  {
    if (not (_flags&Extension)) return "";
    return _elements.back().substr(_elements.back().find_last_of('.')+1); 
  }


  bool  Path::mkdir ( unsigned int mode ) const
  {
    int status = ::mkdir( c_str(), _toMode_T(mode) );
    return (status == 0);
  }


  vector<Path>  Path::listdir () const
  {
    vector<Path> entries;

    if (isdir()) {
      DIR*           fdir   = NULL;
      struct dirent* fentry = NULL;
      fdir = opendir( toString().c_str() );
      if (fdir) {
        while ( (fentry = readdir(fdir)) != NULL ) {
          string name = fentry->d_name;
          if ( (name == ".") or (name == "..") ) continue;
          
          entries.push_back( join(name) );
        }
      }
    }
    return entries;
  }
  
  
  const string& Path::toString () const
  {
    if (_flags & Invalidated) {
      _pathcache.clear();
  
      for ( size_t i=0 ; i<size() ; ++i ) {
        if ((i != 0) or (_flags & Absolute)) {
          _pathcache += '/';
        }
        _pathcache += _elements[i];
      }
    }
    return _pathcache;
  }
  
  
  Path  Path::subpath ( size_t begin, size_t end ) const
  {
    if ((begin > size()) or (begin > end)) return Path();
    if (end > size()) end = size()-1;
  
    Path sub;
    for ( size_t i=begin; i<end ; ++i )
      sub._elements.push_back( _elements[i] );
    if ((begin == 0) and (_flags & Absolute)) sub._flags |= Absolute;
  
    return sub;
  }


  Path  Path::stem () const
  {
    string  dotext = ext();
    if (not dotext.empty()) dotext.insert( 0, "." );
    return basename(dotext);
  }
  
  
  Path  Path::basename ( const string& ext ) const
  {
    if (empty()) return Path();
  
    string basename = _elements.back();
    if (not ext.empty() and (basename.substr(basename.size()-ext.size()) == ext))
      basename = basename.substr(0,basename.size()-ext.size());
    return Path(basename);
  }
  
  
  Path  Path::dirname () const
  {
    if (empty()) return Path();
    if (size() == 1) return Path(".");
    return subpath( 0, size()-1 );
  }
  
  
  bool  Path::exists () const
  {
    struct stat  statinfos;
    int          status = 0;
  
    status = ::stat( toString().c_str(), &statinfos );
    if (not status) return true;
  
    return false;
  }
  
  
  bool  Path::access ( unsigned int mode ) const
  {
    int  status = 0;
  
    status = ::access( toString().c_str(), _toUnistd(mode) );
    if (not status) return true;
  
    return false;
  }


  unsigned int  Path::mode () const
  {
    struct stat  bstat;
    bstat.st_dev     = 0;
    bstat.st_ino     = 0;
    bstat.st_mode    = 0;
    bstat.st_nlink   = 0;
    bstat.st_uid     = 0;
    bstat.st_gid     = 0;
    bstat.st_rdev    = 0;
    bstat.st_size    = 0;
    bstat.st_blksize = 0;
    bstat.st_blocks  = 0;
  //int          status = 0;
  //status =
      ::stat( toString().c_str(), &bstat );
  // Should check for errno here.

    return bstat.st_mode;
  }


  Path::Stat  Path::stat () const
  {
    Stat stat;
    if (exists()) ::stat( _pathcache.c_str(), stat.c_stat() );
    return stat;
  }
  
  
  Path  Path::join ( const Path& tail ) const
  {
    Path result ( *this );
    result._elements.insert( result._elements.end(), tail._elements.begin(), tail._elements.end() );
    result._normalize();
    return result;
  }
  

  Path& Path::operator/= ( const Path& tail )
  {
    _elements.insert( _elements.end(), tail._elements.begin(), tail._elements.end() );
    _normalize();
    return (*this);
  }

  
  Path& Path::operator= ( const Path& other )
  {
    vector<string>().swap( _elements );
    _elements  = other._elements;
    _flags     = other._flags;
    _pathcache = other._pathcache;
    return *this;
  }


  Path  Path::cwd ()
  {
    static char curdir [4096];
    if (::getcwd( curdir, 4096 ) == NULL) {
      strcpy( curdir, "/cwd/has/failed");
    }

    return Path(curdir);
  }


} // Utilities namespace.
