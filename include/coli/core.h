#ifndef _H_IR_
#define _H_IR_

#include <isl/set.h>
#include <isl/map.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <isl/ast_build.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl/space.h>

#include <map>
#include <string.h>

#include <Halide.h>
#include <coli/debug.h>

namespace coli
{

class library;
class function;
class computation;
class buffer;

extern std::map<std::string, computation *> computations_list;

/**
  * A helper function to split a string.
  */
void split_string(std::string str, std::string delimiter,
		  std::vector<std::string> &vector);

/**
 * Retrieve the access function of the ISL AST leaf node (which represents a
 * computation).  Store the access in computation->access.
 */
isl_ast_node *stmt_code_generator(isl_ast_node *node,
		isl_ast_build *build, void *user);

isl_ast_node *for_code_generator_after_for(isl_ast_node *node,
		isl_ast_build *build, void *user);

/**
  * A class that holds all the global variables necessary for COLi.
  * It also holds COLi options.
  */
class context
{
private:
	static bool auto_data_mapping;

public:
	/**
	  * If this option is set to true, COLi automatically
	  * modifies the computation data mapping whenever a new
	  * schedule is applied to a computation.
	  * If it is set to false, it is up to the user to set
	  * the right data mapping before code generation.
	  */
	static void set_auto_data_mapping(bool v)
	{
		context::auto_data_mapping = v;
	}

	/**
	  * Return whether auto data mapping is set.
	  * If auto data mapping is set, COLi automatically
	  * modifies the computation data mapping whenever a new
	  * schedule is applied to a computation.
	  * If it is set to false, it is up to the user to set
	  * the right data mapping before code generation.
	  */
	static bool get_auto_data_mapping()
	{
		return context::auto_data_mapping;
	}

	static void set_default_coli_options()
	{
		set_auto_data_mapping(true);
	}

	context()
	{
		set_default_coli_options();
	}
};


/**
  * A class to represent a full library.  A library is composed of
  * functions (of type coli::function).  Since a common case for DSL
  * (Domain Specific Language) compilers is to generate library functions,
  * COLi provides an easy way for users to declare a library and the
  * functions of that library and to manipulate the library as a whole
  * instead of manipulating the individual functions.
  */
class library
{
private:

	/**
	  * The name of the library.
	  */
	std::string name;

	/**
	  * The list of functions that are a part of the library.
	  */
	std::vector<coli::function *> functions;

	/**
	  * An isl context associate with the library.
	  * All the functions of the library should have the same
	  * isl context.
	  */
	isl_ctx *ctx;

	/**
	  * An isl AST representation for the library program.
	  * The isl AST is generated by calling gen_isl_ast().
	  */
	isl_ast_node *ast;

	/**
	  * A vector representing the parallel dimensions.
	  * A parallel dimension is identified using the pair
	  * <computation_name, level>, for example the pair
	  * <S0, 0> indicates that the loop with level 0
	  * (i.e. the outermost loop) around the computation S0
	  * should be parallelized.
	  */
	std::map<std::string, int> parallel_dimensions;
	std::map<std::string, int> vector_dimensions;

public:

	/**
	  * Instantiate a library.
	  * \p name is the name of the library.
	  */
	library(std::string name): name(name)
	{
		ast = NULL;

		assert((name.length() > 0) && ("Library name empty"));

		// Allocate an isl context.  This isl context will be used by
		// the isl library calls within coli.
		ctx = isl_ctx_alloc();
	};

	/**
	  * Return a vector representing the functions of the library.
	  */
	std::vector<coli::function *> get_functions()
	{
		return functions;
	}

	/**
	  * Return true if the computation \p comp should be parallelized
	  * at the loop level \p lev.
	  */
	bool parallelize(std::string comp, int lev)
	{
		assert(comp.length() > 0);
		assert(lev >= 0);

		const auto &iter = this->parallel_dimensions.find(comp);
		if (iter == this->parallel_dimensions.end())
		{
	    	return false;
		}
		return (iter->second == lev);
	}

