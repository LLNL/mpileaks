#ifndef _MPILEAKS_H_
#define _MPILEAKS_H_

#include <map>
#include <stack>
#include "CallpathRuntime.h"

using namespace std; 

extern int enabled;
extern int depth;
extern CallpathRuntime *runtime;

void callpath_increase_count(Callpath path); 
void callpath_decrease_count(Callpath path); 

extern map<Callpath,int> callpath2count_missing_alloc;

void callpath2count_increase(map<Callpath,int> &callpath2count, Callpath path); 
void callpath2count_decrease(map<Callpath,int> &callpath2count, Callpath path); 


/*
 * Abstract class (cannot be instantiated): 
 * Handle to callpath container (Handle2CPC).  
 * Associate a handle of type T with a callpath container of type U. 
 * 
 * Note: Forced to include full definition of member functions 
 * to avoid linking errors. This occurs because this class is a
 * template and is being used in a different file. 
 */ 
template<class T, class U> class Handle2CPC
{
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
  virtual void add_map_entry(T handle, Callpath path) = 0; 
  virtual void del_map_entry(typename map<T,U>::iterator it) = 0;   
  
  /******************************************************
   * Functions to be used in all derived classes
   ******************************************************/
  Callpath get_callpath(size_t start) {
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

  void allocate( T &handle ) {
    if (enabled) {
      if ( !is_handle_null(handle) ) {
	/* get the call path where this request was allocated,
         * chop off 4 layers of mpileaks calls */
	Callpath path = get_callpath(4);

	/* increase callpath count */
	callpath_increase_count(path);
	
	/* associate handle with callpath */ 	
	add_map_entry(handle, path); 
      } 
    }
  }
  
  void free( T &handle ) {
    if (enabled) {
      if ( !is_handle_null(handle) ) {
	/* lookup stack based on handle value */
	typename map<T,U>::iterator it = handle2cpc.find(handle);
	
	if ( it != handle2cpc.end() ) {
	  /* found handle entry, update callback2count map and rm entry from handle2cpc */ 
	  del_map_entry( it );
	} else  {
	  /* Non-null handle being freed but not found in handle2cpc,
           * capture the callpath of the free call to report later */
	  Callpath path = get_callpath(4);

	  /* increase callpath count for this free call */
	  callpath2count_increase(callpath2count_missing_alloc, path);
        }
      }
    }
  }
  
protected:
  /* handle to callpath container */ 
  map<T, U> handle2cpc;
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
 public:
  void add_map_entry(T handle, Callpath path) {
    Handle2CPC<T,Callpath>::handle2cpc[handle] = path; 
  }
  
  void del_map_entry(typename map<T,Callpath>::iterator it) {
    /* get callpath and update path in callpath2count */ 
    callpath_decrease_count( it->second );
    /* rm entry from handle to callpath_container */ 
    Handle2CPC<T,Callpath>::handle2cpc.erase(it); 
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
template<class T> class Handle2Stack : public Handle2CPC<T, stack<Callpath>*>
{
 public:
  void add_map_entry(T handle, Callpath path) {

    /* look for an existing handle in Handle2CPC */ 
    typename map<T, stack<Callpath>*>::iterator it =
      Handle2CPC<T,stack<Callpath>*>::handle2cpc.find(handle); 

    if ( it != Handle2CPC<T,stack<Callpath>*>::handle2cpc.end() ) {
      /* handle found */ 
      it->second->push(path); 
    } else {
      /* handle not found, create new stack of callpaths, push current */ 
      Handle2CPC<T,stack<Callpath>*>::handle2cpc[handle] = new stack<Callpath>; 
      Handle2CPC<T,stack<Callpath>*>::handle2cpc[handle]->push(path); 
    }
  }

  void del_map_entry(typename map<T,stack<Callpath>*>::iterator it) {
    if ( !it->second->empty() ) {
      /* pop callpath from stack and update callpath2count */ 
      callpath_decrease_count( it->second->top() );
      it->second->pop(); 
      /* if stack is empty, delete entry */ 
      if ( it->second->empty() ) {
        delete it->second; 
	Handle2CPC<T,stack<Callpath>*>::handle2cpc.erase(it); 
      }
    } else {
      cerr << "mpileaks: INTERNAL ERROR: callpath stack should not be empty" << endl; 
    }
  }  
};



#endif 
