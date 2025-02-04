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
#include "TChem_SourceTerm.hpp"

namespace TChem {

  template<typename PolicyType,
           typename DeviceType>
  void
  SourceTerm_TemplateRun(const std::string& profile_name,
                         const PolicyType& policy,                         
                         /// input
                         const Tines::value_type_2d_view<real_type, DeviceType>& state,
                         /// output
                         const Tines::value_type_2d_view<real_type, DeviceType>& source_term,
                         /// workspace
                         const Tines::value_type_2d_view<real_type, DeviceType>& workspace,
                         /// const data from kinetic model
                         const KineticModelConstData<DeviceType >& kmcd)
  {
    Kokkos::Profiling::pushRegion(profile_name);
    using policy_type = PolicyType;
    using device_type = DeviceType;
    using range_type = Kokkos::pair<ordinal_type,ordinal_type>;
    using real_type_1d_view_type = Tines::value_type_1d_view<real_type, device_type>;

    const ordinal_type level = 1;
    const ordinal_type per_team_extent = TChem::SourceTerm::getWorkSpaceSize(kmcd);
    const ordinal_type n = state.extent(0);

    if (workspace.span()) {
      TCHEM_CHECK_ERROR(workspace.extent(0) < policy.league_size(), "Workspace is allocated smaller than the league size");
      TCHEM_CHECK_ERROR(workspace.extent(1) < per_team_extent, "Workspace is allocated smaller than the required");
    }
    
    Kokkos::parallel_for
      (profile_name,
       policy,
       KOKKOS_LAMBDA(const typename policy_type::member_type& member) {
        real_type_1d_view_type work;
        Scratch<real_type_1d_view_type> swork;
        if (workspace.span()) {
          work = Kokkos::subview(workspace, member.league_rank(), Kokkos::ALL());
        } else {
          /// assume that the workspace is given from scratch space
          swork = Scratch<real_type_1d_view_type>(member.team_scratch(level), per_team_extent);
          work = real_type_1d_view_type(swork.data(), swork.span());
        }

        ordinal_type ibeg(0), iend(0), iinc(0);
        Impl::getLeagueRange(member, n, ibeg, iend, iinc);
        for (ordinal_type i=ibeg;i<iend;i+=iinc) {
          const real_type_1d_view_type state_at_i = Kokkos::subview(state, i, Kokkos::ALL());
          const real_type_1d_view_type source_term_at_i = Kokkos::subview(source_term, i, Kokkos::ALL());
          const Impl::StateVector<real_type_1d_view_type> sv_at_i(kmcd.nSpec, state_at_i);
          TCHEM_CHECK_ERROR(!sv_at_i.isValid(), "Error: input state vector is not valid");
          {
            const auto t = sv_at_i.Temperature();
            const auto p = sv_at_i.Pressure();
            const real_type_1d_view_type Ys = sv_at_i.MassFractions();
            
            Impl::SourceTerm<real_type,device_type>
              ::team_invoke(member, t, p, Ys, source_term_at_i, work, kmcd);
          }
        }
      });
    Kokkos::Profiling::popRegion();
  }
  
  void
  SourceTerm::runDeviceBatch( /// thread block size
                             typename UseThisTeamPolicy<exec_space>::type& policy,
                             const real_type_2d_view_type& state,
                             const real_type_2d_view_type& source_term,
                             const real_type_2d_view_type& workspace,
                             const kinetic_model_type& kmcd)
  {
    SourceTerm_TemplateRun("TChem::SourceTerm::runDeviceBatch",
                           policy,
                           state,
                           source_term,
                           workspace,
                           kmcd);
  }
  
  void
  SourceTerm::runHostBatch( /// thread block size
                           typename UseThisTeamPolicy<host_exec_space>::type& policy,  
                           const real_type_2d_view_host_type& state,
                           const real_type_2d_view_host_type& source_term,
                           const real_type_2d_view_host_type& workspace,
                           const kinetic_model_host_type& kmcd)
  {
    SourceTerm_TemplateRun("TChem::SourceTerm::runHostBatch",
                           policy,
                           state,
                           source_term,
                           workspace,
                           kmcd);
  }

  void
  SourceTerm::runDeviceBatch( /// thread block size
                             const real_type_2d_view_type& state,
                             const real_type_2d_view_type& source_term,
                             const real_type_2d_view_type& workspace,
                             const kinetic_model_type& kmcd)
  {
    // setting policy
    const auto exec_space_instance = TChem::exec_space();
    using policy_type = typename TChem::UseThisTeamPolicy<TChem::exec_space>::type;
    const ordinal_type level = 1;
    const ordinal_type per_team_extent = TChem::SourceTerm::getWorkSpaceSize(kmcd); 
    const ordinal_type per_team_scratch = Scratch<real_type_1d_view_type>::shmem_size(per_team_extent);

    policy_type policy(exec_space_instance, state.extent(0), Kokkos::AUTO());
    if (!workspace.span()) 
      policy.set_scratch_size(level, Kokkos::PerTeam(per_team_scratch));

    SourceTerm_TemplateRun("TChem::SourceTerm::runDeviceBatch",
                           policy,
                           state,
                           source_term,
                           workspace,
                           kmcd);
  }

  void
  SourceTerm::runHostBatch( /// thread block size
                           const real_type_2d_view_host_type& state,
                           const real_type_2d_view_host_type& source_term,
                           const real_type_2d_view_host_type& workspace,
                           const kinetic_model_host_type& kmcd)
  {
    // setting policy
    const auto exec_space_instance = TChem::host_exec_space();
    using policy_type = typename TChem::UseThisTeamPolicy<TChem::host_exec_space>::type;
    const ordinal_type level = 1;
    const ordinal_type per_team_extent = TChem::SourceTerm::getWorkSpaceSize(kmcd); ///
    const ordinal_type per_team_scratch = Scratch<real_type_1d_view_host_type>::shmem_size(per_team_extent);

    policy_type policy(exec_space_instance, state.extent(0), Kokkos::AUTO());
    if (!workspace.span()) 
      policy.set_scratch_size(level, Kokkos::PerTeam(per_team_scratch));
    
    SourceTerm_TemplateRun("TChem::SourceTerm::runDeviceBatch",
                           policy,
                           state,
                           source_term,
                           workspace,
                           kmcd);
  }

} // namespace TChem