	/**
	  * Return true if the computation \p comp should be vectorized
	  * at the loop level \p lev.
	  */
	bool vectorize(std::string comp, int lev)
	{
		assert(comp.length() > 0);
		assert(lev >= 0);

		const auto &iter = this->vector_dimensions.find(comp);
		if (iter == this->vector_dimensions.end())
		{
	    	return false;
		}
		return (iter->second == lev);
	}

	/**
	  * Tag the dimension \p dim of the computation \p computation_name to
	  * be parallelized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void add_parallel_dimension(std::string computation_name, int vec_dim);

	/**
	  * Tag the dimension \p dim of the computation \p computation_name to
	  * be parallelized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void add_vector_dimension(std::string computation_name, int vec_dim);

	/**
	  * Add a function to the library.
	  */
	void add_function(coli::function *fct);

	isl_union_set *get_iteration_spaces();
	isl_union_map *get_schedule_map();

	/**
	  * Return the isl context associated with this library.
	  */
	isl_ctx *get_ctx()
	{
		return ctx;
	}

	/**
	  * Return the isl ast associated with this library.
	  */
	isl_ast_node *get_isl_ast()
	{
		assert((ast != NULL) && ("Generate the ISL ast using gen_isl_ast()"
					" before calling get_isl_ast()."));

		return ast;
	}

	/**
	  * Return the time-processor representation of all the computations
	  * of the library.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	isl_union_set *get_time_processor_representation();

	/**
	  * Return a vector of Halide statements where each Halide statement
	  * represents a function in the library.
	  */
	std::vector<Halide::Internal::Stmt> get_halide_stmts();


	/**
	  * Generate an object file.  This object file will contain the compiled
	  * functions that are a part of the library.
	  * \p obj_file_name indicates the name of the generated file.
	  * \p os indicates the target operating system.
	  * \p arch indicates the architecture of the target (the instruction set).
	  * \p bits indicate the bit-width of the target machine.
	  *    must be 0 for unknown, or 32 or 64.
	  * For a full list of supported value for \p os and \p arch please
	  * check the documentation of Halide::Target
	  * (http://halide-lang.org/docs/struct_halide_1_1_target.html).
	  * If the machine parameters are not supplied, it will detect one automatically.
	  */
	// @{
	void gen_halide_obj(std::string obj_file_name, Halide::Target::OS os,
			Halide::Target::Arch arch, int bits);

	void gen_halide_obj(std::string obj_file_name) {
		Halide::Target target = Halide::get_host_target();
		gen_halide_obj(obj_file_name, target.os, target.arch, target.bits);
	}
	// @}

	/**
	  * Generate C code on stdout.
	  * Currently C code code generation is very basic and does not
	  * support many features compared to the Halide code generator.
	  */
	void gen_c_code();


	/**
	  * Get an identity relation for the time-processor representation.
	  */
	isl_union_map *get_time_processor_identity_relation();

	/**
	  * Generate an isl AST for the library.
	  */
	void gen_isl_ast()
	{
		// Check that time_processor representation has already been computed,
		// that the time_processor identity relation can be computed without any
		// issue and check that the access was provided.
		assert(this->get_time_processor_representation() != NULL);
		assert(this->get_time_processor_identity_relation() != NULL);

		isl_ctx *ctx = this->get_ctx();
		isl_ast_build *ast_build = isl_ast_build_alloc(ctx);
		isl_options_set_ast_build_atomic_upper_bound(ctx, 1);
		ast_build = isl_ast_build_set_after_each_for(ast_build, &coli::for_code_generator_after_for, NULL);
		ast_build = isl_ast_build_set_at_each_domain(ast_build, &coli::stmt_code_generator, NULL);

		isl_union_map *sched = this->get_time_processor_identity_relation();
		sched = isl_union_map_intersect_domain(sched,
				this->get_time_processor_representation());
		this->ast = isl_ast_build_node_from_schedule_map(ast_build, isl_union_map_copy(sched));

		isl_ast_build_free(ast_build);
	}

	/**
	  * Generate a Halide stmt for each function in the library.
	  */
	void gen_halide_stmt();

	/**
	  * Generate the time-processor representation of the each
	  * function in the library.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */

	void gen_time_processor_IR();

