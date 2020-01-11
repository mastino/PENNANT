/*
 * main.cc
 *
 *  Created on: Jan 23, 2012
 *      Author: cferenba
 *
 * Copyright (c) 2012, Los Alamos National Security, LLC.
 * All rights reserved.
 * Use of this source code is governed by a BSD-style open-source
 * license; see top-level LICENSE file for full license text.
 */

#include <cstdlib>
#include <string>
#include <iostream>
#include <omp.h>

#include "Parallel.hh"
#include "InputFile.hh"
#include "Driver.hh"

#ifdef USE_CALI
#include <caliper/cali.h>
#endif

using namespace std;


int main(const int argc, const char** argv)
{
    Parallel::init();

    if (argc != 2) {
        if (Parallel::mype == 0)
            cerr << "Usage: pennant <filename>" << endl;
        exit(1);
    }

#ifdef USE_CALI
cali_id_t thread_attr = cali_create_attribute("thread_id", CALI_TYPE_INT, CALI_ATTR_ASVALUE | CALI_ATTR_SKIP_EVENTS);
#pragma omp parallel
{
cali_set_int(thread_attr, omp_get_thread_num());
}
#endif

    const char* filename = argv[1];
    InputFile inp(filename);

    string probname(filename);
    // strip .pnt suffix from filename
    int len = probname.length();
    if (probname.substr(len - 4, 4) == ".pnt")
        probname = probname.substr(0, len - 4);

    Driver drv(&inp, probname);

    drv.run();

    Parallel::final();

    return 0;

}



