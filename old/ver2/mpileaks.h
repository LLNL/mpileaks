#ifndef _MPILEAKS_H_
#define _MPILEAKS_H_

#include <map>
#include "CallpathRuntime.h"

using namespace std; 

extern int enabled; 
extern CallpathRuntime *runtime; 

void callpath_increase_count(Callpath path); 
void callpath_decrease_count(Callpath path); 

/* 
 * Associate a handle of type T with a callpath. 
 * 
 * Note: Forced to include full definition of member functions 
 * to avoid linking errors. This occurs because this class is a
 * template and is being used in a different file. 
 */ 
template<class T, class Comp> class Handle2Callpath {
 public: 
  void allocate( T& handle ) {
    if (enabled) {
      if (!Comp::isnull(handle)) {
	/* get the call path where this request was allocated */
	Callpath path = runtime->doStackwalk(); 
	
	/* increase callpath count */
	callpath_increase_count(path); 
	
	/* associate handle with callpath */ 
	handle2callpath[handle] = path; 
      } 
    }
  }
  
  void free( T& handle ) {
    if (enabled) {
      if (!Comp::isnull(handle)) {
	
	/* todo: figure out why the compiler does not like this: */ 
	// map<T,Callpath>::iterator it = handle2callpath.find(handle);
	
	/* lookup stack based on handle value */
	if (handle2callpath.find(handle) != handle2callpath.end()) {
	  /* found handle; decrease callpath count */ 
	  callpath_decrease_count(handle2callpath.find(handle)->second); 
	  /* erase entry from handle to callpath map */ 
	  handle2callpath.erase(handle2callpath.find(handle)); 
	} else {
	  cout << "mpileaks: ERROR: Non-null handle being freed but not found in handle2callpath" 
	       << endl;
	}
      }
    }
  }

 private: 
  map<T, Callpath> handle2callpath;
}; 



/*
 * Generic class whose functions should be instantiated 
 * by each type of handle used. 
 */

template<class T> class Handle {
 public: 
  static int isnull(T& handle); 
}; 


#endif 