	/**
	  * Set the isl context associated with this class.
	  */
	void set_ctx(isl_ctx *ctx)
	{
		this->ctx = ctx;
	}

	/**
	  * Dump (on stdout) the iteration space representation of the library functions.
	  * This is mainly useful for debugging.
	  */
	void dump_iteration_space_IR();

	/**
	  * Dump (on stdout) the time processor representation of the library functions.
	  * This is mainly useful for debugging.
	  */
	void dump_time_processor_IR();

	/**
	  * Dump (on stdout) the schedule of the library functions.
	  * This is mainly useful for debugging.
	  * The schedule is a relation between the iteration space and a
	  * time space.  The relation provides a logical date of execution for
	  * each point in the iteration space.
	  * The schedule needs first to be set before calling this function.
	  */
	void dump_schedule();

	/**
	  * Dump (on stdout) the library (dump most of the fields of the
	  * library class).
	  * This is mainly useful for debugging.
	  */
	void dump();

	/**
	  * Dump the Halide IR of each function of the library.
	  * gen_halide_stmt should be called before calling this function.
	  */
	void dump_halide_IR();
};


/**
  * A class to represent functions.  A function is composed of
  * computations (of type coli::computation).
  */
class function
{
private:
	// The library where the function is declared.
	// Note that a function can only be a part of one library.
	coli::library *library;

	/**
	  * The name of the function.
	  */
	std::string name;

	/**
	  * Function arguments.  These are the buffers or scalars that are
	  * passed to the function.
	  */
	std::vector<Halide::Argument> arguments;

public:
	/**
	  * Body of the function (a vector of computations).
	  * The order of the computations in the vector do not have any
	  * effect on the actual order of execution of the computations.
	  * The order of execution of computations is specified through the
	  * schedule.
	  */
	std::vector<computation *> body;

	/**
	  * A Halide statement that represents the whole function.
	  * This value stored in halide_stmt is generated by the code generator.
	  */
	Halide::Internal::Stmt *halide_stmt;

	/**
	  * List of buffers of the function.  Some of these buffers are passed
	  * to the function as arguments and some are declared and allocated
	  * within the function itself.
	  */
	std::map<std::string, Halide::Buffer *> buffers_list;

public:

	function(std::string name, coli::library *lib): name(name) {
		assert(lib != NULL);
		assert((name.length()>0) && ("Empty function name"));

		halide_stmt = NULL;
		library = lib;

		lib->add_function(this);
	};

	/**
	  * Return the library where the function is declared.
	  */
	coli::library *get_library()
	{
		return library;
	}

	/**
	  * Get the arguments of the function.
	  */
	std::vector<Halide::Argument> get_arguments()
	{
		return arguments;
	}

	/**
	  * Get the name of the function.
	  */
	std::string get_name()
	{
		return name;
	}

	/**
	  * Return the Halide statement that represents the whole
	  * function.
	  * The Halide statement is generated by the code generator so
	  * before calling this function you should first generate the
	  * Halide statement.
	  */
	Halide::Internal::Stmt get_halide_stmt()
	{
		return *halide_stmt;
	}

	/**
	  * Return the computations of the function.
	  */
	std::vector<computation *> get_computations()
	{
		return body;
	}

	/**
	  * Add a computation to the function.  The order in which
	  * computations are added to the function is not important.
	  * The order of execution is specified using the schedule.
	  */
	void add_computation(computation *cpt);

	/**
	  * Add an argument to the list of arguments of the function.
	  * The order in which the arguments are added is important.
	  * (the first added argument is the first function argument, ...).
	  */
	void add_argument(coli::buffer buf);

	/**
	  * Dump the iteration space representation of the function.
	  */
	void dump_iteration_space_IR();

	/**
	  * Dump the schedule of the computations of the function.
	  */
	void dump_schedule();

	/**
	  * Dump the function on stdard output.  This is useful for debugging.
	  */
	void dump();
};

/**
  * A class that represents computations.
  */
class computation {
	isl_ctx *ctx;

