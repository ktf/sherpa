#ifndef METOOLS_Explicit_C_Object_H
#define METOOLS_Explicit_C_Object_H

#include <iostream>
#include <vector>

namespace METOOLS {

  class CObject {
  protected:

    int m_c[2];

    size_t m_h, m_s;

  public:

    // constructor
    inline CObject() {}

    // destructor
    virtual ~CObject();

    // member functions
    virtual CObject* Copy() const = 0;

    virtual void Delete() = 0;

    virtual void Add(const CObject *c) = 0;
    virtual void Divide(const double &d) = 0;
    virtual void Invert() = 0;

    virtual bool IsZero() const = 0;

    // inline functions
    inline int &operator()(const int i) { return m_c[i]; }

    inline int operator()(const int i) const { return m_c[i]; }

    inline void SetH(const size_t &h) { m_h=h; }
    inline void SetS(const size_t &s) { m_s=s; }

    inline const size_t &H() const { return m_h; }
    inline const size_t &S() const { return m_s; }

    inline bool operator==(const CObject &o) const
    { return m_c[0]==o.m_c[0] && m_c[1]==o.m_c[1] && m_s==o.m_s; }

  };// end of class CObject

  struct CObject_Vector: public std::vector<CObject*> {

    template <typename CType> inline std::vector<CType*> *Get()
    { return static_cast<std::vector<CType*>*>((void*)this); }
    template <typename CType> inline const std::vector<CType*> *Get() const
    { return static_cast<const std::vector<CType*>*>((void*)this); }

  };// end of struct CObject_Vector

  struct CObject_Matrix: public std::vector<CObject_Vector> {

    template <typename CType> inline std::vector<std::vector<CType*> > *Get()
    { return static_cast<std::vector<std::vector<CType*> >*>((void*)this); }
    template <typename CType> inline const std::vector<std::vector<CType*> > *Get() const
    { return static_cast<const std::vector<std::vector<CType*> >*>((void*)this); }

  };// end of struct CObject_Matrix

  std::ostream &operator<<(std::ostream &str,const CObject &s);

}// end of namespace METOOLS

#endif
