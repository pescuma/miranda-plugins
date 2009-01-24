#ifndef __PTR_H__
# define __PTR_H__


template<class T> 
class mir_scope 
{
public:
	mir_scope() : p(NULL) {}
	mir_scope(T t) : p(t) {}
	~mir_scope() { release(); }

	void release()
	{
		if (p != NULL)
			mir_free(p);
		p = NULL;
	}

	T operator=(T t) { release(); p = t; return t; }
	T operator->() const { return p; }
	operator T() const { return p; }

	T detach() 
	{ 
		T ret = p;
		p = NULL;
		return ret;
	}

private:
	T p;
};



#endif // __PTR_H__