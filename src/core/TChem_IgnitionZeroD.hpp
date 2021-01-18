#ifndef __TCHEM_IGNITION_ZEROD_HPP__
#define __TCHEM_IGNITION_ZEROD_HPP__

#include "TChem_KineticModelData.hpp"
#include "TChem_Util.hpp"

#include "TChem_Impl_IgnitionZeroD.hpp"

namespace TChem {

struct IgnitionZeroD
{
  template<typename KineticModelConstDataType>
  static inline ordinal_type getWorkSpaceSize(
    const KineticModelConstDataType& kmcd)
  {
    return (Impl::IgnitionZeroD::getWorkSpaceSize(kmcd) +
            /// value array (in/out)
            Impl::IgnitionZeroD_Problem<
              KineticModelConstDataType>::getNumberOfEquations(kmcd));
  }

  /// tadv - an input structure for time marching
  /// state (nSpec+3) - initial condition of the state vector
  /// work - work space sized by getWorkSpaceSize
  /// t_out - time when this code exits
  /// state_out - final condition of the state vector (the same input state can
  /// be overwritten) kmcd - const data for kinetic model
  static void runHostBatch( /// input
    typename UseThisTeamPolicy<host_exec_space>::type& policy,
    /// global tolerence parameters that governs all samples
    const real_type_1d_view_host& tol_newton,
    const real_type_2d_view_host& tol_time,
    /// sample specific input
    const real_type_2d_view_host& fac,
    const time_advance_type_1d_view_host& tadv,
    const real_type_2d_view_host& state,
    /// output
    const real_type_1d_view_host& t_out,
    const real_type_1d_view_host& dt_out,
    const real_type_2d_view_host& state_out,
    /// const data from kinetic model
    const KineticModelConstDataHost& kmcd);

  static void runDeviceBatch( /// thread block size
    typename UseThisTeamPolicy<exec_space>::type& policy,
    /// global tolerence parameters that governs all samples
    const real_type_1d_view& tol_newton,
    const real_type_2d_view& tol_time,
    /// sample specific input
    const real_type_2d_view& fac,
    const time_advance_type_1d_view& tadv,
    const real_type_2d_view& state,
    /// output
    const real_type_1d_view& t_out,
    const real_type_1d_view& dt_out,
    const real_type_2d_view& state_out,
    /// const data from kinetic model
    const KineticModelConstDataDevice& kmcd);

  static void runHostBatch( /// input
    typename UseThisTeamPolicy<host_exec_space>::type& policy,
    /// global tolerence parameters that governs all samples
    const real_type_1d_view_host& tol_newton,
    const real_type_2d_view_host& tol_time,
    /// sample specific input
    const real_type_2d_view_host& fac,
    const time_advance_type_1d_view_host& tadv,
    const real_type_2d_view_host& state,
    /// output
    const real_type_1d_view_host& t_out,
    const real_type_1d_view_host& dt_out,
    const real_type_2d_view_host& state_out,
    /// const data from kinetic model
    const Kokkos::View<KineticModelConstDataHost*,host_exec_space>& kmcds);

  static void runDeviceBatch( /// thread block size
    typename UseThisTeamPolicy<exec_space>::type& policy,
    /// global tolerence parameters that governs all samples
    const real_type_1d_view& tol_newton,
    const real_type_2d_view& tol_time,
    /// sample specific input
    const real_type_2d_view& fac,
    const time_advance_type_1d_view& tadv,
    const real_type_2d_view& state,
    /// output
    const real_type_1d_view& t_out,
    const real_type_1d_view& dt_out,
    const real_type_2d_view& state_out,
    /// const data from kinetic model
    const Kokkos::View<KineticModelConstDataDevice*,exec_space>& kmcds);

};

} // namespace TChem

#endif
