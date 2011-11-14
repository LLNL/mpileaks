#ifndef _CALLPATH2COUNT_H_
#define _CALLPATH2COUNT_H_

#include <map>
#include <list>
#include "CallpathRuntime.h"             // Callpath

using namespace std; 


class Callpath2Count; 
extern list<Callpath2Count*> *h2cpc_objs; 


struct callpath_count { 
  Callpath path;
  int count;
}; 

typedef struct callpath_count callpath_count_t; 


/* 
 * Root, no-template class. 
 * This class allows a uniform interface to point to 
 * derived classes that can be instantiated. Class
 * 'Handle2CPC' can't serve this purpose because it is 
 * a template class. 
 */ 
class Callpath2Count
{
 public: 
  /******************************************************
   * Constructor and destructor
   ******************************************************/
  /* Register instances of this class with global container.
     Since instances of this class are global, e.g., MPI_Request2Callpath, 
     we need to make sure that global container gets allocated 
     before this instance. 
     Todo: there may be a race condition if instances of this class are 
     allocated in parallel. */ 
  Callpath2Count() {
    if ( h2cpc_objs == NULL ) {
      h2cpc_objs = new list<Callpath2Count*>; 
    }
    
    h2cpc_objs->push_back( this );
  }
  
  /* Virtual destructor required to avoid compiler warning */ 
  virtual ~Callpath2Count() {
    /* TODO: remove ourselves from h2cpc_objs, and if empty, delete this object */
  }
    
  
  /******************************************************
   * Auxiliary functions to be used by derived classes
   ******************************************************/
  void increase_count(map<Callpath,int> &callpath2count, Callpath path, int count) {
    /* search for this path in our stacktrace-to-count map */
    map<Callpath,int>::iterator it_path2count = callpath2count.find(path);
    if ( it_path2count != callpath2count.end() ) {
      /* found it, increment the count for this path */
      it_path2count->second += count;
    } else {
      /* not found, so insert the path with the specified count */
      callpath2count[path] = count;
    } 
  }
  
  void decrease_count(map<Callpath,int> &callpath2count, Callpath path, int count) {
    /* now lookup path in path2count */
    map<Callpath,int>::iterator it_path2count = callpath2count.find(path);
    if (it_path2count != callpath2count.end()) {
      if (it_path2count->second - count > 1) {
	/* decrement the count for this path */
	it_path2count->second -= count;
      } else {
        if (it_path2count->second - count < 0) {
          /* we subtracted more than we ever added */
          cerr << "mpileaks: Internal Error: Callpath2Count: "
               << "negative count detected" 
               << endl;
        }

	/* remove the entry from the path-to-count map */
	callpath2count.erase(it_path2count);
      }
    } else {
      cerr << "mpileaks: Internal Error: Callpath2Count: "
	   << "found a path in handle2cpc, but no count found in callpath2count" 
	   << endl;
    }
  }
  
  int map2list(map<Callpath,int> &callpath2count, list<callpath_count_t> &lst) {
    int count = 0; 
    callpath_count_t entry; 
    map<Callpath,int>::iterator it;
    
    for (it = callpath2count.begin(); it != callpath2count.end(); it++) {
      entry.path  = it->first;
      entry.count = it->second;
      lst.push_back( entry );
      count++; 
    }
    
    return count;  
  }


  /******************************************************
   * Access functions 
   ******************************************************/
  virtual int get_definite_leaks(list<callpath_count_t> &lst) = 0; 
  virtual int get_possible_leaks(list<callpath_count_t> &lst) = 0; 
  
  int get_missing_alloc_leaks(list<callpath_count_t> &lst) {
    return map2list( missing_alloc, lst ); 
  } 

  
 protected: 
  /* map of callpath to count associated with no-allocate leaks */ 
  map<Callpath, int> missing_alloc; 
}; 




#endif    // _CALLPATH2COUNT_H_
