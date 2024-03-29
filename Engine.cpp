//
// Created by ppxjd3 on 14/07/2021.
//

#include "Engine.h"

#include <memory>

Engine::Engine(Options& options)
        : _options{options}, lx{options.systemProps.lx}, ly{options.systemProps.ly}, lz{options.systemProps.lz}{
    begin = std::chrono::steady_clock::now();
    timestep = options.programOptions.timestep;
    f1 = fopen(_options.programOptions.savepath.string().c_str(), "w");
    f3 = fopen(_options.programOptions.csvSavePath.string().c_str(), "w");
    dump_csv_header(f3);
    if (_options.programOptions.dump_separate){
        f2 = fopen(_options.programOptions.savepathbase.string().c_str(), "w");
    }
    init_system();

    basePlate.set_zi(_options.systemProps.base_height);
    basePlate.update(0.0);
    dump(true);
}

void Engine::init_system() {
    adjust_box_dimensions();

    add_particles();
    add_base_particles();
    create_dimples();

    init_lattice_algorithm();
    init_lattice_algorithm_for_base_particles();
}

void Engine::dump(bool first) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::cout << "DUMP Simulation Time : " << Time << " s\t"
        << "Timestep : " << Time/timestep << "\t"
        << "Elapsed time: " << std::chrono::duration_cast<std::chrono::seconds>(now-begin).count() << "s" << std::endl;

    if (step_number >= _options.programOptions.save_delay) {

        dump_preamble(f1, true, !_options.programOptions.dump_separate);
        dump_particles(f1);
    }
    if (!_options.programOptions.dump_separate){
        dump_base(f1);
    }
    std::fflush(f1);
    std::fflush(f3);

    if (first){
        if (_options.programOptions.dump_separate){
            dump_preamble(f2, false, true);
            dump_base(f2);
            fclose(f2);
        }
    }
}

void Engine::dump_preamble(std::FILE *f, bool inc_particles, bool inc_base_particles) const {
    int N = 0;
    if (inc_particles) N += no_of_particles;
    if (inc_base_particles) N += no_of_base_particles;
    std::fprintf(f, "ITEM: TIMESTEP\n%d\n", int(Time / timestep));
    std::fprintf(f, "ITEM: TIME\n%.8f\n", Time);
    std::fprintf(f, "ITEM: AMPLITUDE\n%.8f\n", basePlate.A());
    std::fprintf(f, "ITEM: BOX BOUNDS pp pp f\n%.4f %.4f\n%.4f %.4f\n%.4f %.4f\n", 0.0, lx, 0.0, ly, 0.0, lz);
    std::fprintf(f, "ITEM: NUMBER OF ATOMS\n%d\n", N);
    std::fprintf(f, "ITEM: ATOMS x y z vx vy vz radius type\n");
}

void Engine::dump_csv_header(std::FILE *f) const {
    std::fprintf(f, "frame,particle,time,x,y,z,vx,vy,vz,radius,type\n");
}

void Engine::dump_particles(std::FILE *f) {
    for (Particle& p : particles) {
        std::fprintf(f, "%.9f %.9f %.9f %.9f %.9f %.9f %.9f %d\n", p.x(), p.y(), p.z(), p.vx(), p.vy(), p.vz(), p.r(), 0);
    }
}

void Engine::dump_particle_to_csv(std::FILE *f) {
    int p_n{0};
    for (Particle& p : particles){
        std::fprintf(f, "%d,%d,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%d\n", step_number, p_n, Time, p.x(), p.y(), p.z(), p.vx(), p.vy(), p.vz(), p.r(), 0);
        p_n++;
    }
}

void Engine::dump_base(std::FILE *f) {
    for (Particle &p: base_particles) {
        std::fprintf(f, "%.9f %.9f %.9f %.9f %.9f %.9f %.9f %d\n", p.x(), p.y(), p.z() + basePlate.z(), p.vx(),
                     p.vy(), basePlate.vz(), p.r(), 1);
    }
}

void Engine::step() {
     // Check whether the optimiser needs updating
     if (ilist_needs_update()) {make_ilist();}

     basePlate.update(Time);

     integrate();

     check_dump();
}

