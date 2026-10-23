#ifndef _PSF_DATA
#define _PSF_DATA

#include <stdint.h>
#include <complex>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>

//
// Prototypes
//
class StructDef;
class GroupDef;
class PSFScalar;
class PSFVector;

//
// Type identifiers
//
const int TYPEID_INT8 = 1;
const int TYPEID_STRING = 2;
const int TYPEID_ARRAY = 3;
const int TYPEID_INT32 = 5;
const int TYPEID_DOUBLE = 11;
const int TYPEID_COMPLEXDOUBLE = 12;
const int TYPEID_STRUCT = 16;

// Data types
using PSFInt8 = int8_t;
using PSFInt32 = int32_t;
using PSFDouble = double;
using PSFComplexDouble = std::complex<double>;
using PSFString = std::string;

// 
// Composite data types
//
class Struct: public std::map<std::string, PSFScalar *> {
 private:
    const StructDef *structdef = nullptr;
 public:
    Struct() = default;
    explicit Struct(const StructDef *_structdef) : structdef(_structdef) {}
    Struct(const Struct&);
    ~Struct();

    int datasize() const;

    int deserialize(const char *buf);

    const StructDef *get_structdef() const { return structdef; };

    friend std::ostream &operator<<(std::ostream &stream, const Struct &o);
};

// PSF base class
class PSFBase {
 public:
   virtual ~PSFBase() = default;;
};

//
// Scalar data
//
class PSFScalar : public PSFBase {
 private:
    virtual void print(std::ostream &stream) const = 0;
 public:
    ~PSFScalar() final = default;;
    virtual int deserialize(const char *buf) { return 0; };

    explicit virtual operator int() const { return -1; }
    explicit virtual operator double() const { return 0.0; }
    virtual std::string tostring() const = 0;
    friend std::ostream &operator<<(std::ostream &stream, const PSFScalar &o);
    virtual void *ptr() = 0;
    virtual PSFScalar *clone() const = 0;
};

template<class T> 
class PSFScalarT : public PSFScalar {
 private:
    virtual void print(std::ostream &stream) const { stream << value; }
 public:
    T value;

    PSFScalarT() = default;;
    explicit PSFScalarT(const T& init) : value(init) {};

    explicit operator double() const final;
    explicit operator int() const final;

    std::string tostring() const {
	std::stringstream out;
	out << value;
	return out.str();
    }

    int deserialize(const char *buf);
    void *ptr() { return &value; }
    PSFScalar* clone() const { return std::make_unique<PSFScalarT>(*this); }
};

using PSFDoubleScalar = PSFScalarT<PSFDouble>;
using PSFComplexDoubleScalar = PSFScalarT<PSFComplexDouble>;
using PSFInt8Scalar = PSFScalarT<PSFInt8>;
using PSFInt32Scalar = PSFScalarT<PSFInt32>;
using PSFStringScalar = PSFScalarT<PSFString>;
using StructScalar = PSFScalarT<Struct>;

//
// Vector data
//
class PSFVector : public PSFBase {
 public:
    ~PSFVector() final = default;;
    int type_id;

    virtual std::size_t size() const = 0;
    
    virtual void resize(std::size_t n)=0;
    
    virtual void append_value(void *) {};
    
    virtual void extend(const PSFVector *) {};

    virtual void assign_scalar(int, const PSFScalar &) {};

    virtual void *ptr_at(int i) = 0;
};

template<class T>
class PSFVectorT : public PSFVector, public std::vector<T> {
 private:
   T init;
 public:
   PSFVectorT() = default;;
   explicit PSFVectorT(const T& _init) : init(_init) {};
   void extend(const PSFVector *vec) {
	const auto& tvec = dynamic_cast<const PSFVectorT &>(*vec);
	this->reserve(std::vector<T>::size() + distance(tvec.begin(), tvec.end()));
	this->insert(std::vector<T>::end(), tvec.begin(), tvec.end());
    }
    
    void *ptr_at(int i) { return &std::vector<T>::at(i); }

    void assign_scalar(int i, const PSFScalar &rhs) { 
	(*this)[i] = dynamic_cast<const PSFScalarT<T> &>(rhs).value;
    }

    std::size_t size() const { return std::vector<T>::size(); };
    void resize(std::size_t n) { std::vector<T>::resize(n, init); };

    const T& get_initvalue() const { return init; }
};

typedef PSFVectorT<PSFDouble> PSFDoubleVector;
typedef PSFVectorT<PSFComplexDouble> PSFComplexDoubleVector;
typedef PSFVectorT<PSFInt8> PSFInt8Vector;
typedef PSFVectorT<PSFInt32> PSFInt32Vector;
typedef PSFVectorT<PSFString> PSFStringVector;
typedef PSFVectorT<Struct> StructVector;

int psfdata_size(int type_id);

class VectorStruct: public PSFBase, public std::map<std::string, PSFVector *> {
 private:
    int n = 0;
    void init_from_structdef(const StructDef *structdef);
 public:
    VectorStruct() = default;
    explicit VectorStruct(const StructDef *structdef); 
    explicit VectorStruct(const StructVector &);

    int copy_from_structvector(const StructVector &);
    
    int vectorsize() const { return n; }
};

#endif
