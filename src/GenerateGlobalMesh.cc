/*
 * GenerateGlobalMesh.cc
 *
 *  Created on: Aug 16, 2016
 *      Author: jgraham
 */

#include "GenerateGlobalMesh.hh"

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>

using namespace std;

GenerateGlobalMesh::GenerateGlobalMesh(const InputParameters& input_params) :
	meshtype_(input_params.meshtype_),
	global_nzones_x_(input_params.directs_.nzones_x_),
	global_nzones_y_(input_params.directs_.nzones_y_),
	len_x_(input_params.directs_.len_x_),
	len_y_(input_params.directs_.len_y_)
{
    calcPartitions();
}


GenerateGlobalMesh::~GenerateGlobalMesh() {}


// TODO move bulk of work to SPMD tasks
void GenerateGlobalMesh::generate(
        std::vector<double2>& point_position,
        std::vector<int>& zone_pts_ptr,
        std::vector<int>& zonepoints) const {
    // mesh type-specific calculations
    std::vector<int> zonesize;
    if (meshtype_ == "pie")
        generatePie(point_position, zone_pts_ptr, zonesize, zonepoints);
    else if (meshtype_ == "rect")
        generateRect(point_position, zone_pts_ptr, zonesize, zonepoints);
    else if (meshtype_ == "hex")
        generateHex(point_position, zone_pts_ptr, zonesize, zonepoints);

    zone_pts_ptr.push_back(zone_pts_ptr.back()+zonesize.back());  // compressed row storage

    assert(point_position.size() == numberOfPoints());
	assert((zone_pts_ptr.size() - 1) == numberOfZones());
	assert(zonepoints.size() == numberOfSides());
}


void GenerateGlobalMesh::generateRect(
        std::vector<double2>& pointpos,
        std::vector<int>& zonestart,
        std::vector<int>& zonesize,
        std::vector<int>& zonepoints) const {

    const int nz = global_nzones_x_ * global_nzones_y_;
    const int npx = global_nzones_x_ + 1;
    const int npy = global_nzones_y_ + 1;
    const int np = npx * npy;
    const int zone_y_offset_ = 0;
    const int zone_x_offset_ = 0;

    // generate point coordinates
    pointpos.reserve(np);
    double dx = len_x_ / (double) global_nzones_x_;
    double dy = len_y_ / (double) global_nzones_y_;
    for (int j = 0; j < npy; ++j) {
        double y = dy * (double) (j + zone_y_offset_);
        for (int i = 0; i < npx; ++i) {
            double x = dx * (double) (i + zone_x_offset_);
            pointpos.push_back(make_double2(x, y));
        }
    }
    zonestart.reserve(nz);
    zonesize.reserve(nz);
    zonepoints.reserve(4 * nz);
    for (int j = 0; j < global_nzones_y_; ++j) {
        for (int i = 0; i < global_nzones_x_; ++i) {
            zonestart.push_back(zonepoints.size());
            zonesize.push_back(4);
            int p0 = j * npx + i;
            zonepoints.push_back(p0);
            zonepoints.push_back(p0 + 1);
            zonepoints.push_back(p0 + npx + 1);
            zonepoints.push_back(p0 + npx);
       }
    }

    // generate zone adjacency lists

}


void GenerateGlobalMesh::generatePie(
        std::vector<double2>& pointpos,
        std::vector<int>& zonestart,
        std::vector<int>& zonesize,
        std::vector<int>& zonepoints) const {

    const int nz = global_nzones_x_ * global_nzones_y_;
    const int npx = global_nzones_x_ + 1;
    const int npy = global_nzones_y_ + 1;
    const int zone_y_offset_ = 0;
    const int zone_x_offset_ = 0;
    const int proc_index_y_ = 0;
    const int np = (proc_index_y_ == 0 ? npx * (npy - 1) + 1 : npx * npy);

    // generate point coordinates
    pointpos.reserve(np);
    double dth = len_x_ / (double) global_nzones_x_;
    double dr  = len_y_ / (double) global_nzones_y_;
    for (int j = 0; j < npy; ++j) {
        if (j + zone_y_offset_ == 0) {
            pointpos.push_back(make_double2(0., 0.));
            continue;
        }
        double r = dr * (double) (j + zone_y_offset_);
        for (int i = 0; i < npx; ++i) {
            double th = dth * (double) (global_nzones_x_ - (i + zone_x_offset_));
            double x = r * cos(th);
            double y = r * sin(th);
            pointpos.push_back(make_double2(x, y));
        }
    }

    // generate zone adjacency lists
    zonestart.reserve(nz);
    zonesize.reserve(nz);
    zonepoints.reserve(4 * nz);
    for (int j = 0; j < global_nzones_y_; ++j) {
        for (int i = 0; i < global_nzones_x_; ++i) {
            zonestart.push_back(zonepoints.size());
            int p0 = j * npx + i;
            if (proc_index_y_ == 0) p0 -= npx - 1;
            if (j + zone_y_offset_ == 0) {
                zonesize.push_back(3);
                zonepoints.push_back(0);
            }
            else {
                zonesize.push_back(4);
                zonepoints.push_back(p0);
                zonepoints.push_back(p0 + 1);
            }
            zonepoints.push_back(p0 + npx + 1);
            zonepoints.push_back(p0 + npx);
        }
    }

}


