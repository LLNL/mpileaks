/* Copyright (c) 2012, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Adam Moody <moody20@llnl.gov> and Edgar A. Leon <leon@llnl.gov>.
 * LLNL-CODE-557543.
 * All rights reserved.
 * This file is part of the mpileaks tool package.
 * For details, see https://github.com/hpc/mpileaks
 * Please also read this file: LICENSE.TXT. */

#ifndef _MPILEAKS_H_
#define _MPILEAKS_H_

#include <map>
#include <set>
#include <stack>
#include <list>
#include <utility>                       // pair
#include "CallpathRuntime.h"             // Callpath
#include "callpath2count.h"                // Callpath2Count, callpath_count_t


using namespace std; 

/* Todo: need to encapsulate globals into one struct */ 
extern int enabled;
extern int depth;
extern int chop;
extern CallpathRuntime *runtime;



/*
 * Abstract class (cannot be instantiated): 
 * Handle to callpath container (Handle2CPC).  
 * Associate a handle of type T with a callpath container of type U. 
 * 
 * Note: Forced to include full definition of member functions 
 * to avoid linking errors. This occurs because this class is a
 * template and is being used in a different file. 
 */ 
template<class T, class U> class Handle2CPC : public Callpath2Count
{
 private: 
  /******************************************************
   * Alias for iterator of class member 
   ******************************************************/
  typedef typename map<T,U>::iterator myiterator; 


public:
  /******************************************************
   * Virtual destructor required to avoid compiler warning
   ******************************************************/
  virtual ~Handle2CPC() {
  }

  
  /******************************************************
   * Pure virtual functions, define in derived classes 
   ******************************************************/
  virtual bool is_handle_null(T handle) = 0; 
  virtual void add_callpath(T handle, Callpath path) = 0; 
  virtual void remove_callpath(myiterator it, size_t start) = 0; 


  /******************************************************
   * Auxiliary functions
   ******************************************************/
  Callpath get_callpath(size_t start) {
    /* we wait to create our runtime object as late as possible
     * because it registers the SIGSEGV signal within stackwalker
     * and we want to override any previous registrations for this
     * signal, in particular by MPI  */
    if (runtime == NULL) {
      runtime = new CallpathRuntime;
    }

    /* get the current call path */
    Callpath path = runtime->doStackwalk();

    /* assume we want the entire path going all the way up to main(),
     * unless depth is specified, then just take the number requested */
    size_t size = path.size();
    size_t end = size;
    if (depth > -1) {
      end = start + depth;
      if (end > size) {
        end = size;
      }
    }

    /* chop off frames that are within the mpileaks code itself,
     * and only show frames up to certain depth along path to main() */
    Callpath sliced = path.slice(start, end);
    
    return sliced;
  }


  /******************************************************
   * The main functions of this class. Derived classes 
   * that are instantiated use these functions. 
   ******************************************************/
  void allocate(T &handle, size_t start) {
    if (enabled) {
      if ( !is_handle_null(handle) ) {
	/* get the call path where this request was allocated,
         * chop layers of mpileaks and internal MPI calls */
	Callpath path = get_callpath(start+1);
	
	/* associate handle with callpath */ 	
	add_callpath(handle, path); 
      } 
    }
  }
  
  void free(T &handle, size_t start) {
    if (enabled) {
      if ( !is_handle_null(handle) ) {
	/* lookup stack based on handle value */
	myiterator it = handle2cpc.find(handle);
	if ( it != handle2cpc.end() )
	  /* found handle entry, decrease count associated with handle */ 
	  remove_callpath(it, start+1); 
	else {
	  /* Non-null handle being freed but not found in handle2cpc,
           * capture the callpath of the free call to report later */
	  Callpath path = get_callpath(start+1);
	  
	  /* increase callpath count for this free call */
	  increase_count(missing_alloc, path, 1); 
        }
      }
    }
  }
  
 protected:
  /* handle to callpath-container */ 
  map<T, U> handle2cpc;
}; 



