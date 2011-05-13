#ifndef __lk_var_h
#define __lk_var_h

#include <string>
#include <vector>
#include <cstdio>
#include <cstdarg>
#include <exception>

#ifdef _MSC_VER
#include <unordered_map>
using std::tr1::unordered_map;
#pragma warning(disable: 4290)  // ignore warning: 'C++ exception specification ignored except to indicate a function is not __declspec(nothrow)'
#else
#include <tr1/unordered_map>
using std::tr1::unordered_map;
#endif

#include "lk_absyn.h"


#define LK_DOC(  fn, desc, sig ) if (cxt.doc_mode()) { cxt.document( lk::doc_t(fn , "", desc, sig ) ); return; }
#define LK_DOC1( fn, notes, desc1, sig1 ) if (cxt.doc_mode()) { cxt.document( lk::doc_t(fn , notes, desc1, sig1 )); return; }
#define LK_DOC2( fn, notes, desc1, sig1, desc2, sig2 ) if (cxt.doc_mode()) { cxt.document( lk::doc_t(fn , notes, desc1, sig1, desc2, sig2 )); return; }
#define LK_DOC3( fn, notes, desc1, sig1, desc2, sig2, desc3, sig3 ) if (cxt.doc_mode()) { cxt.document( lk::doc_t(fn , notes, desc1, sig1, desc2, sig2, desc3, sig3 )); return; }


namespace lk {

	class vardata_t;
	typedef unordered_map< std::string, vardata_t* > varhash_t;
		
	
	class error_t : public std::exception
	{
	public:
		error_t() : text("general data exception") {  }
		error_t(const char *fmt, ...) {
			char buf[512];
			va_list args;
			va_start( args, fmt );
			vsprintf( buf, fmt, args );
			va_end( args );
			text = buf;
		}
		error_t(const std::string &t) : text(t) {  }
		virtual ~error_t() throw() {  }
		std::string text;
		virtual const char *what() const throw (){ return text.c_str(); }
	};

	class vardata_t
	{
	private:
		unsigned char m_type;
		union {
			void *p;
			double v;
		} m_u;

	public:
		enum {
			NULLVAL,
			REFERENCE,
			NUMBER,
			STRING,
			VECTOR,
			HASH,
			FUNCTION
		};

		vardata_t();
		vardata_t( const vardata_t &cp );
		~vardata_t();

		static const double nanval;

		inline unsigned char type() { return m_type; }
		std::string typestr();

		void nullify();
		bool as_boolean();
		unsigned int as_unsigned();
		int as_integer();
		std::string as_string();
		double as_number();

		bool copy( vardata_t &rhs );
		bool equals( vardata_t &rhs );
		bool lessthan( vardata_t &rhs );

		
		vardata_t &operator=(const vardata_t &rhs)
		{
			copy( const_cast<vardata_t&>(rhs) );
			return *this;
		}

		vardata_t &deref() throw (error_t);
		
		void assign( double d );
		void assign( const char *s );
		void assign( const std::string &s );
		void empty_vector();
		void empty_hash();
		void assign( const std::string &key, vardata_t *val );
		void unassign( const std::string &key );
		void assign( expr_t *func ); // does NOT take ownership (expr_t must be deleted by the environment
		void assign( vardata_t *ref ); // makes this vardata_t a reference to the object 'ref'
		void resize( size_t n );

		vardata_t *ref();
		double &num() throw(error_t);
		std::string &str() throw(error_t);
		const char *cstr() throw(error_t);
		expr_t *func() throw(error_t);
		vardata_t *index(size_t idx) throw(error_t);
		size_t length();
		vardata_t *lookup( const std::string &key ) throw(error_t);

		std::vector<vardata_t> *vec() throw(error_t);
		void vec_append( double d ) throw(error_t);
		void vec_append( const std::string &s ) throw(error_t);

		varhash_t *hash() throw(error_t);
		void hash_item( const std::string &key, double d ) throw(error_t);
		void hash_item( const std::string &key, const std::string &s ) throw(error_t);
		void hash_item( const std::string &key, const vardata_t &v ) throw(error_t);

	};
	
	class objref_t
	{
	public:
		virtual ~objref_t() {  }
		virtual std::string type_name() = 0;
	};
	
