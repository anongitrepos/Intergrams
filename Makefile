CXXFLAGS = -std=c++20 -march=native -O3 -DNDEBUG -mavx2 -ffast-math -funroll-loops -I/your/path/to/library

LDFLAGS = -L/your/path/to/link -larmadillo
CXX = g++-12

all: test_chunk_reader compute_ngrams_individual_chunks compute_ngrams_lockstep compute_ngrams_lockstep_stealing compute_ngrams_naive_parallel compute_ref_3grams compute_ngrams_pool_parallel compute_ngrams_group_parallel compute_ngrams_full

test_chunk_reader: src/test_chunk_reader.cpp
	$(CXX) $(CXXFLAGS) -o test_chunk_reader src/test_chunk_reader.cpp $(LDFLAGS)

compute_ngrams_individual_chunks: src/compute_ngrams_individual_chunks.cpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_individual_chunks src/compute_ngrams_individual_chunks.cpp $(LDFLAGS)

compute_ngrams_lockstep: src/compute_ngrams_lockstep.cpp src/lockstep_reader_thread.hpp src/lockstep_reader_thread_impl.hpp src/ngram_lockstep_thread.hpp src/ngram_lockstep_thread_impl.hpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_lockstep src/compute_ngrams_lockstep.cpp $(LDFLAGS)

compute_ngrams_lockstep_stealing: src/compute_ngrams_lockstep_stealing.cpp src/lockstep_stealing_reader_thread.hpp src/lockstep_stealing_reader_thread_impl.hpp src/ngram_lockstep_thread.hpp src/ngram_lockstep_thread_impl.hpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_lockstep_stealing src/compute_ngrams_lockstep_stealing.cpp $(LDFLAGS)

compute_ngrams_naive_parallel: src/compute_ngrams_naive_parallel.cpp src/single_reader_thread.hpp src/single_reader_thread_impl.hpp src/single_ngram_thread.hpp src/single_ngram_thread_impl.hpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_naive_parallel src/compute_ngrams_naive_parallel.cpp $(LDFLAGS)

compute_ngrams_pool_parallel: src/compute_ngrams_pool_parallel.cpp src/single_reader_thread.hpp src/single_reader_thread_impl.hpp src/pool_ngram_thread.hpp src/pool_ngram_thread_impl.hpp src/pool_thread_hash_counter.hpp src/pool_thread_hash_counter_impl.hpp src/counts_array.hpp src/counts_array_impl.hpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_pool_parallel src/compute_ngrams_pool_parallel.cpp $(LDFLAGS)

compute_ngrams_group_parallel: src/compute_ngrams_group_parallel.cpp src/group_reader_thread.hpp src/group_reader_thread_impl.hpp src/group_ngram_thread.hpp src/group_ngram_thread_impl.hpp src/group_thread_hash_counter.hpp src/group_thread_hash_counter_impl.hpp src/counts_array.hpp src/counts_array_impl.hpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_group_parallel src/compute_ngrams_group_parallel.cpp $(LDFLAGS)

compute_ngrams_full: src/compute_ngrams_full.cpp src/single_reader_thread.hpp src/single_reader_thread_impl.hpp src/prefix_single_ngram_thread.hpp src/prefix_single_ngram_thread_impl.hpp src/byte_trie.hpp src/byte_trie_impl.hpp src/prefix_multi_thread_hash_counter.hpp src/prefix_multi_thread_hash_counter_impl.hpp src/counts_array.hpp src/counts_array_impl.hpp
	$(CXX) $(CXXFLAGS) -o compute_ngrams_full src/compute_ngrams_full.cpp $(LDFLAGS)

compute_ref_3grams: src/compute_ref_3grams.cpp
	$(CXX) $(CXXFLAGS) -o compute_ref_3grams src/compute_ref_3grams.cpp $(LDFLAGS)

clean:
	rm -f test_chunk_reader compute_ngrams_individual_chunks compute_ngrams_lockstep compute_ngrams_lockstep_stealing compute_ngrams_naive_parallel compute_ref_3grams compute_ngrams_pool_parallel compute_ngrams_group_parallel compute_ngrams_full