void Engine::integrate() {
    // Set forces to zero
    std::for_each(particles.begin(), particles.end(),
                  [&](Particle& p){
        p.set_force_to_zero();
        p.set_torque_to_zero();
        p.predict(timestep);
    });

    // Calculate all the forces between particles
    make_forces();

    // Calculate all the forces between the particles and the plate
    make_plate_forces();

    // Update  the positions of all the particles
    std::for_each(particles.begin(), particles.end(),
                  [&](Particle& p){
            p.correct(timestep, G);
    });

    // Apply periodic boundary conditions
    std::for_each(particles.begin(), particles.end(),
                  [&](Particle& p) {p.periodic_bc(0, 0, lx, ly);});

    Time += timestep;
    step_number++;
}




///////////////////////////////////////////////////////////////////////////////
/// Lattice Method
///////////////////////////////////////////////////////////////////////////////

void Engine::init_lattice_algorithm() {
    rmin = particles.at(0).r();
    rmax = particles.at(0).r();

    // Calculate gk, the size of the lattice sites
    gk = sqrt(2) * rmin;

    // Calcualte gm, the number of lattice sites that the largest particle covers
    gm = int(2*rmax/gk) + 1;

    // calculate the numebr of lattice sites in each dimension
    Nx = int(lx / gk) + 1;
    Ny = int(ly / gk + 1);
    partners.resize(no_of_particles);
    pindex.resize(Nx);
    for (auto& p : pindex){
        p.resize(Ny);
    }
    clear_pindex();
    make_ilist();
}

void Engine::make_ilist() {
    // For each particle, add it to the pindex lattice site
    // and clear its partners
    for (unsigned int i{0}; i<no_of_particles; i++){
        double x = particles[i].x();
        double y = particles[i].y();
        int ix = int(x / gk);
        int iy = int(y / gk);
        pindex[ix][iy] = (int)i;
        partners[i].clear();
    }

    // Generate the partners list for each particle
    for (unsigned int i{ 0 }; i < no_of_particles; i++) {
        double x = particles[i].x();
        double y = particles[i].y();
        if ((x >= 0.0) && (x < lx) && (y >= 0.0) && (y < ly)) {
            int ix = int(x / gk);
            int iy = int(y / gk);
            // Check the adjacent gm lattice sites for particles
            for (int dx = -gm; dx <= gm; dx++) {
                for (int dy = -gm; dy <= gm; dy++) {
                    int iix = (ix + dx + Nx) % Nx;
                    int iiy = (iy + dy + Ny) % Ny;
                    int k = pindex[iix][iiy];
                    // Only record the particle once
                    if (k > (int)i) {
                        partners[i].push_back(k);
                    }
                }
            }
        }
    }
}

bool Engine::ilist_needs_update() {
    // If the pindex is the same as last time then it doesn't need updating
    for (unsigned int i{ 0 }; i < no_of_particles; i++) {
        double x = particles[i].x();
        double y = particles[i].y();
        int ix = int(x / gk);
        int iy = int(y/ gk);
        if (pindex.at(ix).at(iy) != i) {
            clear_pindex();
            return true;
        }
    }
    return false;
}

void Engine::clear_pindex()
{
    for (auto& p : pindex) {
        for (auto& q : p) {
            q = -1;
        }
    }
}

void Engine::make_forces() {
    // Loop over the partners list for each particle
    for (unsigned int i{ 0 }; i < no_of_particles; i++) {
        std::set<size_t> contacts;
        for (unsigned int k{ 0 }; k < partners[i].size(); k++) {
            int pk = partners[i][k];
            bool contact = force(particles[i], particles[pk], _options.systemProps.lx, _options.systemProps.ly, _options.systemProps.lz, timestep);
            if (contact) contacts.insert(pk);
        }
        particles[i].update_particle_contacts(contacts);
    }
}

void Engine::make_plate_forces() {
    for (auto& p: particles){
            std::set<size_t> contacts;
            double x = p.x();
            double y = p.y();
            int ix = int(x / gk_base);
            int iy = int(y / gk_base);
            for (int dx = -gm_base; dx <= gm_base; dx++) {
                for (int dy = -gm_base; dy <= gm_base; dy++) {
                    int iix = (ix + dx + nx_base) % (nx_base);
                    int iiy = (iy + dy + ny_base) % (ny_base);
                    int k = pindex_base[iix][iiy];
                    if (k>=0) {
                        bool contact = force(p, base_particles.at(k), basePlate, timestep);
                        if (contact) contacts.insert(k);
                    }
                }
            }
            p.update_base_contacts(contacts);
    }
}

void Engine::check_dump() {
    if (step_number <= _options.programOptions.save_delay){
        if (save != 1000){
            save++;
        } else {
            save = 1;
            dump(false);
        }
    }
    else {
        if (save != _options.programOptions.save_interval) {
            save++;
        } else {
            dump(false);
            save = 1;
        }

        if (save_csv != _options.programOptions.csv_interval){
            save_csv++;
        } else {
            dump_particle_to_csv(f3);
            save_csv = 1;
        }
    }
}


