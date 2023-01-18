/*

This file is part of VROOM.

Copyright (c) 2015-2022, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include "problems/vrptw/operators/pd_shift.h"
#include "algorithms/local_search/insertion_search.h"
#include "utils/helpers.h"

namespace vroom {
namespace vrptw {

PDShift::PDShift(const Input& input,
                 const utils::SolutionState& sol_state,
                 TWRoute& tw_s_route,
                 Index s_vehicle,
                 Index s_p_rank,
                 Index s_d_rank,
                 TWRoute& tw_t_route,
                 Index t_vehicle,
                 const Eval& gain_threshold)
  : cvrp::PDShift(input,
                  sol_state,
                  static_cast<RawRoute&>(tw_s_route),
                  s_vehicle,
                  s_p_rank,
                  s_d_rank,
                  static_cast<RawRoute&>(tw_t_route),
                  t_vehicle,
                  gain_threshold),
    _source_without_pd(s_route.begin() + _s_p_rank + 1,
                       s_route.begin() + _s_d_rank),
    _tw_s_route(tw_s_route),
    _tw_t_route(tw_t_route) {
}

void PDShift::compute_gain() {
  // Check for valid removal wrt TW constraints.
  bool is_valid_removal =
    _tw_s_route.is_valid_addition_for_tw(_input,
                                         _input.zero_amount(),
                                         _source_without_pd.begin(),
                                         _source_without_pd.end(),
                                         _s_p_rank,
                                         _s_d_rank + 1);
  if (!is_valid_removal) {
    return;
  }

  ls::RouteInsertion rs = ls::compute_best_insertion_pd(_input,
                                                        _sol_state,
                                                        s_route[_s_p_rank],
                                                        t_vehicle,
                                                        _tw_t_route,
                                                        s_gain - stored_gain);

  if (rs.eval != NO_EVAL) {
    _valid = true;
    t_gain -= rs.eval;
    stored_gain = s_gain + t_gain;
    _best_t_p_rank = rs.pickup_rank;
    _best_t_d_rank = rs.delivery_rank;
    _best_t_delivery = rs.delivery;
  }
  gain_computed = true;
}

void PDShift::apply() {
  std::vector<Index> target_with_pd({s_route[_s_p_rank]});
  std::copy(t_route.begin() + _best_t_p_rank,
            t_route.begin() + _best_t_d_rank,
            std::back_inserter(target_with_pd));
  target_with_pd.push_back(s_route[_s_d_rank]);

  _tw_t_route.replace(_input,
                      _best_t_delivery,
                      target_with_pd.begin(),
                      target_with_pd.end(),
                      _best_t_p_rank,
                      _best_t_d_rank);

  if (_s_d_rank == _s_p_rank + 1) {
    _tw_s_route.remove(_input, _s_p_rank, 2);
  } else {
    Amount delivery = _input.zero_amount();
    for (unsigned i = _s_p_rank + 1; i < _s_d_rank; ++i) {
      const auto& job = _input.jobs[s_route[i]];
      if (job.type == JOB_TYPE::SINGLE) {
        delivery += job.delivery;
      }
    }
    _tw_s_route.replace(_input,
                        delivery,
                        _source_without_pd.begin(),
                        _source_without_pd.end(),
                        _s_p_rank,
                        _s_d_rank + 1);
  }
}

} // namespace vrptw
} // namespace vroom
