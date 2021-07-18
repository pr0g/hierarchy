#include "hierarchy/entity.hpp"

#include <benchmark/benchmark.h>

static void expanded_count_3(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  hy::interaction_t interaction;
  for ([[maybe_unused]] auto _ : state) {
    int count = 0;
    count = hy::expanded_count(root_handles[0], entities, interaction);
    benchmark::DoNotOptimize(count);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(expanded_count_3);

static void create_entities(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  hy::interaction_t interaction;
  for ([[maybe_unused]] auto _ : state) {
    auto root_handles = demo::create_bench_entities(entities);
    benchmark::DoNotOptimize(root_handles);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(create_entities);

static void flatten_entities(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);
  hy::interaction_t interaction;
  for ([[maybe_unused]] auto _ : state) {
    auto flattened = hy::build_vector(entities, interaction, root_handles);
    benchmark::DoNotOptimize(flattened);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(flatten_entities);

static void expand_entity(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  hy::interaction_t interaction;
  auto flattened = hy::build_vector(entities, interaction, root_handles);

  for ([[maybe_unused]] auto _ : state) {
    // expand/collapse
  }
}

BENCHMARK(expand_entity);

BENCHMARK_MAIN();
