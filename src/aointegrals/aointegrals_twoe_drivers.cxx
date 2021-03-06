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
//#ifndef __INCLUDED_AOINTEGRALS_TWOE_BUILDERS_HPP__
//#define __INCLUDED_AOINTEGRALS_TOWE_BUILDERS_HPP__

#include <aointegrals.hpp>
#include <cqlinalg.hpp>
#include <cqlinalg/blasutil.hpp>

#include <util/matout.hpp>

#include <util/threads.hpp>

// Debug directives
//#define _DEBUGORTHO
//#define _DEBUGERI
//#define _DEBUGGIAOERI //SS
//#define _DEBUGGIAOONEE //SS 


namespace ChronusQ {

  typedef std::vector<libint2::Shell> shell_set; 

  /**
   *  \brief Allocate, compute and store the full rank-4 ERI tensor using
   *  Libint2 over the CGTO basis. 
   *
   *  Populates internal AOIntegrals<IntsT>::ERI storage
   */ 
  template <typename IntsT>
  void AOIntegrals<IntsT>::computeERIGTO() {

    // Determine the number of OpenMP threads
    int nthreads = GetNumThreads();
 
    // Create a vector of libint2::Engines for possible threading
    std::vector<libint2::Engine> engines(nthreads);

    // Initialize the first engine for the integral evaluation
    engines[0] = libint2::Engine(libint2::Operator::coulomb,
      basisSet_.maxPrim,basisSet_.maxL,0);
    engines[0].set_precision(0.);


    // Copy over the engines to other threads if need be
    for(size_t i = 1; i < nthreads; i++) engines[i] = engines[0];


    // Allocate and zero out ERIs
    size_t NB  = basisSet_.nBasis;
    size_t NB2 = NB*NB;
    size_t NB3 = NB2*NB;
    size_t NB4 = NB2*NB2;

    try { ERI = memManager_.malloc<IntsT>(NB4); } 
    catch(...) {
      std::cout << std::fixed;
      std::cout << "Insufficient memory for the full ERI tensor (" 
                << (NB4/1e9) * sizeof(IntsT) << " GB)" << std::endl;
      std::cout << std::endl << memManager_ << std::endl;
      CErr();
    }
    std::fill_n(ERI,NB4,0.);


    #pragma omp parallel
    {
      int thread_id = GetThreadID();

      // Get threads result buffer
      const auto& buf_vec = engines[thread_id].results();

      size_t n1,n2,n3,n4,i,j,k,l,ijkl,bf1,bf2,bf3,bf4;
      size_t s4_max;
      for(size_t s1(0), bf1_s(0), s1234(0); s1 < basisSet_.nShell; 
          bf1_s+=n1, s1++) { 

        n1 = basisSet_.shells[s1].size(); // Size of Shell 1

      for(size_t s2(0), bf2_s(0); s2 <= s1; bf2_s+=n2, s2++) {

        n2 = basisSet_.shells[s2].size(); // Size of Shell 2

//SS Start generate shellpair1 
/*
        libint2::ShellPair pair1_to_use;
        pair1_to_use.init( basisSet_.shells[s1],basisSet_.shells[s2],-2000);
*/
//SS end

      for(size_t s3(0), bf3_s(0); s3 <= s1; bf3_s+=n3, s3++) {

        n3 = basisSet_.shells[s3].size(); // Size of Shell 3
        s4_max = (s1 == s3) ? s2 : s3; // Determine the unique max of Shell 4

      for(size_t s4(0), bf4_s(0); s4 <= s4_max; bf4_s+=n4, s4++, s1234++) {

        n4 = basisSet_.shells[s4].size(); // Size of Shell 4

//SS start generate shellpair2 and calculate ERI
/*
        libint2::ShellPair pair2_to_use;
        
        pair2_to_use.init( basisSet_.shells[s3],basisSet_.shells[s4],-2000);
            auto two2buff = computeGIAOERIabcd(pair1_to_use,pair2_to_use,
              basisSet_.shells[s1],basisSet_.shells[s2],
              basisSet_.shells[s3],basisSet_.shells[s4],emPert.getAmp());
*/
//SS end

        // Round Robbin work distribution
        #ifdef _OPENMP
        if( s1234 % nthreads != thread_id ) continue;
        #endif


        // Evaluate ERI for shell quartet
        engines[thread_id].compute2<
          libint2::Operator::coulomb, libint2::BraKet::xx_xx, 0>(
          basisSet_.shells[s1],
          basisSet_.shells[s2],
          basisSet_.shells[s3],
          basisSet_.shells[s4]
        );

        // Libint2 internal screening
        const auto *buff =  buf_vec[0] ;
        if(buff == nullptr) continue;

        // Place shell quartet into persistent storage with
        // permutational symmetry
        for(i = 0ul, bf1 = bf1_s, ijkl = 0ul ; i < n1; ++i, bf1++) 
        for(j = 0ul, bf2 = bf2_s             ; j < n2; ++j, bf2++) 
        for(k = 0ul, bf3 = bf3_s             ; k < n3; ++k, bf3++) 
        for(l = 0ul, bf4 = bf4_s             ; l < n4; ++l, bf4++, ++ijkl) {


// SS start compare the difference
/*
if ( std::abs(buff[ijkl]-two2buff[ijkl]) > 1.0e-12  ) {
  std::cout<<"LA "<<basisSet_.shells[s1].contr[0].l
  <<" LB "<<basisSet_.shells[s2].contr[0].l
  <<" LC "<<basisSet_.shells[s3].contr[0].l 
  <<" LD "<<basisSet_.shells[s4].contr[0].l<<std::endl;
  std::cout<<"s1 "<<s1<<" s2 "<<s2<<" s3 "<<s3<<" s4 "<<s4<<std::endl;
  std::cout<<"  buff integral "<<std::setprecision(12)<<buff[ijkl];
  std::cout<<"  own integral  "<<std::setprecision(12)<<two2buff[ijkl]<<std::endl;
}
*/
// SS end


            // (12 | 34)
            ERI[bf1 + bf2*NB + bf3*NB2 + bf4*NB3] = buff[ijkl];
            // (12 | 43)
            ERI[bf1 + bf2*NB + bf4*NB2 + bf3*NB3] = buff[ijkl];
            // (21 | 34)
            ERI[bf2 + bf1*NB + bf3*NB2 + bf4*NB3] = buff[ijkl];
            // (21 | 43)
            ERI[bf2 + bf1*NB + bf4*NB2 + bf3*NB3] = buff[ijkl];
            // (34 | 12)
            ERI[bf3 + bf4*NB + bf1*NB2 + bf2*NB3] = buff[ijkl];
            // (43 | 12)
            ERI[bf4 + bf3*NB + bf1*NB2 + bf2*NB3] = buff[ijkl];
            // (34 | 21)
            ERI[bf3 + bf4*NB + bf2*NB2 + bf1*NB3] = buff[ijkl];
            // (43 | 21)
            ERI[bf4 + bf3*NB + bf2*NB2 + bf1*NB3] = buff[ijkl];


        }; // ijkl loop
      }; // s4
      }; // s3
      }; // s2
      }; // s1
    }; // omp region

    // Debug output of the ERIs
#ifdef _DEBUGERI
    std::cout << "Two-Electron Integrals (ERIs)" << std::endl;
    for(auto i = 0ul; i < NB; i++)
    for(auto j = 0ul; j < NB; j++)
    for(auto k = 0ul; k < NB; k++)
    for(auto l = 0ul; l < NB; l++){
      std::cout << "(" << i << "," << j << "|" << k << "," << l << ")  ";
      std::cout << ERI[i + j*NB  + k*NB2 + l*NB3] << std::endl;
    };
#endif
  }; // AOIntegrals<IntsT>::computeERIRGTO




