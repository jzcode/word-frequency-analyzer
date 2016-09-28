/*Author: Jason Zadwick. Code Modified : Summer 2016.*/

#include "stdafx.h"
#include "parser.h"

// Declared with the extern keyword in "parser.h" header file.
std::mutex g_mutex;
int g_num_words = 0;
std::map<std::string, int> g_word_count_map;
std::vector<std::string> g_word_vector;

// Threads obtain mutually exclusive write-access to g_word_count_map by synchronizing access to their critical sections .
void Parser::operator()(const int start_index, const int num_threads) const {
  if (b_fair_chunk_size == true) {
    for (int i = start_index*offset; i <= (start_index + 1)*offset - 1; ++i) {
      std::string temp(std::move(g_word_vector[i]));
      // Enter critical section.
      std::lock_guard<std::mutex> lck{g_mutex};
      if (g_word_count_map[temp] >= 0) {
        ++g_word_count_map[temp];
      }
      // Leave critical section--the lock_guard<> is destroyed, unlocking the mutex at the end of each for-statement iteration.
    }
  } else if (b_fair_chunk_size == false && start_index != num_threads - 1) {
    for (int i = start_index*offset + ((start_index > 0) ? 1 : 0); i <= (start_index + 1)*offset; ++i) {
      std::string temp(std::move(g_word_vector[i]));
      // Enter critical section.
      std::lock_guard<std::mutex> lck{g_mutex};
      if (g_word_count_map[temp] >= 0) {
        ++g_word_count_map[temp];
      }
      // Leave critical section--the lock_guard<> is destroyed, unlocking the mutex at the end of each for-statement iteration.
    }
  } else {
    for (int i = start_index*offset + ((start_index > 0) ? 1 : 0); i <= (start_index + 1)*offset + (g_num_words % num_threads - 1); ++i) {
      std::string temp(move(g_word_vector[i]));
      // Enter critical section.
      std::lock_guard<std::mutex> lck{g_mutex};
      if (g_word_count_map[temp] >= 0) {
        ++g_word_count_map[temp];
      }
      // Leave critical section--the lock_guard<> is destroyed, unlocking the mutex at the end of each for-statement iteration.
    }
  }
}
