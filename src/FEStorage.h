// This file is a part of nla3d project. For information about authors and
// licensing go to project's repository on github:
// https://github.com/dmitryikh/nla3d 

#pragma once
#include <list>
#include "sys.h"
#include "materials/MaterialFactory.h"
#include "elements/ElementFactory.h"
#include "math/SparseMatrix.h"
#include "FEComponent.h"
#include "PostProcessor.h"
 
namespace nla3d {

//pre-defines
class Element;
class Node;
class PostProcessor;
class Dof;

class BC {
public:
	BC () : is_updatetable(false)
	{  }
	//virtual void apply(FEStorage *storage)=0;
	bool is_updatetable; //нужно ли обновлять на каждом шаге решения
};
class BC_dof_constraint : public BC
{
public:
	BC_dof_constraint () : node(0), node_dof(0), value(0.0)
	{	}
	int32 node;
	uint16 node_dof;
	double value;
	//void apply(FEStorage *storage);
};

class BC_dof_force : public BC
{
public:
	BC_dof_force () : node(0), node_dof(0), value(0.0)
	{	}
	int32 node;
	uint16 node_dof;
	double value;
	//void apply(FEStorage *storage);
};
class MPC_token
{
public:
	MPC_token() : node(0), node_dof(0), coef(0.0)
	{	}
	MPC_token(int32 n, uint16 dof, double coef) : node(n), node_dof(dof), coef(coef)
	{	}
	int32 node;
	uint16 node_dof;
	double coef;
};
// MPC equation like
// coef1 * dof1 + coef2 * dof2 + ... + coefn * dofn - b = 0
class BC_MPC : public BC {
public:
	std::list<MPC_token> eq;
	double b;
	// void apply(FEStorage *storage);
  // virtual void update(const double time) { };
};

class BC_MPC_FINITE_ROTATE : public BC_MPC {

  //unit axis of ratational
  math::Vec<3> n;
  //zero point of rotational
  math::Vec<3> o;
  // speed of rotation
  double theta;
};

#define ST_INIT 1
#define ST_LOADED 2
#define ST_SOLVED 3


// class FEStorage - heart of the NLA programm, it contains all data to keep in memory: 
//				nodes, elements, solution staff (global matrixes and vectors), processors.
//TODO: add mutexes to prevent multithreads errors

class FEStorage {
public:
	std::vector<Element*> elements;
	std::vector<Node> nodes;
	std::list<BC_dof_force> list_bc_dof_force;
	std::list<BC_dof_constraint> list_bc_dof_constraint;
	std::list<BC_MPC> list_bc_MPC;
	std::vector<PostProcessor*> post_procs;
  std::vector<FEComponent*> feComponents;

	uint32 n_nodes;
	uint32 n_elements;
	uint32 n_dofs; //общее число степеней свободы системы

	uint32 n_constrained_dofs;
	uint32 n_solve_dofs;
	uint32 n_MPC_eqs;

	Material* material;
  nla3d::math::SparseSymmetricMatrix* KssCsT; //solve matrix [Kss,CsT;Cs,0]
  nla3d::math::SparseMatrix *Cc; // MPC eqs matrix for constrained dofs
  nla3d::math::SparseMatrix *Kcs; //part of global K matrix dim[n_constrained_dofs x n_solve_dofs]
  nla3d::math::SparseSymmetricMatrix *Kcc; //part of global K matrix dim[n_constrained_dofs x n_constrained_dofs]

	double* vec_q_lambda;
	double* vec_q;
	double* vec_qc;
	double* vec_qs;
	double* vec_lambda;

	double* vec_dq_dlambda;
	double* vec_dq;
	double* vec_dqc;
	double* vec_dqs;
	double* vec_dlambda;

	double* vec_reactions;

	double* vec_F;
	double* vec_Fc;
	double* vec_Fs;

	double* vec_b;

	double* vec_rhs;
	double* vec_rhs_dof;
	double* vec_rhs_mpc;

	Dof *dof_array; //массив степеней свободы, поизводит привязку номера степени свободы и номера уравнения
	//status
	uint16 status;

  ElementFactory::elTypes elType;

	//methods
	FEStorage();
	~FEStorage();

	uint32 get_dof_num(int32 node, uint16 dof);
	uint32 get_dof_eq_num(int32 node, uint16 dof);
	bool is_dof_constrained(int32 node, uint16 dof);


