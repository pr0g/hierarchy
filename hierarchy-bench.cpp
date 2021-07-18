#include "hierarchy/entity.hpp"

#include <benchmark/benchmark.h>

static void expanded_count(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  hy::collapser_t collapser;
  for ([[maybe_unused]] auto _ : state) {
    int count = 0;
    count = hy::expanded_count(root_handles[0], entities, collapser);
    benchmark::DoNotOptimize(count);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(expanded_count);

static void create_entities(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  for ([[maybe_unused]] auto _ : state) {
    auto root_handles = demo::create_bench_entities(entities);
    benchmark::DoNotOptimize(root_handles);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(create_entities);

static void flatten_entities_expanded(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);
  hy::collapser_t collapser;
  for ([[maybe_unused]] auto _ : state) {
    auto flattened = hy::build_vector(entities, collapser, root_handles);
    benchmark::DoNotOptimize(flattened);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(flatten_entities_expanded);

static void flatten_entities_collapsed(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);
  hy::collapser_t collapser;
  for (const auto& handle : root_handles) {
    collapser.collapse(handle, entities);
  }
  for ([[maybe_unused]] auto _ : state) {
    auto flattened = hy::build_vector(entities, collapser, root_handles);
    benchmark::DoNotOptimize(flattened);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(flatten_entities_collapsed);

static void expand_entity(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  hy::collapser_t collapser;
  auto flattened = hy::build_vector(entities, collapser, root_handles);

  for ([[maybe_unused]] auto _ : state) {
    // expand/collapse
  }
}

BENCHMARK(expand_entity);

BENCHMARK_MAIN();