	/**
	  * Time-processor representation if the computation.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	isl_set *time_processor_space;

public:
	/**
	  * Iteration space representation of the computation.
	  * In this representation, the order of execution of computations
	  * is not specified, the computations are also not mapped to memory.
	 */
	isl_set *iter_space;

	/**
	  * Schedule of the computation.
	  */
	isl_map *schedule;

	/**
	  * The function where this computation is declared.
	  */
	coli::function *function;

	/**
	  * The name of this computation.
	  */
	std::string name;

	/**
	  * Halide expression that represents the computation.
	  */
	Halide::Expr expression;

	/**
	  * Halide statement that assigns the computation to a buffer location.
	  */
	Halide::Internal::Stmt stmt;

	/**
	  * Access function.  A map indicating how each computation should be stored
	  * in memory.
	  */
	isl_map *access;

	/**
	  * An isl_ast_expr representing the index of the array where
	  * the computation will be stored.  This index is computed after the scheduling is done.
	  */
	isl_ast_expr *index_expr;

	/**
	  * Create a computation.
	  * \p expr is an expression representing the computation.
	  *
	  * \p iteration_space_str is a string that represents the iteration
	  * space of the computation.  The iteration space should be encoded
	  * using the ISL set format. For details about the ISL format for set
	  * please check the ISL documentation:
	  * http://isl.gforge.inria.fr/user.html#Sets-and-Relations
	  *
	  * The iteration space of a statement is a set that contains
	  * all execution instances of the statement (a statement in a loop
	  * has an execution instance for each loop iteration upon which
	  * it executes). Each execution instance of a statement in a loop
	  * nest is uniquely represented by an identifier and a tuple of
	  * integers  (typically,  the  values  of  the  outer  loop  iterators).
	  * These  integer  tuples  are  compactly  described  by affine constraints.
	  * The iteration space of the statement S0 in the following loop nest
	  * for (i=0; i<N; i++)
	  *   for (j=0; j<M; j++)
	  *      S0;
	  *
	  * is
	  *
	  * {S0[i,j]: 0<=i<N and 0<=j<N}
	  *
	  * THis should be read as: the set of point (i,j) such that
	  * 0<=i<N and 0<=j<N.
	  *
	  * \p fct is a pointer to the coli function where this computation
	  * should be added.
	  */
	computation(Halide::Expr expr,
		    std::string iteration_space_str, coli::function *fct) {

		assert(fct != NULL);
		assert(iteration_space_str.length()>0 && ("Empty iteration space"));

		// Initialize all the fields to NULL (useful for later asserts)
		index_expr = NULL;
		access = NULL;
		schedule = NULL;
		stmt = Halide::Internal::Stmt();
		time_processor_space = NULL;

		assert(fct->get_library() != NULL);

		this->ctx = fct->get_library()->get_ctx();

		iter_space = isl_set_read_from_str(ctx, iteration_space_str.c_str());
		name = std::string(isl_space_get_tuple_name(isl_set_get_space(iter_space), isl_dim_type::isl_dim_set));
		this->expression = expr;
		computations_list.insert(std::pair<std::string, computation *>(name, this));
		function = fct;
		function->add_computation(this);
		this->set_identity_schedule();
	}

	/**
	  * Return the access function of the computation.
	  */
	isl_map *get_access()
	{
		return access;
	}

	/**
	  * Return the function where the computation is declared.
	  */
	coli::function *get_function()
	{
		return function;
	}

	/**
	  * Return the iteration space representation of the computation.
	  * In this representation, the order of execution of computations
	  * is not specified, the computations are also not mapped to memory.
	  */
	isl_set *get_iteration_space_representation()
	{
		// Every computation should have an iteration space.
		assert(iter_space != NULL);

		return iter_space;
	}

	/**
	  * Return the time-processor representation of the computation.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	isl_set *get_time_processor_representation()
	{
		return time_processor_space;
	}

	/**
	  * Get an identity map for the time processor representation.
	  * This is a map that takes one element of the time-processor space
	  * and returns the same element in the time-processor space.
	  */
	isl_map *get_time_processor_identity_relation()
	{
		assert(get_time_processor_representation() != NULL);

		isl_space *sp = isl_set_get_space(get_time_processor_representation());
		isl_map *out = isl_map_identity(isl_space_map_from_set(sp));
		out = isl_map_set_tuple_name(out, isl_dim_out, "");
		return out;
	}