/*
 * Abstract class Handle2Set:  
 * Associate a 'handle' with a pair '(set_of_callpaths, count)'. 
 *   'count': number of active allocate calls using 'handle'. 
 *   'set_of_callpaths': callpaths associated with allocate requests;
 *                       they are not freed until count is zero. 
 * This class covers the general case where one handle can be associated with
 * multiple callpaths. 
 */
template<class T> class Handle2Set : public Handle2CPC< T, pair<set<Callpath>,int> >
{
 private:
  /******************************************************
   * Alias for iterator of class member 
   ******************************************************/
  typedef typename map< T, pair<set<Callpath>,int> >::iterator myiterator; 
  
 public:
  void add_callpath(T handle, Callpath path) {
    /* locate map entry associated with handle */ 
    if ( this->handle2cpc.find(handle) != this->handle2cpc.end() ) {
      /* handle found */ 
      this->handle2cpc[handle].second++;
    } else {
      /* handle not found, initialize count */ 
      this->handle2cpc[handle].second = 1;
    }
    
    /* insert path to the set of callpaths associated with handle */ 
    /* map[handle].set_of_callpaths.insert */ 
    this->handle2cpc[handle].first.insert( path );
  }
  
  void remove_callpath(myiterator it, size_t start) {
    if ( it->second.first.empty() || it->second.second <= 0 ) {
      /* handle being freed but no callpaths in set,
       * capture the callpath of the free call to report later */
      Callpath path = this->get_callpath(start);
      
      /* increase callpath count for this free call */
      this->increase_count(this->missing_alloc, path, 1); 
      
      /* clean-up map entry */ 
      if ( !it->second.first.empty() ) { 
	it->second.first.clear(); 
      }
      this->handle2cpc.erase( it ); 
    } else { 
      /* if count decreases to 0, remove entry */ 
      /* it->pair.count */ 
      if (--(it->second.second) == 0) {
	/* drop all callpaths in set */ 
	it->second.first.clear();
	/* erase map entry */ 
	this->handle2cpc.erase(it); 
      }
    }
  }

  /* identify all handles that map to a single callpath,
   * and for the union of all such callpaths, sum the total outstanding count by callpath */
  int get_definite_leaks(list<callpath_count_t> &lst) {
    /* we use this to sum counts by callpath */
    map<Callpath,int> tmp_callpath2count;
    
    /* Iterate over map of handle to set of callpaths */ 
    myiterator it_map; 
    for ( it_map = this->handle2cpc.begin(); 
	  it_map != this->handle2cpc.end(); it_map++ )
    { 
      
      /* If there's only one leak source (definite) */ 
      if ( it_map->second.first.size() == 1 ) {
#if 0
	cerr << "ea: definite: setsize = " << it_map->second.first.size() 
	     << " count = " << it_map->second.second << endl; 
#endif 
	Callpath path = *(it_map->second.first.begin()); 
	int count = it_map->second.second; 
        this->increase_count(tmp_callpath2count, path, count);
      }
    }

    /* now build a list of counts by callpath */
    return this->map2list(tmp_callpath2count, lst);
  }
  
  /* identify all handles that map to more than one callpath,
   * and for the union of all such callpaths, sum the total outstanding count by callpath */
  int get_possible_leaks(list<callpath_count_t> &lst) {
    /* we use this to sum counts by callpath */
    map<Callpath,int> tmp_callpath2count;

    /* Iterate over map of handle to set of callpaths */ 
    myiterator it_map; 
    for ( it_map = this->handle2cpc.begin(); 
	  it_map != this->handle2cpc.end(); it_map++ )
    { 
      
      /* If there's more than one possible leak source */ 
      if ( it_map->second.first.size() > 1 ) { 
#if 0
	cerr << "ea: possible: setsize = " << it_map->second.first.size() 
	     << " count = " << it_map->second.second << endl; 
#endif 
	/* Iterate over the set of callpaths */ 
        set<Callpath>::iterator it_set; 
	for ( it_set = it_map->second.first.begin(); 
	      it_set != it_map->second.first.end(); it_set++ )
        { 
	  /* Flatten the set of callpaths into a list with the set's count */ 
	  Callpath path = *it_set; 
	  int count = it_map->second.second; 
          this->increase_count(tmp_callpath2count, path, count);
	}
      }
    }
    
    return this->map2list(tmp_callpath2count, lst); 
  }

  /* for debugging purposes */ 
  void print_outstanding() {
    myiterator it; 
    for (it = this->handle2cpc.begin(); it != this->handle2cpc.end(); it++) {
      cout << "handle=" << it->first << " count=" << it->second.second << endl; 
    }
  }
};



