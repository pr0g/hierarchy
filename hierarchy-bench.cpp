#include "hierarchy/entity.hpp"

#include <benchmark/benchmark.h>

static void expanded_count(benchmark::State& state) {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities, 1, 1000000);

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
  thh::handle_vector_t<hy::entity_t> entities;
  for ([[maybe_unused]] auto _ : state) {
    auto root_handles = demo::create_bench_entities(entities, 1, 1000000);
    benchmark::DoNotOptimize(root_handles);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(create_entities);

static void flatten_entities_expanded(benchmark::State& state) {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities, 1, 1000000);
  hy::collapser_t collapser;
  for ([[maybe_unused]] auto _ : state) {
    auto flattened = hy::flatten_entities(entities, collapser, root_handles);
    benchmark::DoNotOptimize(flattened);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(flatten_entities_expanded);

static void flatten_entities_collapsed(benchmark::State& state) {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities, 1, 1000000);
  hy::collapser_t collapser;
  for (const auto& handle : root_handles) {
    collapser.collapse(handle, entities);
  }
  for ([[maybe_unused]] auto _ : state) {
    auto flattened = hy::flatten_entities(entities, collapser, root_handles);
    benchmark::DoNotOptimize(flattened);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(flatten_entities_collapsed);

static void expand_entity(benchmark::State& state) {
  thh::handle_vector_t<hy::entity_t> entities;
  auto root_handles = demo::create_bench_entities(entities, 1, state.range(0));

  hy::collapser_t collapser;
  for (const auto& handle : root_handles) {
    collapser.collapse(handle, entities);
  }

  hy::view_t view(
    hy::flatten_entities(entities, collapser, root_handles), 0, 20);

  for ([[maybe_unused]] auto _ : state) {
    view.expand(entities, collapser);
    benchmark::DoNotOptimize(view);
    benchmark::ClobberMemory();
  }
}

BENCHMARK(expand_entity)->Range(100, 1<<22);

BENCHMARK_MAIN();