	/**
	  * Return the schedule of the computation.
	  */
	isl_map *get_schedule()
	{
		return this->schedule;
	}

	/**
	  * Return the name of the computation.
	  */
	std::string get_name()
	{
		return name;
	}

	/**
	  * Tag the dimension \p dim of the computation to be parallelized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void tag_parallel_dimension(int dim);

	/**
	  * Tag the dimension \p dim of the computation to be vectorized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void tag_vector_dimension(int dim);

	/**
	  * Generate the time-processor representation of the computation.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	void gen_time_processor_IR()
	{
		assert(this->get_iteration_space_representation() != NULL);
		assert(this->get_schedule() != NULL);

		time_processor_space = isl_set_apply(
				isl_set_copy(this->get_iteration_space_representation()),
				isl_map_copy(this->get_schedule()));
	}


	void SetWriteAccess(std::string access_str)
	{
		assert(access_str.length() > 0);

		this->access = isl_map_read_from_str(this->ctx, access_str.c_str());
	}

	void create_halide_assignement(std::vector<std::string> &iterators);

	/**
	  * Set the identity schedule.
	  */
	void set_identity_schedule()
	{
		std::string domain = isl_set_to_str(this->get_iteration_space_representation());
		std::string schedule_map_str = isl_set_to_str(this->get_iteration_space_representation());
		domain = domain.erase(domain.find("{"), 1);
		domain = domain.erase(domain.find("}"), 1);
		if (schedule_map_str.find(":") != std::string::npos)
			domain = domain.erase(domain.find(":"), domain.length() - domain.find(":"));

		if (schedule_map_str.find(":") != std::string::npos)
			schedule_map_str.insert(schedule_map_str.find(":"), " -> " + domain);
		else
			schedule_map_str.insert(schedule_map_str.find("]")+1, " -> " + domain);

		this->set_schedule(schedule_map_str.c_str());
	}

	/**
	  * Tile the two dimension \p inDim0 and \p inDim1 with rectangular
	  * tiling.  \p sizeX and \p sizeY represent the tile size.
	  * \p inDim0 and \p inDim1 should be two consecutive dimensions,
	  * and the should satisfy \p inDim0 > \p inDim1.
	  */
	void tile(int inDim0, int inDim1, int sizeX, int sizeY);

	/**
	 * Modify the schedule of this computation so that it splits the
	 * dimension inDim0 of the iteration space into two new dimensions.
	 * The size of the inner dimension created is sizeX.
	 */
	void split(int inDim0, int sizeX);

	/**
	 * Modify the schedule of this computation so that the two dimensions
	 * inDim0 and inDime1 are interchanged (swaped).
	 */
	void interchange(int inDim0, int inDim1);

	/**
	  * Set the mapping from iteration space to time-processor space.
	  * The name of the domain and range space must be identical.
	  * The input string must be in the ISL map format.
	  */
	void set_schedule(std::string map_str);

	/**
	  * Set the mapping from iteration space to time-processor space.
	  * The name of the domain and range space must be identical.
	  */
	void set_schedule(isl_map *map)
	{
		// The tuple name for the domain and the range
		// should be identical.
		int diff = strcmp(isl_map_get_tuple_name(map, isl_dim_in),
				  isl_map_get_tuple_name(map, isl_dim_out));
		assert((diff == 0) && "Domain and range space names in the mapping from iteration space to processor-time must be identical.");

		this->schedule = map;
	}


	/**
	  * Dump the iteration space representation of the computation.
	  * This is useful for debugging.
	  */
	void dump_iteration_space_IR();

	/**
	  * Dump the schedule of the computation.
	  * This is mainly useful for debugging.
	  * The schedule is a relation between the iteration space and a
	  * time space.  The relation provides a logical date of execution for
	  * each point in the iteration space.
	  * The schedule needs first to be set before calling this function.
	  */
	void dump_schedule();

