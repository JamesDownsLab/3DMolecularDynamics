//
// Created by ppxjd3 on 14/07/2021.
//

#ifndef INC_3DMOLECULARDYNAMICS_ENGINE_H
#define INC_3DMOLECULARDYNAMICS_ENGINE_H

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <vector>
#include "Particle.h"
#include "BasePlate.h"
#include <random>
#include <chrono>
#include "Options.h"
#include "nanoflann.h"
#include "KDTreeVectorOfVectorsAdaptor.h"
#include <Eigen/Dense>
#include <set>

typedef  std::vector<std::vector<double>> my_vector_of_vectors_t;
typedef KDTreeVectorOfVectorsAdaptor<my_vector_of_vectors_t, double> my_kd_tree_t;

namespace fs = std::filesystem;

const double SQRT3 = sqrt(3);

class Engine {

public:
    /**
     * Initialises the Engine class
     *
     * \param fname Filename of initialisation data
     * \param options Struct containing various options for the program
     */
     explicit Engine(Options& options);

     ///Iterates the simulation by one timestep
     void step();

     void set_baseplate(double A, double T){basePlate.set_A(A); basePlate.set_T(T);}

private:
    /// Setup the system

    void init_system();

    void add_particles();

    void add_base_particles();

    void create_dimples();

    /// Adjust lx and ly to match an integer number of dimples
    void adjust_box_dimensions();

    /// Calculate the collisional forces between all particles
    void make_forces();

    /// Calculate the forces with the base
    void make_plate_forces();

    /// Calculate forces and updates positions/velocities
    void integrate();

    /////////////////////////////////////////////////////////////////////////////
    /// Lattice algorithm for balls
    /////////////////////////////////////////////////////////////////////////////

    /// 2D vector representing the lattice cells
    /// Value contains -1 if empty or the index of the particle.
    std::vector<std::vector<int>> pindex;

    /// Vector containing a list of neighbours for each particle.
    std::vector<std::vector<int>> partners;


    /// Updates pindex and partners.
    void make_ilist();

    /// Checks if pindex will change.
    bool ilist_needs_update();
    void clear_pindex();
    void init_lattice_algorithm();

    double rmin{0}, rmax{0}, gk{0};
    int gm{0}, Nx{0}, Ny{0};
    /////////////////////////////////////////////////////////////////////////////
    /// Lattice algorithm for base
    /////////////////////////////////////////////////////////////////////////////

    /// 2D vector representing the lattice cells for base particles
    /// Value contains -1 if empty or the index of the particle
    std::vector<std::vector<int>> pindex_base;

    void init_lattice_algorithm_for_base_particles();
    void clear_pindex_base();


    double r_base{0}, gk_base{0};
    int gm_base{0}, nx_base{0}, ny_base{0};

    ///////////////////////////////////////////////////////////
    /// File saving
    //////////////////////////////////////////////////////////

    void dump(bool first);
    void dump_preamble(std::FILE* f, bool inc_particles, bool inc_base_particles) const;
    void dump_csv_header(std::FILE* f) const;
    void dump_particles(std::FILE* f);
    void dump_particle_to_csv(std::FILE* f);
    void dump_base(std::FILE * f);
    void check_dump();
    int save{1};
    int save_csv{1};
    std::FILE* f1;
    std::FILE* f2;
    std::FILE* f3;

    ///////////////////////////////////////////////////////////
    /// Particle data
    //////////////////////////////////////////////////////////

    std::vector<Particle> particles;
    size_t no_of_particles{0};
    std::vector<Particle> base_particles;
    size_t no_of_base_particles{0};



    ///////////////////////////////////////////////////////////
    /// System
    //////////////////////////////////////////////////////////

    Options _options;

    double timestep{0};

    double Time{0};
    unsigned int step_number{0};
    Eigen::Vector3d G{0, 0, -9.81};
    BasePlate basePlate{0, 0, 0};
    double lx;
    double ly;
    double lz;

    std::chrono::steady_clock::time_point begin;

};


#endif //INC_3DMOLECULARDYNAMICS_ENGINE_H
