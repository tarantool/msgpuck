#include <benchmark/benchmark.h>
#include <random>
#include <functional>

#include "msgpuck.h"

#define DATA_SIZE (128 * 1024 * 1024)

static auto generator = std::mt19937_64{};

struct data_set {
	char *data;
	char *end;
	char *buf_end;
	int count;
};

static void
data_set_create(struct data_set *set)
{
	set->data = (char *)malloc(DATA_SIZE);
	set->buf_end = set->data + DATA_SIZE;
	/* No any data on creation. */
	set->end = set->data;
	set->count = 0;
}

static void
data_set_destroy(struct data_set *set)
{
	free(set->data);
}

template<class Generator>
static void
data_set_reset(struct data_set *set, Generator gen)
{
	set->count = 0;
	set->end = set->data;
	while (true) {
		size_t size = gen(set->end, set->buf_end);
		if (size == 0)
			break;
		set->end += size;
		set->count++;
	}
}

static uint64_t
uint_max(int bits)
{
	if (bits == 64)
		return UINT64_MAX;
	else
		return (1ULL << bits) - 1;
}

template<class UintDist>
static size_t
mp_uint_generate(char *buf, char *buf_end, UintDist dist)
{
	uint64_t v = dist(generator);
	size_t size = mp_sizeof_uint(v);
	if (buf + size > buf_end)
		return 0;
	mp_encode_uint(buf, v);
	return size;
}

static void
bench_mp_next_uint(benchmark::State& state)
{
	using namespace std::placeholders;
	struct data_set set;
	data_set_create(&set);

	/** The test parametrized by number of bits in UINT. */
	int bits = state.range(0);
	auto dist = std::uniform_int_distribution<uint64_t>(0, uint_max(bits));
	data_set_reset(&set,
		       std::bind(mp_uint_generate<decltype(dist)>, _1, _2,
			         dist));

	for (auto _ : state) {
		const char *d = set.data;
		for (int i = 0; i < set.count; i++)
			mp_next(&d);
	}
	state.SetItemsProcessed(state.iterations() * set.count);

	data_set_destroy(&set);
}

static void
bench_mp_check_uint(benchmark::State& state)
{
	using namespace std::placeholders;
	struct data_set set;
	data_set_create(&set);

	/** The test parametrized by number of bits in UINT. */
	int bits = state.range(0);
	auto dist = std::uniform_int_distribution<uint64_t>(0, uint_max(bits));
	data_set_reset(&set,
		       std::bind(mp_uint_generate<decltype(dist)>, _1, _2,
			         dist));

	for (auto _ : state) {
		const char *d = set.data;
		for (int i = 0; i < set.count; i++) {
			if (mp_check(&d, set.end) != 0)
				abort();
		}
	}
	state.SetItemsProcessed(state.iterations() * set.count);

	data_set_destroy(&set);
}

BENCHMARK(bench_mp_next_uint)->Arg(7)->Arg(16)->Arg(32)->Arg(64);
BENCHMARK(bench_mp_check_uint)->Arg(7)->Arg(16)->Arg(32)->Arg(64);

BENCHMARK_MAIN();
