/* 
 *  This file is part of the Chronus Quantum (ChronusQ) software package
 *  
 *  Copyright (C) 2014-2018 Li Research Group (University of Washington)
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *  
 *  Contact the Developers:
 *    E-Mail: xsli@uw.edu
 *  
 */

#include "scf.hpp"

BOOST_AUTO_TEST_SUITE( UHF_GIAO )

// H2 TRIPLET GIAO 6-31G test  B = 0, 0, -0.001
BOOST_FIXTURE_TEST_CASE( H2_TRIPLET_UHF_GIAO_631G, SerialJob ) {

  CQSCFTEST( "scf/serial/uhf_giao/h2_triplet_uhf_giao_631G", "h2_triplet_uhf_giao_631G.bin.ref", 1e-6 );
 
};

#ifdef _CQ_DO_PARTESTS

// SMP H2 TRIPLET GIAO 6-31G test  B = 0, 0, -0.001
BOOST_FIXTURE_TEST_CASE( Par_H2_TRIPLET_UHF_GIAO_631G, ParallelJob ) {

  CQSCFTEST( "scf/parallel/uhf_giao/h2_triplet_uhf_giao_631G", "h2_triplet_uhf_giao_631G.bin.ref", 1e-6 );
 
};

#endif

BOOST_AUTO_TEST_SUITE_END()
