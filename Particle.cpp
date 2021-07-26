//
// Created by ppxjd3 on 14/07/2021.
//

#include "Particle.h"
#include <cmath>



void force(Particle &p1, Particle &p2, double lx, double ly, double lz) {
    double dx = normalize(p1.x() - p2.x(), lx);
    double dy = normalize(p1.y() - p2.y(), ly);
    double dz = normalize(p1.z() - p2.z(), lz);
    if (std::abs(dx) < p1.r() + p2.r() && std::abs(dy) < p1.r() + p2.r() && std::abs(dz) < p1.r() + p2.r()){
        double rr = sqrt(dx*dx + dy*dy + dz*dz);
        double r1 = p1.r();
        double r2 = p2.r();

        // Overlap
        double xi = r1 + r2 - rr;

        if (xi > 1e-10) { // If overlapping



            double Y = p1._youngs_modulus;
            double poisson = p1._poisson;
            double force_constant = 2*Y*sqrt(r1)/(3*(1-poisson*poisson));

            double sqrt_xi = sqrt(xi);
            double rr_rez = 1 / rr;

            // Unit vectors
            double ex = dx * rr_rez;
            double ey = dy * rr_rez;
            double ez = dz * rr_rez;

            double dvx = p1.vx() - p2.vx();
            double dvy = p1.vy() - p2.vy();
            double dvz = p1.vz() - p2.vz();

            Eigen::Vector3d n = {ex, ey, ez};

            Eigen::Vector3d v1 = p1.rtd1;
            Eigen::Vector3d v2 = p2.rtd1;
            Eigen::Vector3d omega1 = p1.rot1;
            Eigen::Vector3d omega2 = p2.rot2;
            Eigen::Vector3d vrel = v1 - v2 - (r1*omega1 + r2*omega2).cross(n);
            Eigen::Vector3d vtrel = vrel - vrel.dot(n)*n;
            Eigen::Vector3d t = vtrel * (1/vtrel.norm());

            double xidot = -(ex * dvx + ey * dvy + ez*dvz);

            double gamma = 0.1;

            // Normal Forces
            double elastic_force = force_constant * xi * sqrt_xi;
            double dissipative_force = force_constant * p1._damping_constant * sqrt_xi * xidot;
            double fn = elastic_force + dissipative_force;
            if (fn < 0) fn = 0;

            // Tangential forces
            double mu = 0.5;
            double ft = -gamma * xidot;
            if (ft < -mu*fn) ft = -mu*fn;
            if (ft>mu*fn) ft = mu*fn;

            // Total force
            Eigen::Vector3d force = fn*n + ft*t;

            Eigen::Vector3d torque = force.cross(n);

            p1.add_force(force);
            p2.add_force(-1*force);

            p1.add_torque(torque);
            p2.add_torque(-1*torque);


        }
    }
}


void Particle::periodic_bc(double x_0, double y_0, double lx, double ly) {
    while (rtd0.x() < x_0) rtd0.x() += lx;
    while (rtd0.x() > x_0 + lx) rtd0.x() -= lx;
    while (rtd0.y() < y_0) rtd0.y() += ly;
    while (rtd0.y() > y_0 + ly) rtd0.y() -= ly;
}

