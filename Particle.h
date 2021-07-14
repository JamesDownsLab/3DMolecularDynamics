//
// Created by ppxjd3 on 14/07/2021.
//

#ifndef INC_3DMOLECULARDYNAMICS_PARTICLE_H
#define INC_3DMOLECULARDYNAMICS_PARTICLE_H

#include <iostream>
#include "Vector.h"

class Particle {

public:
    Particle() : rtd0(null), rtd1(null), rtd2(null), rtd3(null), rtd4(null) {}
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
    Vector& force() {return _force;}
    Vector force() const {return _force;}

    ///////////////////////////////////////
    /// Setters
    ///////////////////////////////////////
    void add_force(const Vector& f) {_force += f;}
    void periodic_bc(double x_0, double y_0, double lx, double ly);
    void boundary_conditions(double timestep, double Time);

    void velocity_verlet(double dt);

private:
    Vector rtd0, rtd1, rtd2, rtd3, rtd4;
    Vector rtd0_old, rtd0_new;
    Vector _force;
    double _r, _m;
};


#endif //INC_3DMOLECULARDYNAMICS_PARTICLE_H