/*
 * Abstract class Handle2Callpath.
 * Derived classes of this class need to define: 
 *   bool is_handle_null(T handle)
 * This class uses 'callpath' as a callpath container. 
 * This is the simplest usage of a callpath container where a 
 * handle is associated with only one callpath. 
 */
template<class T> class Handle2Callpath : public Handle2CPC<T, Callpath>
{
 private:
  /******************************************************
   * Alias for iterator of class member 
   ******************************************************/
  typedef typename map<T,Callpath>::iterator myiterator; 

 public:
  void add_callpath(T handle, Callpath path) {
    /* locate map entry associated with handle */ 
    myiterator it = this->handle2cpc.find(handle); 

    if ( it == this->handle2cpc.end() ) { 
      this->handle2cpc[handle] = path; 
      this->increase_count( callpath2count, path, 1 ); 
    } else {
      /* found handle! */ 
      cerr << "mpileaks: Internal Error: Handle2Callpath: "
	   << "attempting to overwrite callpath of existing handle;" 
	   << endl << "  cannot associate one handle to more than one callpath "
	   << "(use Handle2Set instead)." << endl;       
    }
  }

  void remove_callpath(myiterator it, int start) {
    /* get callpath and update path in callpath2count */ 
    this->decrease_count( callpath2count, it->second, 1 );

    /* rm entry from handle to callpath_container */ 
    this->handle2cpc.erase( it ); 
  }

  int get_definite_leaks(list<callpath_count_t> &lst) {
    return this->map2list(callpath2count, lst); 
  }

  int get_possible_leaks(list<callpath_count_t> &lst) {
    return 0; 
  }
  

 protected: 
  /* map of callpath to count */ 
  map<Callpath, int> callpath2count; 
};


/*
 * Abstract class Handle2Stack.
 * Derived classes of this class need to define: 
 *   bool is_handle_null(T handle)
 * This class uses 'stack<callpath>' as a callpath container. 
 * A handle is associated with a stack of callpaths. 
 */
template<class T> class Handle2Stack : public Handle2CPC< T, stack<Callpath> >
{
 private:
  /******************************************************
   * Alias for iterator of class member 
   ******************************************************/
  typedef typename map< T, stack<Callpath> >::iterator myiterator; 
  
 public:
  void add_callpath(T handle, Callpath path) {
    this->handle2cpc[handle].push( path ); 
    this->increase_count( callpath2count, path, 1 ); 
  }
  
  void remove_callpath(myiterator it, int start) {
    if ( !it->second.empty() ) {
      /* pop callpath from stack and update callpath2count */ 
      this->decrease_count( callpath2count, it->second.top(), 1 );
      it->second.pop(); 

      /* if stack is empty, delete entry */ 
      if ( it->second.empty() ) {
	this->handle2cpc.erase(it); 
      }
    } else {
      /* handle being freed without any associated callpaths; 
       * capture the callpath of the free call to report later */
      Callpath path = this->get_callpath(start);

      /* increase callpath count for this free call */
      this->increase_count( this->missing_alloc, path, 1 );
    }
  }
  
  /* Todo: need to think about what definite and possible 
     mean in this context. For now, using same policy as if 
     a one-to-one mapping of handle to callpath exists. */ 
  int get_definite_leaks(list<callpath_count_t> &lst) {
    return this->map2list(callpath2count, lst); 
  }

  int get_possible_leaks(list<callpath_count_t> &lst) {
    return 0; 
  }


 protected: 
  /* map of callpath to count */ 
  map<Callpath, int> callpath2count;   
};



#endif   // _MPILEAKS_H_