void Engine::add_particles() {
    double r = _options.ballProps.radius;
    double dx = 2*r;
    double dy = 2*r*sqrt(3.0)/2.0;
    int nx = floor(lx / dx) - 1;
    int ny = floor(ly / dy) - 1;

    std::random_device rd;
    std::default_random_engine eng(rd());
    std::uniform_real_distribution<> distr(0.0, 1.0);

    size_t index = 0;
    for (int i{0}; i < nx; i++){
        for (int j{0}; j < ny; j++){
            double x = double(i)*dx + double(j%2)*dx/2.0;
            double y = double(j)*dy;
            double z = _options.systemProps.ball_height;
            Particle pp(x, y, z, index, _options.ballProps);
            double area_fraction = _options.systemProps.area_fraction;
            if (distr(eng) < area_fraction) {
                particles.push_back(pp);
                index++;
            }
        }
    }
    no_of_particles = particles.size();
}

void Engine::add_base_particles() {
    double r = _options.baseProps.radius;
    double dx = 2*r;
    double dy = 2*r*sqrt(3.0)/2.0;

    int nx = floor(lx / dx);
    int ny = floor(ly / dy);
    size_t index = 0;
    for (int i{0}; i <= nx; i++){
        for (int j{0}; j <= ny; j++){
            double x = double(i)*dx + double(j%2)*dx/2.0;
            double y = double(j)*dy;
            double z = _options.systemProps.base_height;
            Particle pp(x, y, z, index, _options.baseProps);
            base_particles.push_back(pp);
            index++;
        }
    }
    no_of_base_particles = base_particles.size();
}



void Engine::init_lattice_algorithm_for_base_particles() {
    r_base = base_particles.at(0).r();
    gk_base = sqrt(2)*r_base;
    gm_base = int(2*particles.at(0).r()/gk+1);
    nx_base = int(lx/gk_base)+1;
    ny_base = int(ly/gk_base)+1;

    pindex_base.resize(nx_base);
    for (auto & px : pindex_base){
        px.resize(ny_base);
    }
    clear_pindex_base();

    for (int i=0; i<base_particles.size(); i++) {
        double x = base_particles[i].x();
        double y = base_particles[i].y();
        int ix = int(x/gk_base);
        int iy = int(y/gk_base);
        pindex_base.at(ix).at(iy)=i;
    }
}

void Engine::clear_pindex_base() {
    for (auto &px: pindex_base) {
        for (auto &py: px) {
            py = -1;
        }
    }
}

void Engine::create_dimples() {

    // Make kd tree of base particles
    my_vector_of_vectors_t tree_input;
    for (auto &p: base_particles) {
        tree_input.push_back({p.x(), p.y()});
    }
    const size_t dims{2};
    auto tree = my_kd_tree_t{dims, tree_input, 10};
    tree.index->buildIndex();


    double L = _options.systemProps.dimple_spacing;
    double dx = L;
    double dy = L * sqrt(3) / 2;

    int nx = ceil(lx / dx);
    int ny = ceil(ly / dy);
    for (int i{0}; i <= nx; i++) {
        for (int j{0}; j <= ny; j++) {
            double x = double(i) * dx + double(j % 2) * dx / 2.0;
            double y = double(j) * dy;
            const double query_pt[2] = {x, y};

            std::vector<std::pair<size_t, double>> ret_matches;
            nanoflann::SearchParams params;
            const size_t nMatches = tree.index->radiusSearch(&query_pt[0], _options.systemProps.dimple_radius*_options.systemProps.dimple_radius,
                                                             ret_matches, params);
            for (int n{0}; n < nMatches; n++) {
                size_t index = ret_matches[n].first;
                base_particles[index].set_z(-_options.systemProps.dimple_depth);
            }
        }

    }
}

void Engine::adjust_box_dimensions() {
    double L = _options.systemProps.dimple_spacing;
    double dx = L;
    double dy = L * sqrt(3) / 2;

    int nx = ceil(lx / dx);
    if (nx % 2 == 0) nx += 1;
    std::cout << "Nx: " << nx << std::endl;
    lx = dx * nx;

    int ny = ceil(ly / dy);
    if (ny % 2 == 1) ny += 1;
    std::cout << "Ny: " << ny << std::endl;
    ly = dy * ny;
}