	class env_t;
	class invoke_t;

	typedef void (*fcall_t)( lk::invoke_t& );

	struct fcallinfo_t {
		fcall_t f;
		void *user_data;
	};

	typedef unordered_map< std::string, fcallinfo_t > funchash_t;

	class doc_t
	{
	friend class invoke_t;
	public:
		doc_t() : has_2(false), has_3(false), m_ok(false) {  }
		doc_t( const char *f, const char *n,
						 const char *d1, const char *s1 )
							 : func_name(f), notes(n),
							 desc1(d1), sig1(s1),
							 desc2(""), sig2(""),
							 desc3(""), sig3(""),
							 has_2(false), has_3(false), m_ok(false) { }

		doc_t( const char *f, const char *n,
						 const char *d1, const char *s1,
						 const char *d2, const char *s2 )
							 : func_name(f), notes(n),
							 desc1(d1), sig1(s1),
							 desc2(d2), sig2(s2),
							 desc3(""), sig3(""),
							 has_2(true), has_3(false), m_ok(false) { }

		doc_t( const char *f, const char *n,
						 const char *d1, const char *s1,
						 const char *d2, const char *s2,
						 const char *d3, const char *s3 )
							 : func_name(f), notes(n),
							 desc1(d1), sig1(s1),
							 desc2(d2), sig2(s2),
							 desc3(d3), sig3(s3),
							 has_2(true), has_3(true), m_ok(false) { }

		std::string func_name;
		std::string notes;
		std::string desc1, sig1;
		std::string desc2, sig2;
		std::string desc3, sig3;
		bool has_2, has_3;

		bool ok() { return m_ok; }

		static bool info( fcall_t f, doc_t &d );
	private:
		void copy_data( doc_t *p );
		bool m_ok;
	};

	class invoke_t
	{
		friend class doc_t;
	private:
		doc_t *m_docPtr;
		std::string m_funcName;
		env_t *m_env;
		vardata_t &m_resultVal;
		std::vector< vardata_t > m_argList;

		std::string m_error;
		bool m_hasError;

		void *m_userData;
		
	public:
		invoke_t( const std::string &n, env_t *e, vardata_t &result, void *user_data = 0)
			: m_docPtr(0), m_funcName(n), m_env(e), m_resultVal(result), m_hasError(false), m_userData(user_data) { }

		bool doc_mode();
		void document( doc_t d );
		env_t *env() { return m_env; }
		std::string name() { return m_funcName; }
		vardata_t &result() { return m_resultVal; }
		void *user_data() { return m_userData; }

		std::vector< vardata_t > &arg_list() { return m_argList; }
		size_t arg_count() { return m_argList.size(); }
		vardata_t &arg(size_t idx) throw( error_t ) {
			if (idx < m_argList.size())	return m_argList[idx].deref();
			else throw error_t( "invalid access to function argument %d, only %d given", idx, m_argList.size());
		}

		void error( const std::string &text ) { m_error = text; m_hasError = true; }
		std::string error() { return m_error; }
		bool has_error() { return m_hasError; }
	};
	
	class env_t
	{
	private:
		env_t *m_parent;

		varhash_t m_varHash;
		varhash_t::iterator m_varIter;

		funchash_t m_funcHash;
		std::vector< objref_t* > m_objTable;
	public:
		env_t();
		env_t(env_t *p);
		~env_t();

		void clear_vars();
		void clear_objs();

		void assign( const std::string &name, vardata_t *value );
		void unassign( const std::string &name );
		vardata_t *lookup( const std::string &name, bool search_hierarchy);
		const char *first();
		const char *next();
		unsigned int size();
		env_t *parent();
		env_t *global();
		
		bool register_func( fcall_t f, void *user_data = 0 );
		bool register_funcs( std::vector<fcall_t> l );
		fcall_t lookup_func( const std::string &name, void **user_data = 0 );
		std::vector<std::string> list_funcs();

		size_t insert_object( objref_t *o );
		bool destroy_object( objref_t *o );
		objref_t *query_object( size_t ref );

		void call( const std::string &name,
				   std::vector< vardata_t > &args,
				   vardata_t &result ) throw( error_t );
		
	};

}; // namespace lk

#endif
