#ifndef clone_ptr_H_HEADER_GUARD_
#define clone_ptr_H_HEADER_GUARD_

/*
// clone_ptr class by David Maisonave (Axter)
// Copyright (C) 2005
// David Maisonave (Axter) (609-345-1007) (www.axter.com)
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  David Maisonave (Axter) makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
Description:
	The clone_ptr class is a smart pointer class that can be used
	with an STL container to create a container of smart pointers.
	The main purpose of the clone_ptr, is to make it easier to create
	a container of abstract based objects.
	The clone_ptr does not share it's pointer, nor does it move it's pointer,
	and moreover, it's based on the idea of strict pointer ownership logic.
	The main difference between clone_ptr and other similar smart pointers
	is that the clone_ptr has a clone interface, which is used to create
	a copy of the derived object.  The clone interface is used in
	the clone_ptr copy constructor and in the assignment operator.

	The clone_ptr can also be used with sorted containers (std::map, std::set).
	When used with sorted containers, the base class must have an operator<() function. (See example code.)

	****** For more detailed description and example usage see following links: *******
	Example Program:
	http://code.axter.com/clone_ptr_demo.zip
	Detail description:
	http://axter.com/lib/

	See example program for example usage.

*/
#include <memory>

template<typename T>
class clone_ptr
{
private:
	class func_ptr_interface
	{
	public:
		typedef T* TP;
		virtual T* construct() = 0;
		virtual void destroy() = 0;
		virtual ~func_ptr_interface(){}
		virtual func_ptr_interface* clone_func_ptr_interface(T* Obj)=0;
	};
	template<typename T_obj, class AX_TYPE = std::allocator<T_obj> >
		class func_ptr_interface_default : public func_ptr_interface
	{
	public:
		func_ptr_interface_default(T_obj* Obj): m_Obj(Obj){}
		T* construct()
		{
			AX_TYPE alloc;
			T_obj* tmp_ptr = alloc.allocate(1, NULL);
			alloc.construct(tmp_ptr, *m_Obj);
			return tmp_ptr;
		}
		func_ptr_interface* clone_func_ptr_interface(T* Obj)
		{
			return new func_ptr_interface_default((T_obj*)Obj);
		}
		void destroy()
		{
			AX_TYPE alloc;
			alloc.destroy(m_Obj);
			alloc.deallocate(m_Obj, 1);
		}
	private:
		T_obj* m_Obj;
	};
	public:
	typedef T* pointer;
	typedef T& reference;
	//Constructor for types having clone() named method
	template<typename T_obj>
		clone_ptr(T_obj* type):m_type(type), m_func_ptr_interface(new func_ptr_interface_default<T_obj, std::allocator<T_obj> >(type)){}
	//Constructor for types having clone() named method
	template<typename T_obj, class AX_TYPE>
		clone_ptr(T_obj* type, const AX_TYPE&):m_type(type), m_func_ptr_interface(new func_ptr_interface_default<T_obj, AX_TYPE>(type)){}
	//Destructor
	~clone_ptr(){m_func_ptr_interface->destroy();m_type=NULL;delete m_func_ptr_interface;}
	//Copy constructor
	clone_ptr(const clone_ptr& Src):m_type(Src.m_func_ptr_interface->construct()), m_func_ptr_interface(Src.m_func_ptr_interface->clone_func_ptr_interface(m_type)){}
	//Assignment operator
	clone_ptr& operator=(const clone_ptr& Src)
	{
		if (&Src != this)
		{
			m_func_ptr_interface->destroy();
			delete m_func_ptr_interface;
			m_type = Src.m_func_ptr_interface->construct();
			m_func_ptr_interface = Src.m_func_ptr_interface->clone_func_ptr_interface(m_type);
		}
		return *this;
	}
#if !defined(_MSC_VER) || (_MSC_VER > 1200)
	template<typename CompatibleT>
		clone_ptr& operator=(const clone_ptr<CompatibleT>& Src)
	{
		if (Src.get_ptr() != m_type)
		{
			m_func_ptr_interface->destroy();
			delete m_func_ptr_interface;
			CompatibleT* Tmp_ptr = Src.m_func_ptr_interface->construct();
			m_type = Tmp_ptr;
			m_func_ptr_interface = new func_ptr_interface_default<CompatibleT>(Tmp_ptr);
		}
		return *this;
	}
#endif //_MSC_VER != 1200
	bool operator! () const
	{
		return m_type == 0;
	}
	template<typename T2>
		clone_ptr& equal(const T2& Src){
		(*m_type) = (Src);
		return *this;
	}
	T* operator->() const{return m_type;}
	T& operator*() const{return *m_type;}
	//Consider changing the follow operators to template methods
	clone_ptr& operator+=(const clone_ptr& Src){
		m_type->operator+=(*Src.m_type);
		return *this;
	}
	template<typename T2>
		clone_ptr& operator+=(const T2& Src){
		m_type->operator+=(Src);
		return *this;
	}
	clone_ptr& operator+(const clone_ptr& Src){
		m_type->operator+(*Src.m_type);
		return *this;
	}
	clone_ptr& operator-=(const clone_ptr& Src){
		m_type->operator-=(*Src.m_type);
		return *this;
	}
	clone_ptr& operator-(const clone_ptr& Src){
		m_type->operator-(*Src.m_type);
		return *this;
	}
	T* get_ptr()const{return m_type;}
	//Other Misc methods
    void swap(clone_ptr<T> & other){std::swap(m_type, other.m_type);std::swap(m_func_ptr_interface, other.m_func_ptr_interface);}
private:
	T* m_type;
public:
	func_ptr_interface *m_func_ptr_interface;
};

template<class T, class U> inline bool operator<(clone_ptr<T> const & a, clone_ptr<U> const & b){return (*a.get_ptr()) < (*b.get_ptr());}
template<class T, class U> inline bool operator>(clone_ptr<T> const & a, clone_ptr<U> const & b){return (*a.get_ptr()) > (*b.get_ptr());}
template<class T, class U> inline bool operator<=(clone_ptr<T> const & a, clone_ptr<U> const & b){return (*a.get_ptr()) <= (*b.get_ptr());}
template<class T, class U> inline bool operator>=(clone_ptr<T> const & a, clone_ptr<U> const & b){return (*a.get_ptr()) >= (*b.get_ptr());}
template<class T, class U> inline bool operator==(clone_ptr<T> const & a, clone_ptr<U> const & b){return (*a.get_ptr()) == (*b.get_ptr());}
template<class T, class U> inline bool operator!=(clone_ptr<T> const & a, clone_ptr<U> const & b){return (*a.get_ptr()) != (*b.get_ptr());}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96
// Resolve the ambiguity between our op!= and the one in rel_ops
template<class T> inline bool operator!=(clone_ptr<T> const & a, clone_ptr<T> const & b){return (*a.get_ptr()) != (*b.get_ptr());}
#endif

#endif //!clone_ptr_H_HEADER_GUARD_

