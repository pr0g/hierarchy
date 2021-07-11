#include "hierarchy/entity.hpp"

#include <benchmark/benchmark.h>



static void expanded_count_1(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  hy::interaction_t interaction;
  for ([[maybe_unused]] auto _ : state) {
    int count = 0;
    hy::expanded_count(root_handles[0], entities, interaction, count);
    benchmark::DoNotOptimize(count);
    benchmark::ClobberMemory();
  }
}

// BENCHMARK(expanded_count_1);

static void expanded_count_2(benchmark::State& state) {
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

// BENCHMARK(expanded_count_2);

static void expanded_count_3(benchmark::State& state) {
  thh::container_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities);

  hy::interaction_t interaction;
  for ([[maybe_unused]] auto _ : state) {
    int count = 0;
    count = hy::expanded_count_again(root_handles[0], entities, interaction);
    benchmark::DoNotOptimize(count);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(expanded_count_3);

BENCHMARK_MAIN();