  /**
   *  \brief Allocate, compute and store the full rank-4 complex ERI tensor using
   *  in house GIAO code over the CGTO basis. 
   *
   *  Populates internal AOIntegrals<IntsT>::GIAOERI storage
   */ 

  template <>
  void AOIntegrals<double>::computeERIGIAO(EMPerturbation &emPert) {
    CErr("GIAO + Real is an invalid option",std::cout);
  };
  template <>
  void AOIntegrals<dcomplex>::computeERIGIAO(EMPerturbation &emPert) {

    // Determine the number of OpenMP threads
    // int nthreads = GetNumThreads();
    
    
    // Create a vector of libint2::Engines for possible threading
    //  std::vector<libint2::Engine> engines(nthreads);

    // Initialize the first engine for the integral evaluation
    /*
    engines[0] = libint2::Engine(libint2::Operator::coulomb,
      basisSet_.maxPrim,basisSet_.maxL,0);
    engines[0].set_precision(0.);
    */

    // Copy over the engines to other threads if need be
    // for(size_t i = 1; i < nthreads; i++) engines[i] = engines[0];


    // define magnetic field

    // Allocate and zero out ERIs
    size_t NB  = basisSet_.nBasis;
    size_t NB2 = NB*NB;
    size_t NB3 = NB2*NB;
    size_t NB4 = NB2*NB2;

    try { ERI = memManager_.malloc<dcomplex>(NB4); } 
    catch(...) {
      std::cout << std::fixed;
      std::cout << "Insufficient memory for the full ERI tensor (" 
                << (NB4/1e9) * sizeof(dcomplex) << " GB)" << std::endl;
      std::cout << std::endl << memManager_ << std::endl;
      CErr();
    }
    std::fill_n(ERI,NB4,0.);

/*
    #pragma omp parallel
    {
      int thread_id = GetThreadID();
      // Get threads result buffer
      const auto& buf_vec = engines[thread_id].results();
*/

      auto magAmp = emPert.getDipoleAmp(Magnetic);
      // std::cout<<"magAmp 2e 0: "<<magAmp[0]<<" 1: "<<magAmp[1]<<" 2: "<<magAmp[2]<<std::endl;   


      size_t n1,n2,n3,n4,i,j,k,l,ijkl,bf1,bf2,bf3,bf4;
      size_t s4_max;
      for(size_t s1(0), bf1_s(0), s1234(0); s1 < basisSet_.nShell; 
          bf1_s+=n1, s1++) { 

        n1 = basisSet_.shells[s1].size(); // Size of Shell 1

      for(size_t s2(0), bf2_s(0); s2 <= s1; bf2_s+=n2, s2++) {

        n2 = basisSet_.shells[s2].size(); // Size of Shell 2

        //SS Start generate shellpair1 

        libint2::ShellPair pair1_to_use;
        pair1_to_use.init( basisSet_.shells[s1],basisSet_.shells[s2],-1000);

        libint2::ShellPair pair1_to_use_switch;
        // switch s1 and s2
        pair1_to_use_switch.init( basisSet_.shells[s2],basisSet_.shells[s1],-1000); 


      for(size_t s3(0), bf3_s(0); s3 <= s1; bf3_s+=n3, s3++) {

        n3 = basisSet_.shells[s3].size(); // Size of Shell 3
        s4_max = (s1 == s3) ? s2 : s3; // Determine the unique max of Shell 4

      for(size_t s4(0), bf4_s(0); s4 <= s4_max; bf4_s+=n4, s4++, s1234++) {

        n4 = basisSet_.shells[s4].size(); // Size of Shell 4

        //SS start generate shellpair2 and calculate GIAO ERI

        libint2::ShellPair pair2_to_use;
        
        pair2_to_use.init( basisSet_.shells[s3],basisSet_.shells[s4],-1000);

#ifdef _DEBUGGIAOERI
 std::cout<<" s1 "<<s1<<" s2 "<<s2<<" s3 "<<s3<<" s4 "<<s4<<std::endl;
#endif

        // calculate integral (s1,s2|s3,s4)
        auto two2buff = ComplexGIAOIntEngine::computeGIAOERIabcd(pair1_to_use,pair2_to_use,
          basisSet_.shells[s1],basisSet_.shells[s2],
          basisSet_.shells[s3],basisSet_.shells[s4],&magAmp[0]);


        // calculate integral (s2,s1|s3,s4)
        auto two2buff_switch = ComplexGIAOIntEngine::computeGIAOERIabcd(pair1_to_use_switch,pair2_to_use,
          basisSet_.shells[s2],basisSet_.shells[s1],
          basisSet_.shells[s3],basisSet_.shells[s4],&magAmp[0]);
        

/*
        // Round Robbin work distribution
        #ifdef _OPENMP
        if( s1234 % nthreads != thread_id ) continue;
        #endif
        // Evaluate ERI for shell quartet
        engines[thread_id].compute2<
          libint2::Operator::coulomb, libint2::BraKet::xx_xx, 0>(
          basisSet_.shells[s1],
          basisSet_.shells[s2],
          basisSet_.shells[s3],
          basisSet_.shells[s4]
        );
        // Libint2 internal screening
        const double *buff = buf_vec[0];
        if(buff == nullptr) continue;
*/

        // Place shell quartet into persistent storage with
        // permutational symmetry
        for(i = 0ul, bf1 = bf1_s, ijkl = 0ul ; i < n1; ++i, bf1++) 
        for(j = 0ul, bf2 = bf2_s             ; j < n2; ++j, bf2++) 
        for(k = 0ul, bf3 = bf3_s             ; k < n3; ++k, bf3++) 
        for(l = 0ul, bf4 = bf4_s             ; l < n4; ++l, bf4++, ++ijkl) {

          int jikl;
          jikl = j*n1*n3*n4 + i*n3*n4 + k*n4 + l;
          
// SS start compare the difference between non switch GIAO and switched-GIAO integrals
/*
if ( std::abs(two2buff[ijkl]-two2buff_switch[ijkl]) > 1.0e-11  ) {
  std::cout<<"LA "<<basisSet_.shells[s1].contr[0].l
  <<" LB "<<basisSet_.shells[s2].contr[0].l
  <<" LC "<<basisSet_.shells[s3].contr[0].l 
  <<" LD "<<basisSet_.shells[s4].contr[0].l<<std::endl;
  std::cout<<"  GIAO integral "<<std::setprecision(12)<<two2buff[ijkl];
  std::cout<<"  GIAO switch integral  "<<std::setprecision(12)<<two2buff_switch[jikl]<<std::endl;
}
*/
// SS end

/*
            // (12 | 34)
            ERI[bf1 + bf2*NB + bf3*NB2 + bf4*NB3] = two2nonbuff[ijkl];
            // (12 | 43)
            ERI[bf1 + bf2*NB + bf4*NB2 + bf3*NB3] = two2nonbuff[ijkl];
            // (21 | 34)
            ERI[bf2 + bf1*NB + bf3*NB2 + bf4*NB3] = two2nonbuff[ijkl];
            // (21 | 43)
            ERI[bf2 + bf1*NB + bf4*NB2 + bf3*NB3] = two2nonbuff[ijkl];
            // (34 | 12)
            ERI[bf3 + bf4*NB + bf1*NB2 + bf2*NB3] = two2nonbuff[ijkl];
            // (43 | 12)
            ERI[bf4 + bf3*NB + bf1*NB2 + bf2*NB3] = two2nonbuff[ijkl];
            // (34 | 21)
            ERI[bf3 + bf4*NB + bf2*NB2 + bf1*NB3] = two2nonbuff[ijkl];
            // (43 | 21)
            ERI[bf4 + bf3*NB + bf2*NB2 + bf1*NB3] = two2nonbuff[ijkl];
*/
            // (12 | 34)
            ERI[bf1 + bf2*NB + bf3*NB2 + bf4*NB3] = two2buff[ijkl];
            // (34 | 12)
            ERI[bf3 + bf4*NB + bf1*NB2 + bf2*NB3] = two2buff[ijkl];
            // (21 | 43)
            ERI[bf2 + bf1*NB + bf4*NB2 + bf3*NB3] = std::conj(two2buff[ijkl]);
            // (43 | 21)
            ERI[bf4 + bf3*NB + bf2*NB2 + bf1*NB3] = std::conj(two2buff[ijkl]);


            // (21 | 34)
            ERI[bf2 + bf1*NB + bf3*NB2 + bf4*NB3] = two2buff_switch[jikl];
            // (34 | 21)
            ERI[bf3 + bf4*NB + bf2*NB2 + bf1*NB3] = two2buff_switch[jikl];
            // (12 | 43)
            ERI[bf1 + bf2*NB + bf4*NB2 + bf3*NB3] = std::conj(two2buff_switch[jikl]);
            // (43 | 12)
            ERI[bf4 + bf3*NB + bf1*NB2 + bf2*NB3] = std::conj(two2buff_switch[jikl]);



        }; // ijkl loop
      }; // s4
      }; // s3
      }; // s2
      }; // s1
//    }; // omp region

    // Debug output of the ERIs
#ifdef _DEBUGGIAOERI
    std::cout << "Two-Electron GIAO Integrals (GIAO ERIs)" << std::endl;
    for(auto k = 0ul; k < NB; k++)
    for(auto l = 0ul; l < NB; l++)
    for(auto i = 0ul; i < NB; i++)
    for(auto j = 0ul; j < NB; j++){
      std::cout << "(" << i << "," << j << "|" << k << "," << l << ")  ";
      std::cout <<std::setprecision(12)<< ERI[i + j*NB  + k*NB2 + l*NB3] << std::endl;
    };
#endif
  }; // AOIntegrals<dcomplex>::computeGIAOERI


