//-----------------------------------------------------------------------------
// Torque Game Engine 
// Quake GL DirectX wrapper
//-----------------------------------------------------------------------------

#ifndef _GLLIST_H_
#define _GLLIST_H_

//#define USE_IOSTREAM 1

#if defined(USE_IOSTREAM) 
#include <iostream>
#endif

// forward class declarations

template<class Type> class GLListIter;
template<class Type> class GLListManip;

template<class Type> class GLLink {

  // Private members

  Type d_data;
  GLLink<Type> *d_next_p;

  // Links cannot be copied or assigned

  GLLink(const GLLink<Type>&);
  GLLink<Type>& operator=(const GLLink<Type>&);

  // Public members
  
public:

  // Construct & Destroy
  
  inline GLLink(GLLink<Type> **addLinkPtr, const Type &data);
  ~GLLink() {}			
  
  // Modifiers
  
  inline void setData(const Type &data);
  inline void setNext(GLLink<Type> *link);
  inline GLLink<Type>*& getNextRef(); // generally a bad practice
  
  // Accessors
  
  inline Type& getData();
  inline GLLink<Type>* getNext() const;

};

template<class Type> class GLList {

  // Private members

  int d_length;
  GLLink<Type> *d_head_p;

  // Friends
  
  friend class GLListIter<Type>;
  friend class GLListManip<Type>;

  // Public members
  
public:

  // Construct & Destroy

  GLList();
  GLList(const GLList<Type>&);
  ~GLList();

  // Modifiers
  
  GLList<Type>& operator=(const GLList<Type> &list);
  GLList<Type>& operator+=(const Type &i);  
  GLList<Type>& operator+=(const GLList<Type> &list); 
  GLList<Type>& prepend(const Type &i);
  GLList<Type>& prepend(const GLList<Type> &list); 

  // Accessors
  
  int length() const;

};

//template<class Type> ostream& operator<<(ostream& o, const GLList<Type>& list);

template<class Type> class GLListIter {

  // Private Data

  GLLink<Type> *d_current_p;

  // Public Members
  
public:

  // Construct and Destroy

  inline GLListIter(const GLList<Type> &list);
  inline GLListIter(const GLListIter<Type> &iter);
  ~GLListIter() {}

  // Modifiers
  inline GLListIter<Type>& operator=(const GLListIter<Type> &iter);
  inline void operator++();

  // Accessors
  inline operator const void* () const;
  inline Type& operator()() const;

};

template<class Type> class GLListManip {

  // Private Data

  GLList<Type> *d_list_p;
  GLLink<Type> **d_current_p;
  
  // Links cannot be copied or assigned

  GLListManip(const GLListManip<Type> &manip);	  
  GLListManip<Type>& operator=(const GLListManip<Type> &manip);

  // Public Members

public:
  // Construct and Destroy
  inline GLListManip(GLList<Type> *list);
  ~GLListManip() {}

  // Modifiers
  inline void operator++();
  inline void insert (const Type &data);
  inline void remove ();

  // Accessors
  inline operator const void *() const;
  inline Type& operator()() const;

};

template<class Type> GLLink<Type>::GLLink(GLLink<Type> **addLinkPtr, const Type &data) : d_next_p(*addLinkPtr), d_data(data)
{
  *addLinkPtr = this;
}

template<class Type> void GLLink<Type>::setData(const Type &data)
{
  d_data = data;
}

template<class Type> void GLLink<Type>::setNext(GLLink<Type> *link)
{
  d_next_p = link;
}

template<class Type> GLLink<Type>*& GLLink<Type>::getNextRef()
{
  return d_next_p;
}

template<class Type> Type& GLLink<Type>::getData()
{
  return d_data;
}

template<class Type> GLLink<Type>* GLLink<Type>::getNext() const
{
  return d_next_p;
}

template<class Type> GLList<Type>::GLList() : d_head_p(0), d_length(0)
{
}

template<class Type> GLList<Type>::GLList(const GLList<Type>& list) : d_head_p(0)
{
  GLListManip<Type> m(this);
  GLListIter<Type> l(list);

  while(l) {
    m.insert(l());
    ++l;
    ++m;
  }
}

template<class Type> GLList<Type>::~GLList()
{
  GLListManip<Type> m(this);

  while(m != 0)
    m.remove();
}

template<class Type> GLList<Type>& GLList<Type>::operator=(const GLList<Type>& list)
{
  GLListManip<Type> m(this);
  GLListIter<Type> l(list);

  if(this != &list) {
    while(m)
      m.remove();
    while(l) {
      m.insert(l());
      ++l;
      ++m;
    }
  }
  return *this;
}

template<class Type> GLList<Type>& GLList<Type>::operator+=(const Type &i)
{
  GLListManip<Type> m(this);

  while(m)
    ++m;
  m.insert(i);
  return *this;
}

template<class Type> GLList<Type>& GLList<Type>::operator+=(const GLList<Type>& list)
{
  unsigned i, s;
  GLListIter<Type> l(list);
  GLListManip<Type> m(this);
  
  while(m)
    ++m;
  s = list.d_length;
  for(i = 0; i < s; ++i) {
    m.insert(l());
    ++m;
    ++l;
  }
  return *this;
}

template<class Type> GLList<Type>& GLList<Type>::prepend(const Type &i)
{
  GLListManip<Type> m(this);

  m.insert(i);
  return *this;
}

template<class Type> GLList<Type>& GLList<Type>::prepend(const GLList<Type> &list)
{
  GLListIter<Type> l(list);
  GLListManip<Type> m(this);

  while(l) {
    m.insert(l());
    ++m;
    ++l;
  }
  return *this;
}

template<class Type> int GLList<Type>::length() const
{
  return d_length;
}

#if defined(USE_IOSTREAM)
template<class Type> ostream& operator<<(ostream &o, const GLList<Type>& list)
{
  GLListIter<Type> l(list);

  o << "[ ";
  while(l != 0) {
    o << l();
    o << " ";
    ++l;
  }
  return o << "]";
}
#endif

template<class Type> GLListIter<Type>::GLListIter(const GLList<Type> &list) : d_current_p(list.d_head_p)
{
}

template<class Type> GLListIter<Type>::GLListIter(const GLListIter<Type> &iter) : d_current_p(iter.d_current_p)
{
}

template<class Type> GLListIter<Type>& GLListIter<Type>::operator=(const GLListIter<Type> &iter)
{
  d_current_p = iter.d_current_p;
  return *this;
}

template<class Type> void GLListIter<Type>::operator++()
{
  d_current_p = d_current_p -> getNext();
}

template<class Type> Type& GLListIter<Type>::operator()() const
{
  return d_current_p -> getData();
}

template<class Type> GLListIter<Type>::operator const void* () const
{
  return d_current_p;
}

template<class Type> GLListManip<Type>::GLListManip(GLList<Type> *list) : d_current_p(&(list -> d_head_p)), d_list_p(list)
{
}

template<class Type> void GLListManip<Type>::operator++()
{
  d_current_p = &((*d_current_p) -> getNextRef());
}

template<class Type> void GLListManip<Type>::insert(const Type &data)
{
  new GLLink<Type>(d_current_p, data);
  ++(d_list_p -> d_length);
}

template<class Type> void GLListManip<Type>::remove()
{
  GLLink<Type> *t = *d_current_p;

  *d_current_p = (*d_current_p) -> getNext();
  delete t;
  --(d_list_p -> d_length);
}

template<class Type> GLListManip<Type>::operator const void* () const
{
  return *d_current_p;
}

template<class Type> Type& GLListManip<Type>::operator()() const
{
  return (*d_current_p) -> getData();
}

#endif
