/*Author: Jason Zadwick. Code Modified : Summer 2016.*/

#ifndef WORD_FREQUENCY_ANALYZER_WORD_FREQUENCY_ANALYZER_PARSER_H_
#define WORD_FREQUENCY_ANALYZER_WORD_FREQUENCY_ANALYZER_PARSER_H_

// Global std::mutex object to enforce Mutual Exclusion when std::thread's execute Parser::operator().
extern std::mutex g_mutex;
// Main performs all write-operations before this global variable is accessed inside a critical section.
extern int g_num_words;
// Stores unique words and their respective number of occurrences.
extern std::map<std::string, int> g_word_count_map;
// Stores a copy of each word in text file. 
extern std::vector<std::string> g_word_vector;


// A functor to be instantiated and passed by its lvalue to std::thread constructor.
// Example:
//		Parser ac;
//		const int kStartIndex=1, kNumThreads=3;
//		std::thread th(ac, kStartIndex, kNumThreads);
struct Parser {
  // Used to assign threads their respective unique range of indices into the global object: 'vector<string> g_word_vector'.
  int offset;
  // Influences flow control and the calculation of for loop counter variables in the body of Parser::operator().
  bool b_fair_chunk_size;
  // If false, two words spelled equivalently, regardless of differences in letter case, contribute to the same word-occurrence.
  bool b_case_sensitive = false;

  // Invoked when main launches a thread.
  void operator()(const int, const int) const;
};

#endif // WORD_FREQUENCY_ANALYZER_WORD_FREQUENCY_ANALYZER_PARSER_H_