	void Kij_add(int32 nodei, uint16 dofi, int32 nodej, uint16 dofj, double value);
	void Cij_add(uint32 eq_num, int32 nodej, uint32 dofj, double value);
	void Fi_add(int32 nodei, uint16 dofi, double value);
	void zeroK();
	void zeroF();
  nla3d::math::SparseSymmetricMatrix& get_solve_mat();
	double* get_solve_rhs ();
	double* get_solve_result_vector();
	double* get_vec_dqs();
	Material& getMaterial();
	Node& getNode(uint32 _nn);
	Element& getElement(uint32 _en);
	PostProcessor& getPostProc(size_t _np);

	uint32 getNumDofs () {
		return n_dofs; 
	}
	uint32 getNumNode () {
		return n_nodes;
	}
	uint32 getNumElement () {
		return n_elements;
	}
	size_t getNumPostProc () {
		return post_procs.size();
	}
	uint16 getStatus() {
		return status;
	}
	void setStatus(uint16 st) {
		status = st;
	}
	uint32 get_n_solve_dofs() {
		return n_solve_dofs;
	}
	uint32 get_n_constrained_dofs() {
		return n_constrained_dofs;
	}
	uint32 gen_n_MPC_eqs() {
		return n_MPC_eqs;
	}

	std::list<BC_dof_constraint>& get_dof_const_BC_list();

	uint16 add_post_proc (PostProcessor *pp);
	void add_bounds (BC_dof_constraint &bc) ;
	void add_bounds (BC_dof_force &bc) ;
	void add_bounds (BC_MPC &bc);
  void addFEComponent (FEComponent* comp);
  void listFEComponents (std::ostream& out);
  FEComponent* getFEComponent(size_t i);
  FEComponent* getFEComponent(std::string name);

	void clearMesh ();
  void deleteElements();
	void nodes_reassign(uint32 nn); 
	void elements_reassign(uint32 en);

	bool prepare_for_solution ();

	void element_nodes(uint32 el, Node** node_ptr);
	void get_q_e(uint32 el, double* ptr);
	void get_dq_e(uint32 el, double* ptr);
	void get_q_n(uint32 n, double* ptr);
	double get_qi_n(int32 n, uint16 dof);
	void get_node_pos(uint32 n, double* ptr, bool def = false); //def. or initial pos

	double get_reaction_force(int32 n, uint16 dof);

	void pre_first();
	void post_first();
	void process_solution();
	void apply_BCs (uint16 curLoadstep, uint16 curIteration, double d_par, double cum_par);
private:
};



inline double* FEStorage::get_solve_result_vector ()
{
	assert(vec_dq_dlambda);
	return vec_dqs;
}


inline Material& FEStorage::getMaterial()
{
	assert(material);
	return *material;
}

//getNode(nn)
//нумерация с 1
inline Node& FEStorage::getNode(uint32 _nn)
{
	assert(_nn <= n_nodes);
	return nodes[_nn-1];
}

//getElement(_en)
//нумерация с 1
inline Element& FEStorage::getElement(uint32 _en)
{
	assert(_en <= n_elements);
	return *(elements[_en-1]);
}

//getPostProc(_np)
//нумерация с 0
inline PostProcessor& FEStorage::getPostProc(size_t _np)
{
	assert(_np < getNumPostProc());
	return *post_procs[_np];
}


inline std::list<BC_dof_constraint>& FEStorage::get_dof_const_BC_list()
{
	return list_bc_dof_constraint;
}


//add_Bounds
inline void FEStorage::add_bounds (BC_dof_constraint &bc)
{
	//TODO: предполагается, что закрепления не повторяют одну и туже степень свободы!
	list_bc_dof_constraint.push_back(bc);
	n_constrained_dofs++;
}


inline void FEStorage::add_bounds (BC_dof_force &bc)
{
	//TODO: предполагается, что закрепления не повторяют одну и туже степень свободы!
	list_bc_dof_force.push_back(bc);
}


inline void FEStorage::add_bounds (BC_MPC &bc)
{
	//TODO: предполагается, что закрепления не повторяют одну и туже степень свободы!
	list_bc_MPC.push_back(bc);
}



//get_qi_n(n, dof)
// n начинается с 1
inline double FEStorage::get_qi_n(int32 n, uint16 dof)
{
	assert(vec_q_lambda);
	return vec_q[get_dof_eq_num(n, dof)-1];
}


inline double* FEStorage::get_vec_dqs()
{
	assert(vec_dq_dlambda);
	return vec_dqs;
}

bool read_ans_data (const char *filename, FEStorage *storage);

} // namespace nla3d 