void force(Particle &p, Particle &b, BasePlate &basePlate) {
    double dx = p.x() - b.x();
    double dy = p.y() - b.y();
    double dz = p.z() - (basePlate.z()+b.z());
    if (std::abs(dx) < p.r() + b.r() && std::abs(dy) < p.r() + b.r() && std::abs(dz) < p.r() + b.r()) {
        double rr = sqrt(dx*dx + dy*dy + dz*dz);
        double r1 = p.r();
        double r2 = b.r();

        // Overlap
        double xi = r1 + r2 - rr;
        if (xi > 1e-10) {
            double Y = (p._youngs_modulus*b._youngs_modulus)/(p._youngs_modulus+b._youngs_modulus);
            double poisson = 0.5*(p._poisson+b._poisson);
            double A = 0.5*(p._damping_constant + b._damping_constant);
            double force_constant = 2*Y*sqrt(r1)/(3*(1-poisson*poisson));
            double sqrt_xi = sqrt(xi);

            // Unit vectors
            double rr_rez = 1/rr;
            double ex = dx * rr_rez;
            double ey = dy * rr_rez;
            double ez = dz * rr_rez;

            // Relative velocities
            double dvx = p.vx();
            double dvy = p.vy();
            double dvz = p.vz() - basePlate.vz();

            Eigen::Vector3d n = {ex, ey, ez};

            Eigen::Vector3d v1 = p.rtd1;
            Eigen::Vector3d v2 = {0, 0, basePlate.vz()};
            Eigen::Vector3d omega1 = p.rot1;
            Eigen::Vector3d omega2 = {0, 0, 0};
            Eigen::Vector3d vrel = v1 - v2 -  (r1*omega1).cross(n);
            Eigen::Vector3d vtrel = vrel - vrel.dot(n)*n;
            double vtrel_size = vtrel.norm();
            Eigen::Vector3d t;
            if (vtrel_size > 0){
                t = vtrel * (1/vtrel_size);
            }
            else{
                t = {0, 0, 0};
            }

            double xidot = -(ex * dvx + ey * dvy + ez*dvz);
            double gamma = 0.1;

            // Normal forces
            double elastic_force = force_constant * xi * sqrt_xi;
            double dissipative_force = force_constant * p._damping_constant * sqrt_xi * xidot;
            double fn = elastic_force + dissipative_force;
            if (fn < 0) fn = 0;

            // Tangential forces
            double mu = 0.5;
            double ft = -gamma * xidot;
            if (ft < -mu*fn) ft = -mu*fn;
            if (ft > mu*fn) ft = mu*fn;

            // Total force
            Eigen::Vector3d force = fn*n + ft*t;
            Eigen::Vector3d torque = force.cross(n);
            p.add_force(force);
            p.add_torque(torque);
        }
    }
}

void Particle::predict(double dt) {
    double a1 = dt;
    double a2 = a1*dt/2;
    double a3 = a2*dt/3;
    double a4 = a3*dt/4;

    rtd0 += a1*rtd1 + a2*rtd2 + a3*rtd3 + a4*rtd4;
    rtd1 += a1*rtd2 + a2*rtd3 + a3*rtd4;
    rtd2 += a1*rtd3 + a2*rtd4;
    rtd3 += a1*rtd4;

    rot0 += a1*rot1 + a2*rot2 + a3*rot3 + a4*rot4;
    rot1 += a1*rot2 + a2*rot3 + a3*rot4;
    rot2 += a1*rot3 + a2*rot4;
    rot3 += a1*rot4;
}

void Particle::correct(double dt, Eigen::Vector3d G) {

    static Eigen::Vector3d accel, corr, rot_accel, rot_corr;

    double dtrez = 1/dt;
    const double coeff0 = double(19)/double(90) * (dt*dt/double(2));
    const double coeff1 = double(3)/double(4)*(dt/double(2));
    const double coeff3 = double(1)/double(2)*(double(3)*dtrez);
    const double coeff4 = double(1)/double(12)*(double(12)*(dtrez*dtrez));

    accel = Eigen::Vector3d((1/_m)*_force.x()+G.x(), (1/_m)*_force.y()+G.y(), (1/_m)*_force.z()+G.z());
    corr = accel - rtd2;
    rtd0 += coeff0*corr;
    rtd1 += coeff1*corr;
    rtd2 = accel;
    rtd3 += coeff3*corr;
    rtd4 += coeff4*corr;

    rot_accel = _torque * (1/J);
    rot_corr = rot_accel - rot2;
    rot0 += coeff0*rot_corr;
    rot1 += coeff1*rot_corr;
    rot2 = rot_accel;
    rot3 += coeff3*rot_corr;
    rot4 += coeff4*rot_corr;
}
