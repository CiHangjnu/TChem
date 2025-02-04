/* =====================================================================================
TChem version 2.0
Copyright (2020) NTESS
https://github.com/sandialabs/TChem

Copyright 2020 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
Under the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
certain rights in this software.

This file is part of TChem. TChem is open source software: you can redistribute it
and/or modify it under the terms of BSD 2-Clause License
(https://opensource.org/licenses/BSD-2-Clause). A copy of the licese is also
provided under the main directory

Questions? Contact Cosmin Safta at <csafta@sandia.gov>, or
           Kyungjoo Kim at <kyukim@sandia.gov>, or
           Oscar Diaz-Ibarra at <odiazib@sandia.gov>

Sandia National Laboratories, Livermore, CA, USA
===================================================================================== */
#include "TChem_Util.hpp"

#include "TChem_Impl_KForwardReverseSurface.hpp"

#include "TChem_KForwardReverseSurface.hpp"

namespace TChem {

void
KForwardReverseSurface::runDeviceBatch( /// input
  const ordinal_type nBatch,
  const real_type_2d_view_type& state,
  /// input
  const real_type_2d_view_type& gk,
  const real_type_2d_view_type& gkSurf,
  /// output
  const real_type_2d_view_type& kfor,
  const real_type_2d_view_type& krev,
  /// const data from kinetic model
  const kinetic_model_type& kmcd,
  /// const data from kinetic model surface
  const kinetic_surf_model_type& kmcdSurf)
{
  Kokkos::Profiling::pushRegion(
    "TChem::KForwardReverseSurface::runDeviceBatch");
  using policy_type = Kokkos::TeamPolicy<exec_space>;
  using KForwardReverseSurface = Impl::KForwardReverseSurface<real_type,device_type>;

  const ordinal_type level = 1;

  // policy_type policy(nBatch); // error
  policy_type policy(nBatch, Kokkos::AUTO()); // fine
  // policy_type policy(nBatch, Kokkos::AUTO(), Kokkos::AUTO()); // error
  policy.set_scratch_size(level, Kokkos::PerTeam(0));
  Kokkos::parallel_for(
    "TChem::KForwardReverseSurface::runDeviceBatch",
    policy,
    KOKKOS_LAMBDA(const typename policy_type::member_type& member) {
      const ordinal_type i = member.league_rank();
      const real_type_1d_view state_at_i =
        Kokkos::subview(state, i, Kokkos::ALL());

      const real_type_1d_view gk_at_i = Kokkos::subview(gk, i, Kokkos::ALL());
      const real_type_1d_view gkSurf_at_i =
        Kokkos::subview(gkSurf, i, Kokkos::ALL());

      const real_type_1d_view kfor_at_i =
        Kokkos::subview(kfor, i, Kokkos::ALL());
      const real_type_1d_view krev_at_i =
        Kokkos::subview(krev, i, Kokkos::ALL());

      const Impl::StateVector<real_type_1d_view> sv_at_i(kmcd.nSpec,
                                                         state_at_i);
      TCHEM_CHECK_ERROR(!sv_at_i.isValid(),
                        "Error: input state vector is not valid");
      {
        const real_type t = sv_at_i.Temperature();
        const real_type p = sv_at_i.Pressure();
        const real_type_1d_view Xc = sv_at_i.MassFractions();

        KForwardReverseSurface ::team_invoke(member,
                                                   // inputs
                                                   t,
                                                   p,
                                                   gk_at_i,
                                                   gkSurf_at_i,
                                                   // outputs
                                                   kfor_at_i,
                                                   krev_at_i,
                                                   kmcd,
                                                   kmcdSurf);
      }
    });

  Kokkos::Profiling::popRegion();
}

} // namespace TChem