void GenerateGlobalMesh::generateHex(
        std::vector<double2>& pointpos,
        std::vector<int>& zonestart,
        std::vector<int>& zonesize,
        std::vector<int>& zonepoints) const {

    const int nz = global_nzones_x_ * global_nzones_y_;
    const int npx = global_nzones_x_ + 1;
    const int npy = global_nzones_y_ + 1;
    const int zone_y_offset_ = 0;
    const int zone_x_offset_ = 0;
    const int proc_index_x_ = 0;

    // generate point coordinates
    pointpos.reserve(2 * npx * npy);  // upper bound
    double dx = len_x_ / (double) (global_nzones_x_ - 1);
    double dy = len_y_ / (double) (global_nzones_y_ - 1);

    vector<int> pbase(npy);
    for (int j = 0; j < npy; ++j) {
        pbase[j] = pointpos.size();
        int gj = j + zone_y_offset_;
        double y = dy * ((double) gj - 0.5);
        y = max(0., min(len_y_, y));
        for (int i = 0; i < npx; ++i) {
            int gi = i + zone_x_offset_;
            double x = dx * ((double) gi - 0.5);
            x = max(0., min(len_x_, x));
            if (gi == 0 || gi == global_nzones_x_ || gj == 0 || gj == global_nzones_y_)
                pointpos.push_back(make_double2(x, y));
            else if (i == global_nzones_x_ && j == 0)
                pointpos.push_back(
                        make_double2(x - dx / 6., y + dy / 6.));
            else if (i == 0 && j == global_nzones_y_)
                pointpos.push_back(
                        make_double2(x + dx / 6., y - dy / 6.));
            else {
                pointpos.push_back(
                        make_double2(x - dx / 6., y + dy / 6.));
                pointpos.push_back(
                        make_double2(x + dx / 6., y - dy / 6.));
            }
        } // for i
    } // for j
    //int np = pointpos.size();

    // generate zone adjacency lists
    zonestart.reserve(nz);
    zonesize.reserve(nz);
    zonepoints.reserve(6 * nz);  // upper bound
    for (int j = 0; j < global_nzones_y_; ++j) {
        int gj = j + zone_y_offset_;
        int pbasel = pbase[j];
        int pbaseh = pbase[j+1];
        if (proc_index_x_ > 0) {
            if (gj > 0) pbasel += 1;
            if (j < global_nzones_y_ - 1) pbaseh += 1;
        }
        for (int i = 0; i < global_nzones_x_; ++i) {
            int gi = i + zone_x_offset_;
            vector<int> v(6);
            v[1] = pbasel + 2 * i;
            v[0] = v[1] - 1;
            v[2] = v[1] + 1;
            v[5] = pbaseh + 2 * i;
            v[4] = v[5] + 1;
            v[3] = v[4] + 1;
            if (gj == 0) {
                v[0] = pbasel + i;
                v[2] = v[0] + 1;
                if (gi == global_nzones_x_ - 1) v.erase(v.begin()+3);
                v.erase(v.begin()+1);
            } // if j
            else if (gj == global_nzones_y_ - 1) {
                v[5] = pbaseh + i;
                v[3] = v[5] + 1;
                v.erase(v.begin()+4);
                if (gi == 0) v.erase(v.begin()+0);
            } // else if j
            else if (gi == 0)
                v.erase(v.begin()+0);
            else if (gi == global_nzones_x_ - 1)
                v.erase(v.begin()+3);
            zonestart.push_back(zonepoints.size());
            zonesize.push_back(v.size());
            zonepoints.insert(zonepoints.end(), v.begin(), v.end());
        } // for i
    } // for j

}

int GenerateGlobalMesh::numberOfPoints() const {
    if (meshtype_ == "pie")
        return numberOfPointsPie();
    else if (meshtype_ == "rect")
    	return numberOfPointsRect();
    else if (meshtype_ == "hex")
    	return numberOfPointsHex();
    else
    	return -1;
}


int GenerateGlobalMesh::numberOfPointsRect() const {
    return (global_nzones_x_ + 1) * (global_nzones_y_ + 1);
}


int GenerateGlobalMesh::numberOfPointsPie() const {
    return (global_nzones_x_ + 1) * global_nzones_y_ + 1;
}


int GenerateGlobalMesh::numberOfPointsHex() const {
    return 2 * global_nzones_x_ * global_nzones_y_ + 2;
}


int GenerateGlobalMesh::numberOfZones() const {
	return global_nzones_x_ * global_nzones_y_;
}