  template <typename IntsT>
  void AOIntegrals<IntsT>::computeERI(EMPerturbation &emPert) {
    if(basisSet_.basisType == REAL_GTO) computeERIGTO();
    else if (basisSet_.basisType == COMPLEX_GIAO) computeERIGIAO(emPert);
  };
  template void AOIntegrals<double>::computeERI(EMPerturbation&);
  template void AOIntegrals<dcomplex>::computeERI(EMPerturbation&);


  /**
   *  \brief Allocate and evaluate the Schwartz bounds over the
   *  CGTO shell pairs.
   */ 
  template <typename IntsT>
  void AOIntegrals<IntsT>::computeSchwartz() {

    if( schwartz != nullptr ) memManager_.free(schwartz);

    // Allocate the schwartz tensor
    schwartz = memManager_.malloc<double>(basisSet_.nShell*basisSet_.nShell);

    // Define the libint2 integral engine
    libint2::Engine engine(libint2::Operator::coulomb,
      basisSet_.maxPrim,basisSet_.maxL,0);

    engine.set_precision(0.); // Don't screen prims during evaluation

    const auto &buf_vec = engine.results();

    auto topSch = std::chrono::high_resolution_clock::now();
  
    size_t n1,n2;
    for(auto s1(0ul); s1 < basisSet_.nShell; s1++) {
      n1 = basisSet_.shells[s1].size(); // Size shell 1
    for(auto s2(0ul); s2 <= s1; s2++) {
      n2 = basisSet_.shells[s2].size(); // Size shell 2



      // Evaluate the shell quartet (s1 s2 | s1 s2)
      engine.compute(
        basisSet_.shells[s1],
        basisSet_.shells[s2],
        basisSet_.shells[s1],
        basisSet_.shells[s2]
      );

      if(buf_vec[0] == nullptr) continue;

      // Allocate space to hold the diagonals
      double* diags = memManager_.malloc<double>(n1*n2);

      for(auto i(0), ij(0); i < n1; i++)
      for(auto j(0); j < n2; j++, ij++)
        diags[i + j*n1] = buf_vec[0][ij*n1*n2 + ij];


      schwartz[s1 + s2*basisSet_.nShell] = 
        std::sqrt(MatNorm<double>('I',n1,n2,diags,n1));

      // Free up space
      memManager_.free(diags);

    } // loop s2
    } // loop s1

    auto botSch = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> durSch = botSch - topSch;

    HerMat('L',basisSet_.nShell,schwartz,basisSet_.nShell);

#if 0
    prettyPrintSmart(std::cout,"Schwartz",schwartz,basisSet_.nShell,
      basisSet_.nShell,basisSet_.nShell);
#endif

  }; // AOIntegrals<IntsT>::computeSchwartz

  template void AOIntegrals<double>::computeSchwartz();
  template void AOIntegrals<dcomplex>::computeSchwartz();

}; // namespace ChronusQ

//#endif