	/**
	  * Dump (on stdout) the computation (dump most of the fields of the
	  * computation class).
	  * This is mainly useful for debugging.
	  */
	void dump();
};

/**
  * A class that represents a buffer.  The result of a computation
  * can be stored in a buffer.  A computation can also be a binding
  * to a buffer (i.e. a buffer element is represented as a computation).
  */
class buffer
{
	/**
	  * The name of the buffer.
	  */
	std::string name;

	/**
	  * The number of dimensions of the buffer.
	  */
	int nb_dims;

	/**
	  * The size of buffer dimensions.  Assuming the following
	  * buffer: buf[N0][N1][N2].  The first vector element represents the
	  * leftmost dimension of the buffer (N0), the second vector element
	  * represents N1, ...
	  */
	std::vector<int> dim_sizes;

	/**
	  * The type of the elements of the buffer.
	  */
	Halide::Type type;

	/**
	  * Buffer data.
	  */
	uint8_t *data;

	/**
	  * The coli function where this buffer is declared or where the
	  * buffer is an argument.
	  */
	coli::function *fct;

public:
	/**
	  * Create a coli buffer where computations can be stored
	  * or buffers bound to computation.
	  * \p name is the name of the buffer.
	  * \p nb_dims is the number of dimensions of the buffer.
	  * A scalar is a one dimensional buffer with one element.
	  * \p dim_sizes is a vector of integers that represent the size
	  * of each dimension in the buffer.
	  * \p type is the type of the buffer.
	  * \p data is the data stored in the buffer.  This is useful
	  * for binding a computation to an already existing buffer.
	  * \p fct is i a pointer to a coli function where the buffer is
	  * declared/used.
	  */
	buffer(std::string name, int nb_dims, std::vector<int> dim_sizes,
			Halide::Type type, uint8_t *data, coli::function *fct):
		name(name), nb_dims(nb_dims), dim_sizes(dim_sizes), type(type),
		data(data), fct(fct)
		{
			assert(name.length()>0 && "Empty buffer name");
			assert(nb_dims>0 && "Buffer dimensions <= 0");
			assert(nb_dims == dim_sizes.size() && "Mismatch in the number of dimensions");
			assert(fct != NULL && "Input function is NULL");

			Halide::Buffer *buf = new Halide::Buffer(type, dim_sizes, data, name);
			fct->buffers_list.insert(std::pair<std::string, Halide::Buffer *>(buf->name(), buf));
		};

	/**
	  * Return the name of the buffer.
	  */
	std::string get_name()
	{
		return name;
	}

	Halide::Type get_type()
	{
		return type;
	}

	/**
	  * Get the number of dimensions of the buffer.
	  */
	int get_n_dims()
	{
		return nb_dims;
	}
};

namespace parser
{

/**
  * A class to hold parsed tokens of an isl_space.
  * This class is useful in parsing isl_space objects.
  */
class space
{
public:
	std::vector<std::string> dimensions;

	/**
	  * Parse an isl_space.
	  * The isl_space is a string in the format of ISL.
	  */
	space(std::string isl_space_str)
	{
		assert(isl_space_str.empty() == false);
		this->parse(isl_space_str);
	};

	space() {};

	/**
	  * Return a string that represents the parsed string.
	  */
	std::string get_str()
	{
		std::string result;

		for (int i=0; i<dimensions.size(); i++)
		{
			if (i != 0)
				result = result + ",";
			result = result + dimensions.at(i);
		}

		return result;
	};

	void replace(std::string in, std::string out1, std::string out2)
	{
		std::vector<std::string> new_dimensions;

		for (auto dim:dimensions)
		{
			if (dim.compare(in) == 0)
			{
				new_dimensions.push_back(out1);
				new_dimensions.push_back(out2);
			}
			else
				new_dimensions.push_back(dim);
		}

		dimensions = new_dimensions;
	}

	void parse(std::string space);
	bool empty() {return dimensions.empty();};
};


/**
 * A class to hold parsed tokens of isl_constraints.
 */
class constraint
{
public:
	std::vector<std::string> constraints;
	constraint() { };

	void parse(std::string str)
	{
		assert(str.empty() == false);

		split_string(str, "and", this->constraints);
	};

