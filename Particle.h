//
// Created by ppxjd3 on 14/07/2021.
//

#ifndef INC_3DMOLECULARDYNAMICS_PARTICLE_H
#define INC_3DMOLECULARDYNAMICS_PARTICLE_H

#include <iostream>
#include "BasePlate.h"
#include "Options.h"
#include <Eigen/Dense>
#include <map>
#include <set>

const Eigen::Vector3d null_vec{0, 0, 0};

inline double normalize(double dx, double L) {
    while (dx < -L / 2) dx += L;
    while (dx >= L / 2) dx -= L;
    return dx;
}

class Particle {
    //////////////////////
    /// Force calculation
    //////////////////////
    friend bool force(Particle& p1, Particle& p2, double lx, double ly, double lz, double timestep);
    friend bool force(Particle& p, Particle& b, BasePlate& basePlate, double timestep);


public:
    Particle(double x, double y, double z, size_t index, ParticleProps props) :
        rtd0(x, y, z), index(index), rtd1(null_vec), rtd2(null_vec), _r(props.radius),
        _m(props.mass), _youngs_modulus(props.youngs_modulus), _poisson(props.poisson),
        _damping_constant(props.damping_factor), _friction(props.friction), _tangential_damping(props.tangential_damping){J = 0.4*_m*_r*_r;}
    Particle(const Particle& rhs) = default;
    Particle(Particle && rhs) = default;
    Particle& operator=(const Particle & rhs) = default;
    Particle& operator=(Particle && rhs) = default;
    virtual ~Particle() = default;

    ///////////////////////////////////////
    /// Getters
    ///////////////////////////////////////
    double& r() { return _r; }
    double r() const { return _r; }
    double m() const { return _m; }

    double& x() { return rtd0.x(); }
    double x() const { return rtd0.x(); }
    double& y() { return rtd0.y(); }
    double y() const { return rtd0.y(); }
    double& z() { return rtd0.z(); }
    double z() const { return rtd0.z(); }
    double& vx() { return rtd1.x(); }
    double vx() const { return rtd1.x(); }
    double& vy() { return rtd1.y(); }
    double vy() const { return rtd1.y(); }
    double& vz() { return rtd1.z(); }
    double vz() const { return rtd1.z(); }
    Eigen::Vector3d & force() {return _force;}
    Eigen::Vector3d force() const {return _force;}

    ///////////////////////////////////////
    /// Setters
    ///////////////////////////////////////
    void add_force(const Eigen::Vector3d& f) {_force += f;}
    void add_torque(const Eigen::Vector3d & t) {_torque += t;}
    void periodic_bc(double x_0, double y_0, double lx, double ly);
    void set_force_to_zero() { _force = null_vec;}
    void set_torque_to_zero() {_torque = null_vec;}
    void set_z(double z) {rtd0 += Eigen::Vector3d(0, 0, z);}

    ///////////////////////////////////////
    /// Integration
    ////////////////////////////////////
    void predict(double dt);
    void correct(double dt, Eigen::Vector3d G);

    void update_base_contacts(std::set<size_t>& contacts);
    void update_particle_contacts(std::set<size_t>& contacts);

private:
    Eigen::Vector3d rtd0{null_vec}, rtd1{null_vec}, rtd2{null_vec}, rtd3{null_vec};
    Eigen::Vector3d rot0{null_vec}, rot1{null_vec}, rot2{null_vec}, rot3{null_vec};
    Eigen::Vector3d _force{null_vec};
    Eigen::Vector3d _torque{null_vec};
    double _r;
    double _m;
    double _youngs_modulus;
    double _poisson;
    double _damping_constant;
    double J{0};
    double _friction; // Coefficient of friction
    double _tangential_damping;
    size_t index;

    // Keep track of contacts with base particles
    std::map<size_t, Eigen::Vector3d> base_contacts; // <particle index, spring elongation>

    std::map<size_t, Eigen::Vector3d> particle_contacts;

};
#endif //INC_3DMOLECULARDYNAMICS_PARTICLE_H