void GenerateGlobalMesh::colorPartitions(const std::vector<int>& zone_pts_ptr,
		Coloring *zone_map, Coloring *side_map,
		Coloring *pt_map) const
{
    if (meshtype_ == "pie")
    		colorPartitionsPie(zone_pts_ptr, zone_map, side_map, pt_map);
    else if (meshtype_ == "rect")
    		colorPartitionsRect(zone_pts_ptr, zone_map, side_map, pt_map);
    else if (meshtype_ == "hex")
    		colorPartitionsHex(zone_pts_ptr, zone_map, side_map, pt_map);
}


void GenerateGlobalMesh::colorPartitionsPie(const std::vector<int>& zone_pts_ptr,
		Coloring *zone_map, Coloring *side_map,
		Coloring *pt_map) const
{
	colorPartitionsRect(zone_pts_ptr, zone_map, side_map, pt_map);
}


void GenerateGlobalMesh::colorPartitionsHex(const std::vector<int>& zone_pts_ptr,
		Coloring *zone_map, Coloring *side_map,
		Coloring *pt_map) const
{
	colorPartitionsRect(zone_pts_ptr, zone_map, side_map, pt_map);
}


void GenerateGlobalMesh::colorPartitionsRect(const std::vector<int>& zone_pts_ptr,
		Coloring *zone_map, Coloring *side_map,
		Coloring *local_pt_map) const
{
	for (int proc_index_y = 0; proc_index_y < num_proc_y_; proc_index_y++) {
		const int zone_y_start = proc_index_y * global_nzones_y_ / num_proc_y_;
		const int zone_y_stop = (proc_index_y + 1) * global_nzones_y_ / num_proc_y_;
		for (int proc_index_x = 0; proc_index_x < num_proc_x_; proc_index_x++) {
			const int color = proc_index_y * num_proc_x_ + proc_index_x;
			const int zone_x_start = proc_index_x * global_nzones_x_ / num_proc_x_;
			const int zone_x_stop = (proc_index_x + 1) * global_nzones_x_ / num_proc_x_;
			for (int j = zone_y_start; j < zone_y_stop; j++) {
				for (int i = zone_x_start; i < zone_x_stop; i++) {
					int zone = j * (global_nzones_x_) + i;
					(*zone_map)[color].points.insert(zone);
					(*side_map)[color].ranges.insert(pair<int, int>(zone_pts_ptr[zone], zone_pts_ptr[zone+1] - 1));
				}
			}
			for (int j = zone_y_start; j <= zone_y_stop; j++) {
				for (int i = zone_x_start; i <= zone_x_stop; i++) {
					int pt = j * (global_nzones_x_ + 1) + i;
					(*local_pt_map)[color].points.insert(pt);
				}
			}
		}
	}
}


int GenerateGlobalMesh::numberOfSides() const {
    if (meshtype_ == "pie")
    	return numberOfCornersPie();
    else if (meshtype_ == "rect")
    	return numberOfCornersRect();
    else if (meshtype_ == "hex")
    	return numberOfCornersHex();
    else
    	return -1;
}


int GenerateGlobalMesh::numberOfCornersRect() const {
    return 4 * numberOfZones();
}


int GenerateGlobalMesh::numberOfCornersPie() const {
    return 4 * numberOfZones() - global_nzones_x_;
}


int GenerateGlobalMesh::numberOfCornersHex() const {
    return 6 * numberOfZones() - 2 * global_nzones_x_ - 2 * global_nzones_y_ + 2;
}


void GenerateGlobalMesh::calcPartitions() {

    // pick numpex, numpey such that PE blocks are as close to square
    // as possible
    // we would like:  gnzx / numpex == gnzy / numpey,
    // where numpex * numpey = numpe (total number of PEs available)
    // this solves to:  numpex = sqrt(numpe * gnzx / gnzy)
    // we compute this, assuming gnzx <= gnzy (swap if necessary)
    double nx = static_cast<double>(global_nzones_x_);
    double ny = static_cast<double>(global_nzones_y_);
    bool swapflag = (nx > ny);
    if (swapflag) swap(nx, ny);
    double n = sqrt(Parallel::num_subregions() * nx / ny);
    // need to constrain n to be an integer with numpe % n == 0
    // try rounding n both up and down
    int n1 = floor(n + 1.e-12);
    n1 = max(n1, 1);
    while (Parallel::num_subregions() % n1 != 0) --n1;
    int n2 = ceil(n - 1.e-12);
    while (Parallel::num_subregions() % n2 != 0) ++n2;
    // pick whichever of n1 and n2 gives blocks closest to square,
    // i.e. gives the shortest long side
    double longside1 = max(nx / n1, ny / (Parallel::num_subregions()/n1));
    double longside2 = max(nx / n2, ny / (Parallel::num_subregions()/n2));
    num_proc_x_ = (longside1 <= longside2 ? n1 : n2);
    num_proc_y_ = Parallel::num_subregions() / num_proc_x_;
    if (swapflag) swap(num_proc_x_, num_proc_y_);

}