	void add(std::string str)
	{
		assert(str.empty() == false);
		constraints.push_back(str);
	}

	std::string get_str()
	{
		std::string result;

		for (int i=0; i<constraints.size(); i++)
		{
			if (i != 0)
				result = result + " and ";
			result = result + constraints.at(i);
		}

		return result;
	};

	bool empty() {return constraints.empty();};
};


/**
  * A class to hold parsed tokens of isl_maps.
  */
class map
{
public:
	coli::parser::space parameters;
	std::string domain_name;
	std::string range_name;
	coli::parser::space domain;
	coli::parser::space range;
	coli::parser::constraint constraints;

	map(std::string map_str)
	{
		int map_begin =  map_str.find("{")+1;
		int map_end   =  map_str.find("}")-1;

		assert(map_begin != std::string::npos);
		assert(map_end != std::string::npos);

		int domain_space_begin = map_str.find("[", map_begin)+1;
		int domain_space_begin_pre_bracket = map_str.find("[", map_begin)-1;
		int domain_space_end   = map_str.find("]", map_begin)-1;

		assert(domain_space_begin != std::string::npos);
		assert(domain_space_end != std::string::npos);

		domain_name = map_str.substr(map_begin,
		 		             domain_space_begin_pre_bracket-map_begin+1);

		std::string domain_space_str =
			map_str.substr(domain_space_begin,
		 		       domain_space_end-domain_space_begin+1);

		domain.parse(domain_space_str);

		int pos_arrow = map_str.find("->", domain_space_end);
		int first_char_after_arrow = pos_arrow + 2;

		assert(pos_arrow != std::string::npos);

		int range_space_begin = map_str.find("[", first_char_after_arrow)+1;
		int range_space_begin_pre_bracket = map_str.find("[", first_char_after_arrow)-1;
		int range_space_end = map_str.find("]", first_char_after_arrow)-1;

		assert(range_space_begin != std::string::npos);
		assert(range_space_end != std::string::npos);

		range_name = map_str.substr(first_char_after_arrow,
		 		             range_space_begin_pre_bracket-first_char_after_arrow+1);
		std::string range_space_str = map_str.substr(range_space_begin,
							 range_space_end-range_space_begin+1);
		range.parse(range_space_str);
		int column_pos = map_str.find(":")+1;

		if (column_pos != std::string::npos)
		{
			std::string constraints_str = map_str.substr(column_pos,
								     map_end-column_pos+1);
			constraints.parse(constraints_str);
		}

		if (DEBUG2)
		{
			coli::str_dump("Parsing the map : " + map_str + "\n");
			coli::str_dump("The parsed map  : " + this->get_str() + "\n");
		}
	};

	std::string get_str()
	{
		std::string result;

		result = "{" + domain_name + "[" + domain.get_str() + "] ->" +
			  range_name + "[" +
			  range.get_str() + "]";

		if (constraints.empty() == false)
			result = result + " :" + constraints.get_str();

		result = result + " }";

		return result;
	};

	isl_map *get_isl_map(isl_ctx *ctx)
	{
		return isl_map_read_from_str(ctx, this->get_str().c_str());
	};
};

}

// Halide IR specific functions

void halide_IR_dump(Halide::Internal::Stmt s);

/**
  * Generate a Halide statement from an ISL ast node object in the ISL ast
  * tree.
  * Level represents the level of the node in the schedule.  0 means root.
  */
Halide::Internal::Stmt *generate_Halide_stmt_from_isl_node(coli::library lib, isl_ast_node *node,
		int level, std::vector<std::string> &generated_stmts,
		std::vector<std::string> &iterators);

}

/** Transform the iteration space into time-processor space representation.
 *  This is done by applying the schedule to the iteration space.
 */
isl_union_set *create_time_space_representation(
		__isl_take isl_union_set *set,
		__isl_take isl_union_map *umap);

isl_schedule *create_schedule_tree(isl_ctx *ctx,
		   isl_union_set *udom,
		   isl_union_map *sched_map);

isl_ast_node *generate_isl_ast_node(isl_ctx *ctx,
		   isl_schedule *sched_tree);

#endif
