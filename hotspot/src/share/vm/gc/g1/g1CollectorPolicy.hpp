/*
 * Copyright (c) 2001, 2016, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_VM_GC_G1_G1COLLECTORPOLICY_HPP
#define SHARE_VM_GC_G1_G1COLLECTORPOLICY_HPP

#include "gc/g1/collectionSetChooser.hpp"
#include "gc/g1/g1CollectorState.hpp"
#include "gc/g1/g1GCPhaseTimes.hpp"
#include "gc/g1/g1InCSetState.hpp"
#include "gc/g1/g1InitialMarkToMixedTimeTracker.hpp"
#include "gc/g1/g1MMUTracker.hpp"
#include "gc/g1/g1Predictions.hpp"
#include "gc/shared/collectorPolicy.hpp"
#include "utilities/pair.hpp"

// A G1CollectorPolicy makes policy decisions that determine the
// characteristics of the collector.  Examples include:
//   * choice of collection set.
//   * when to collect.

class HeapRegion;
class CollectionSetChooser;
class G1IHOPControl;
class G1YoungGenSizer;

class G1CollectorPolicy: public CollectorPolicy {
 private:
  G1IHOPControl* _ihop_control;

  G1IHOPControl* create_ihop_control() const;
  // Update the IHOP control with necessary statistics.
  void update_ihop_prediction(double mutator_time_s,
                              size_t mutator_alloc_bytes,
                              size_t young_gen_size);
  void report_ihop_statistics();

  G1Predictions _predictor;

  double get_new_prediction(TruncatedSeq const* seq) const;
  size_t get_new_size_prediction(TruncatedSeq const* seq) const;

  G1MMUTracker* _mmu_tracker;

  void initialize_alignments();
  void initialize_flags();

  CollectionSetChooser* _cset_chooser;

  double _full_collection_start_sec;

  // These exclude marking times.
  TruncatedSeq* _recent_gc_times_ms;

  TruncatedSeq* _concurrent_mark_remark_times_ms;
  TruncatedSeq* _concurrent_mark_cleanup_times_ms;

  // Ratio check data for determining if heap growth is necessary.
  uint _ratio_over_threshold_count;
  double _ratio_over_threshold_sum;
  uint _pauses_since_start;

  uint _young_list_target_length;
  uint _young_list_fixed_length;

  // The max number of regions we can extend the eden by while the GC
  // locker is active. This should be >= _young_list_target_length;
  uint _young_list_max_length;

  SurvRateGroup* _short_lived_surv_rate_group;
  SurvRateGroup* _survivor_surv_rate_group;
  // add here any more surv rate groups

  double _gc_overhead_perc;

  double _reserve_factor;
  uint   _reserve_regions;

  enum PredictionConstants {
    TruncatedSeqLength = 10,
    NumPrevPausesForHeuristics = 10,
    // MinOverThresholdForGrowth must be less than NumPrevPausesForHeuristics,
    // representing the minimum number of pause time ratios that exceed
    // GCTimeRatio before a heap expansion will be triggered.
    MinOverThresholdForGrowth = 4
  };

  TruncatedSeq* _alloc_rate_ms_seq;
  double        _prev_collection_pause_end_ms;

  TruncatedSeq* _rs_length_diff_seq;
  TruncatedSeq* _cost_per_card_ms_seq;
  TruncatedSeq* _cost_scan_hcc_seq;
  TruncatedSeq* _young_cards_per_entry_ratio_seq;
  TruncatedSeq* _mixed_cards_per_entry_ratio_seq;
  TruncatedSeq* _cost_per_entry_ms_seq;
  TruncatedSeq* _mixed_cost_per_entry_ms_seq;
  TruncatedSeq* _cost_per_byte_ms_seq;
  TruncatedSeq* _constant_other_time_ms_seq;
  TruncatedSeq* _young_other_cost_per_region_ms_seq;
  TruncatedSeq* _non_young_other_cost_per_region_ms_seq;

  TruncatedSeq* _pending_cards_seq;
  TruncatedSeq* _rs_lengths_seq;

  TruncatedSeq* _cost_per_byte_ms_during_cm_seq;

  G1YoungGenSizer* _young_gen_sizer;

  uint _eden_cset_region_length;
  uint _survivor_cset_region_length;
  uint _old_cset_region_length;

  void init_cset_region_lengths(uint eden_cset_region_length,
                                uint survivor_cset_region_length);

  uint eden_cset_region_length() const     { return _eden_cset_region_length;     }
  uint survivor_cset_region_length() const { return _survivor_cset_region_length; }
  uint old_cset_region_length() const      { return _old_cset_region_length;      }

  uint _free_regions_at_end_of_collection;

  size_t _recorded_rs_lengths;
  size_t _max_rs_lengths;

  size_t _rs_lengths_prediction;

#ifndef PRODUCT
  bool verify_young_ages(HeapRegion* head, SurvRateGroup *surv_rate_group);
#endif // PRODUCT

  void adjust_concurrent_refinement(double update_rs_time,
                                    double update_rs_processed_buffers,
                                    double goal_ms);

  double _pause_time_target_ms;

  size_t _pending_cards;

  // The amount of allocated bytes in old gen during the last mutator and the following
  // young GC phase.
  size_t _bytes_allocated_in_old_since_last_gc;

  G1InitialMarkToMixedTimeTracker _initial_mark_to_mixed;
public:
  const G1Predictions& predictor() const { return _predictor; }

  // Add the given number of bytes to the total number of allocated bytes in the old gen.
  void add_bytes_allocated_in_old_since_last_gc(size_t bytes) { _bytes_allocated_in_old_since_last_gc += bytes; }

  // Accessors

  void set_region_eden(HeapRegion* hr, int young_index_in_cset) {
    hr->set_eden();
    hr->install_surv_rate_group(_short_lived_surv_rate_group);
    hr->set_young_index_in_cset(young_index_in_cset);
  }

  void set_region_survivor(HeapRegion* hr, int young_index_in_cset) {
    assert(hr->is_survivor(), "pre-condition");
    hr->install_surv_rate_group(_survivor_surv_rate_group);
    hr->set_young_index_in_cset(young_index_in_cset);
  }

#ifndef PRODUCT
  bool verify_young_ages();
#endif // PRODUCT

  void record_max_rs_lengths(size_t rs_lengths) {
    _max_rs_lengths = rs_lengths;
  }

  size_t predict_rs_length_diff() const;

  double predict_alloc_rate_ms() const;

  double predict_cost_per_card_ms() const;

  double predict_scan_hcc_ms() const;

  double predict_rs_update_time_ms(size_t pending_cards) const;

  double predict_young_cards_per_entry_ratio() const;

  double predict_mixed_cards_per_entry_ratio() const;

  size_t predict_young_card_num(size_t rs_length) const;

  size_t predict_non_young_card_num(size_t rs_length) const;

  double predict_rs_scan_time_ms(size_t card_num) const;

  double predict_mixed_rs_scan_time_ms(size_t card_num) const;

  double predict_object_copy_time_ms_during_cm(size_t bytes_to_copy) const;

  double predict_object_copy_time_ms(size_t bytes_to_copy) const;

  double predict_constant_other_time_ms() const;

  double predict_young_other_time_ms(size_t young_num) const;

  double predict_non_young_other_time_ms(size_t non_young_num) const;

  double predict_base_elapsed_time_ms(size_t pending_cards) const;
  double predict_base_elapsed_time_ms(size_t pending_cards,
                                      size_t scanned_cards) const;
  size_t predict_bytes_to_copy(HeapRegion* hr) const;
  double predict_region_elapsed_time_ms(HeapRegion* hr, bool for_young_gc) const;

  void set_recorded_rs_lengths(size_t rs_lengths);

  uint cset_region_length() const       { return young_cset_region_length() +
                                           old_cset_region_length(); }
  uint young_cset_region_length() const { return eden_cset_region_length() +
                                           survivor_cset_region_length(); }

  double predict_survivor_regions_evac_time() const;

  bool should_update_surv_rate_group_predictors() {
    return collector_state()->last_gc_was_young() && !collector_state()->in_marking_window();
  }

  void cset_regions_freed() {
    bool update = should_update_surv_rate_group_predictors();

    _short_lived_surv_rate_group->all_surviving_words_recorded(update);
    _survivor_surv_rate_group->all_surviving_words_recorded(update);
  }

  G1MMUTracker* mmu_tracker() {
    return _mmu_tracker;
  }

  const G1MMUTracker* mmu_tracker() const {
    return _mmu_tracker;
  }

  double max_pause_time_ms() const {
    return _mmu_tracker->max_gc_time() * 1000.0;
  }

  double predict_remark_time_ms() const;

  double predict_cleanup_time_ms() const;

  // Returns an estimate of the survival rate of the region at yg-age
  // "yg_age".
  double predict_yg_surv_rate(int age, SurvRateGroup* surv_rate_group) const;

  double predict_yg_surv_rate(int age) const;

  double accum_yg_surv_rate_pred(int age) const;

protected:
  virtual double average_time_ms(G1GCPhaseTimes::GCParPhases phase) const;
  virtual double other_time_ms(double pause_time_ms) const;

  double young_other_time_ms() const;
  double non_young_other_time_ms() const;
  double constant_other_time_ms(double pause_time_ms) const;

  CollectionSetChooser* cset_chooser() const {
    return _cset_chooser;
  }

private:
  // Statistics kept per GC stoppage, pause or full.
  TruncatedSeq* _recent_prev_end_times_for_all_gcs_sec;

  // Add a new GC of the given duration and end time to the record.
  void update_recent_gc_times(double end_time_sec, double elapsed_ms);

  // The head of the list (via "next_in_collection_set()") representing the
  // current collection set. Set from the incrementally built collection
  // set at the start of the pause.
  HeapRegion* _collection_set;

  // The number of bytes in the collection set before the pause. Set from
  // the incrementally built collection set at the start of an evacuation
  // pause, and incremented in finalize_old_cset_part() when adding old regions
  // (if any) to the collection set.
  size_t _collection_set_bytes_used_before;

  // The number of bytes copied during the GC.
  size_t _bytes_copied_during_gc;

  // The associated information that is maintained while the incremental
  // collection set is being built with young regions. Used to populate
  // the recorded info for the evacuation pause.

  enum CSetBuildType {
    Active,             // We are actively building the collection set
    Inactive            // We are not actively building the collection set
  };

  CSetBuildType _inc_cset_build_state;

  // The head of the incrementally built collection set.
  HeapRegion* _inc_cset_head;

  // The tail of the incrementally built collection set.
  HeapRegion* _inc_cset_tail;

  // The number of bytes in the incrementally built collection set.
  // Used to set _collection_set_bytes_used_before at the start of
  // an evacuation pause.
  size_t _inc_cset_bytes_used_before;

  // The RSet lengths recorded for regions in the CSet. It is updated
  // by the thread that adds a new region to the CSet. We assume that
  // only one thread can be allocating a new CSet region (currently,
  // it does so after taking the Heap_lock) hence no need to
  // synchronize updates to this field.
  size_t _inc_cset_recorded_rs_lengths;

  // A concurrent refinement thread periodically samples the young
  // region RSets and needs to update _inc_cset_recorded_rs_lengths as
  // the RSets grow. Instead of having to synchronize updates to that
  // field we accumulate them in this field and add it to
  // _inc_cset_recorded_rs_lengths_diffs at the start of a GC.
  ssize_t _inc_cset_recorded_rs_lengths_diffs;

  // The predicted elapsed time it will take to collect the regions in
  // the CSet. This is updated by the thread that adds a new region to
  // the CSet. See the comment for _inc_cset_recorded_rs_lengths about
  // MT-safety assumptions.
  double _inc_cset_predicted_elapsed_time_ms;

  // See the comment for _inc_cset_recorded_rs_lengths_diffs.
  double _inc_cset_predicted_elapsed_time_ms_diffs;

  // Stash a pointer to the g1 heap.
  G1CollectedHeap* _g1;

  G1GCPhaseTimes* _phase_times;

  // The ratio of gc time to elapsed time, computed over recent pauses,
  // and the ratio for just the last pause.
  double _recent_avg_pause_time_ratio;
  double _last_pause_time_ratio;

  double recent_avg_pause_time_ratio() const {
    return _recent_avg_pause_time_ratio;
  }

  // This set of variables tracks the collector efficiency, in order to
  // determine whether we should initiate a new marking.
  double _mark_remark_start_sec;
  double _mark_cleanup_start_sec;

  // Updates the internal young list maximum and target lengths. Returns the
  // unbounded young list target length.
  uint update_young_list_max_and_target_length();
  uint update_young_list_max_and_target_length(size_t rs_lengths);

  // Update the young list target length either by setting it to the
  // desired fixed value or by calculating it using G1's pause
  // prediction model. If no rs_lengths parameter is passed, predict
  // the RS lengths using the prediction model, otherwise use the
  // given rs_lengths as the prediction.
  // Returns the unbounded young list target length.
  uint update_young_list_target_length(size_t rs_lengths);

  // Calculate and return the minimum desired young list target
  // length. This is the minimum desired young list length according
  // to the user's inputs.
  uint calculate_young_list_desired_min_length(uint base_min_length) const;

  // Calculate and return the maximum desired young list target
  // length. This is the maximum desired young list length according
  // to the user's inputs.
  uint calculate_young_list_desired_max_length() const;

  // Calculate and return the maximum young list target length that
  // can fit into the pause time goal. The parameters are: rs_lengths
  // represent the prediction of how large the young RSet lengths will
  // be, base_min_length is the already existing number of regions in
  // the young list, min_length and max_length are the desired min and
  // max young list length according to the user's inputs.
  uint calculate_young_list_target_length(size_t rs_lengths,
                                          uint base_min_length,
                                          uint desired_min_length,
                                          uint desired_max_length) const;

  // Result of the bounded_young_list_target_length() method, containing both the
  // bounded as well as the unbounded young list target lengths in this order.
  typedef Pair<uint, uint, StackObj> YoungTargetLengths;
  YoungTargetLengths young_list_target_lengths(size_t rs_lengths) const;

  void update_rs_lengths_prediction();
  void update_rs_lengths_prediction(size_t prediction);

  // Calculate and return chunk size (in number of regions) for parallel
  // concurrent mark cleanup.
  uint calculate_parallel_work_chunk_size(uint n_workers, uint n_regions) const;

  // Check whether a given young length (young_length) fits into the
  // given target pause time and whether the prediction for the amount
  // of objects to be copied for the given length will fit into the
  // given free space (expressed by base_free_regions).  It is used by
  // calculate_young_list_target_length().
  bool predict_will_fit(uint young_length, double base_time_ms,
                        uint base_free_regions, double target_pause_time_ms) const;

  // Calculate the minimum number of old regions we'll add to the CSet
  // during a mixed GC.
  uint calc_min_old_cset_length() const;

  // Calculate the maximum number of old regions we'll add to the CSet
  // during a mixed GC.
  uint calc_max_old_cset_length() const;

  // Returns the given amount of uncollected reclaimable space
  // as a percentage of the current heap capacity.
  double reclaimable_bytes_perc(size_t reclaimable_bytes) const;

  // Sets up marking if proper conditions are met.
  void maybe_start_marking();

  // The kind of STW pause.
  enum PauseKind {
    FullGC,
    YoungOnlyGC,
    MixedGC,
    LastYoungGC,
    InitialMarkGC,
    Cleanup,
    Remark
  };

  // Calculate PauseKind from internal state.
  PauseKind young_gc_pause_kind() const;
  // Record the given STW pause with the given start and end times (in s).
  void record_pause(PauseKind kind, double start, double end);
  // Indicate that we aborted marking before doing any mixed GCs.
  void abort_time_to_mixed_tracking();
public:

  G1CollectorPolicy();

  virtual ~G1CollectorPolicy();

  virtual G1CollectorPolicy* as_g1_policy() { return this; }

  G1CollectorState* collector_state() const;

  G1GCPhaseTimes* phase_times() const { return _phase_times; }

  // Check the current value of the young list RSet lengths and
  // compare it against the last prediction. If the current value is
  // higher, recalculate the young list target length prediction.
  void revise_young_list_target_length_if_necessary(size_t rs_lengths);

  // This should be called after the heap is resized.
  void record_new_heap_size(uint new_number_of_regions);

  void init();

  virtual void note_gc_start(uint num_active_workers);

  // Create jstat counters for the policy.
  virtual void initialize_gc_policy_counters();

  bool need_to_start_conc_mark(const char* source, size_t alloc_word_size = 0);

  bool about_to_start_mixed_phase() const;

  // Record the start and end of an evacuation pause.
  void record_collection_pause_start(double start_time_sec);
  void record_collection_pause_end(double pause_time_ms, size_t cards_scanned, size_t heap_used_bytes_before_gc);

  // Record the start and end of a full collection.
  void record_full_collection_start();
  void record_full_collection_end();

  // Must currently be called while the world is stopped.
  void record_concurrent_mark_init_end(double mark_init_elapsed_time_ms);

  // Record start and end of remark.
  void record_concurrent_mark_remark_start();
  void record_concurrent_mark_remark_end();

  // Record start, end, and completion of cleanup.
  void record_concurrent_mark_cleanup_start();
  void record_concurrent_mark_cleanup_end();
  void record_concurrent_mark_cleanup_completed();

  virtual void print_phases();

  // Record how much space we copied during a GC. This is typically
  // called when a GC alloc region is being retired.
  void record_bytes_copied_during_gc(size_t bytes) {
    _bytes_copied_during_gc += bytes;
  }

  // The amount of space we copied during a GC.
  size_t bytes_copied_during_gc() const {
    return _bytes_copied_during_gc;
  }

  size_t collection_set_bytes_used_before() const {
    return _collection_set_bytes_used_before;
  }

  // Determine whether there are candidate regions so that the
  // next GC should be mixed. The two action strings are used
  // in the ergo output when the method returns true or false.
  bool next_gc_should_be_mixed(const char* true_action_str,
                               const char* false_action_str) const;

  // Choose a new collection set.  Marks the chosen regions as being
  // "in_collection_set", and links them together.  The head and number of
  // the collection set are available via access methods.
  double finalize_young_cset_part(double target_pause_time_ms);
  virtual void finalize_old_cset_part(double time_remaining_ms);

  // The head of the list (via "next_in_collection_set()") representing the
  // current collection set.
  HeapRegion* collection_set() { return _collection_set; }

  void clear_collection_set() { _collection_set = NULL; }

  // Add old region "hr" to the CSet.
  void add_old_region_to_cset(HeapRegion* hr);

  // Incremental CSet Support

  // The head of the incrementally built collection set.
  HeapRegion* inc_cset_head() { return _inc_cset_head; }

  // The tail of the incrementally built collection set.
  HeapRegion* inc_set_tail() { return _inc_cset_tail; }

  // Initialize incremental collection set info.
  void start_incremental_cset_building();

  // Perform any final calculations on the incremental CSet fields
  // before we can use them.
  void finalize_incremental_cset_building();

  void clear_incremental_cset() {
    _inc_cset_head = NULL;
    _inc_cset_tail = NULL;
  }

  // Stop adding regions to the incremental collection set
  void stop_incremental_cset_building() { _inc_cset_build_state = Inactive; }

  // Add information about hr to the aggregated information for the
  // incrementally built collection set.
  void add_to_incremental_cset_info(HeapRegion* hr, size_t rs_length);

  // Update information about hr in the aggregated information for
  // the incrementally built collection set.
  void update_incremental_cset_info(HeapRegion* hr, size_t new_rs_length);

private:
  // Update the incremental cset information when adding a region
  // (should not be called directly).
  void add_region_to_incremental_cset_common(HeapRegion* hr);

  // Set the state to start a concurrent marking cycle and clear
  // _initiate_conc_mark_if_possible because it has now been
  // acted on.
  void initiate_conc_mark();

public:
  // Add hr to the LHS of the incremental collection set.
  void add_region_to_incremental_cset_lhs(HeapRegion* hr);

  // Add hr to the RHS of the incremental collection set.
  void add_region_to_incremental_cset_rhs(HeapRegion* hr);

#ifndef PRODUCT
  void print_collection_set(HeapRegion* list_head, outputStream* st);
#endif // !PRODUCT

  // This sets the initiate_conc_mark_if_possible() flag to start a
  // new cycle, as long as we are not already in one. It's best if it
  // is called during a safepoint when the test whether a cycle is in
  // progress or not is stable.
  bool force_initial_mark_if_outside_cycle(GCCause::Cause gc_cause);

  // This is called at the very beginning of an evacuation pause (it
  // has to be the first thing that the pause does). If
  // initiate_conc_mark_if_possible() is true, and the concurrent
  // marking thread has completed its work during the previous cycle,
  // it will set during_initial_mark_pause() to so that the pause does
  // the initial-mark work and start a marking cycle.
  void decide_on_conc_mark_initiation();

  // If an expansion would be appropriate, because recent GC overhead had
  // exceeded the desired limit, return an amount to expand by.
  virtual size_t expansion_amount();

  // Clear ratio tracking data used by expansion_amount().
  void clear_ratio_check_data();

  // Print stats on young survival ratio
  void print_yg_surv_rate_info() const;

  void finished_recalculating_age_indexes(bool is_survivors) {
    if (is_survivors) {
      _survivor_surv_rate_group->finished_recalculating_age_indexes();
    } else {
      _short_lived_surv_rate_group->finished_recalculating_age_indexes();
    }
    // do that for any other surv rate groups
  }

  size_t young_list_target_length() const { return _young_list_target_length; }

  bool is_young_list_full() const;

  bool can_expand_young_list() const;

  uint young_list_max_length() const {
    return _young_list_max_length;
  }

  bool adaptive_young_list_length() const;

  virtual bool should_process_references() const {
    return true;
  }

private:
  //
  // Survivor regions policy.
  //

  // Current tenuring threshold, set to 0 if the collector reaches the
  // maximum amount of survivors regions.
  uint _tenuring_threshold;

  // The limit on the number of regions allocated for survivors.
  uint _max_survivor_regions;

  // For reporting purposes.
  // The value of _heap_bytes_before_gc is also used to calculate
  // the cost of copying.

  // The amount of survivor regions after a collection.
  uint _recorded_survivor_regions;
  // List of survivor regions.
  HeapRegion* _recorded_survivor_head;
  HeapRegion* _recorded_survivor_tail;

  AgeTable _survivors_age_table;

public:
  uint tenuring_threshold() const { return _tenuring_threshold; }

  uint max_survivor_regions() {
    return _max_survivor_regions;
  }

  static const uint REGIONS_UNLIMITED = (uint) -1;

  uint max_regions(InCSetState dest) const {
    switch (dest.value()) {
      case InCSetState::Young:
        return _max_survivor_regions;
      case InCSetState::Old:
        return REGIONS_UNLIMITED;
      default:
        assert(false, "Unknown dest state: " CSETSTATE_FORMAT, dest.value());
        break;
    }
    // keep some compilers happy
    return 0;
  }

  void note_start_adding_survivor_regions() {
    _survivor_surv_rate_group->start_adding_regions();
  }

  void note_stop_adding_survivor_regions() {
    _survivor_surv_rate_group->stop_adding_regions();
  }

  void record_survivor_regions(uint regions,
                               HeapRegion* head,
                               HeapRegion* tail) {
    _recorded_survivor_regions = regions;
    _recorded_survivor_head    = head;
    _recorded_survivor_tail    = tail;
  }

  uint recorded_survivor_regions() const {
    return _recorded_survivor_regions;
  }

  void record_age_table(AgeTable* age_table) {
    _survivors_age_table.merge(age_table);
  }

  void update_max_gc_locker_expansion();

  // Calculates survivor space parameters.
  void update_survivors_policy();

  virtual void post_heap_initialize();
};

#endif // SHARE_VM_GC_G1_G1COLLECTORPOLICY_HPP
