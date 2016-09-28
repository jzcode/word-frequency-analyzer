/*
Author: Jason Zadwick. Code Modified: Summer 2016.

SUMMARY:
A concurrent program using features of the standard C++11 threading library to implement word frequency analysis of a user-input text file. To provide mutual exclusion,
a std::mutex object allows multiple std::threads to synchronize access to their critical sections by constructing a std::lock_guard<std::mutex> during entry of their
critical sections. A std::thread inside its critical section has mutually exclusive access to the shared data structure, 'std::map<std::string, int> g_word_count_map,'
which contains each unique word in the text file as keys and their associated counter of occurrences as values. Since main spawns std::threads tasked to transfer data
from g_word_vector into g_word_count_map, then bounding from above the number of spawned threads, based on an approximation of how many parallel hardware threads are
possible and the total number of words in the text file is a practical restriction aiming to increase the thread-level parallelism exhibited by the program.

Note: The total number of words in the text file shall be a value that can be cast without loss of data from a size_t data type to an int data type in the body of readFile().

MANUAL VC++ 2015 PROJECT CONFIGURATION:
Tested in Visual Studio Community 2015, as a 'Win32 Console Application', with x64 settings and precompiled headers enabled, under the Windows 10 runtime configuration.
  Note on passing <inputFile> as command argument to Visual Studio IDE:
    Add <inputFile> to Project->'<Project Name> Properties'->Debugging->Command Arguments.
    Place <inputFile> into the directory specified by $(ProjectDir), typically takes a form similar to: "\<Project Name>\<Project Name>\"
    One can also add <inputFile> to the Solution Explorer with: Project->Add Existing Item...
  Alternatively, one can place <inputFile> in the same directory as the application and run it from the Command Prompt with <inputFile> specified in the command line.

REFERENCES:
TANENBAUM, A.S., BOS, H.: "Modern Operating Systems," 4th ed., Upper Saddle River, NJ: Prentice Hall, 2015.

WILLIAMS, A.: "C++ Concurrency in Action: Practical Multithreading," Manning Publications Co: 2012.

STROUSTRUP, B. (2014). "The C++ Programming Language," 4th edition. Addison-Wesley Professional.
*/

#include "stdafx.h"
#include "parser.h"
using std::cout;
using std::cin;
using std::endl;

// Parses each word of the text file into the shared resource: g_word_vector.
// main calls this function preceding the spawning of additional threads.
// Throws invalid_argument, runtime_error.
size_t readFile(const std::string&, Parser*);
// Performs the final word frequency computations, calculates a checksum to validate that all words have been correctly parsed, and displays the results of analysis to standard output.
// First parameter is the format width used on standard output stream to display words.
// Second parameter should be the number of additional threads spawned by main, second parameter is the final form of the data structure: g_word_count_map, after all threads (including main) 
// have finished modifying its state.
// main calls this function after its calls to join the spawned threads have returned.
// Throws runtime_error.
void printResults(size_t, const int, const std::map<std::string, int>&);
// Validates the number of words parsed from the input file is non-zero and is a value that can be cast from size_t to int without narrowing.
// Throws runtime_error, or invalid_argument.
int size_validation(size_t, const std::string&);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "USAGE: \"" << argv[0] << "\" <inputFile>\n";
    return -1;
  }

  std::string exe_name(argv[0]);
  std::string file_name(argv[1]);
  // Default construct the function object which will be passed as a parameter to the constructors of additional threads spawned by main.
  Parser data_accumulator;
  // Used to format the standard output stream in printResults().
  size_t max_token_size;
  char user_choice;
  cout << "\"" << exe_name << "\" is RUNNING...\n\n***CONCURRENT WORD FREQUENCY ANALYSIS OF A TEXT FILE***"
    "\n\nINITIAL CONDITIONS:"
    "\nWords from the input text file are parsed by iterating over matches to the following regular expression: [^[:space:],;:.-]+"
    "\nShall the program perform a case insensitive parse on the text file? [y,n]: ";
  do { // Perform error checking on first character of user input.
    while (!(cin >> user_choice)) {
      cin.clear();
      cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      cout << "Invalid input: Shall the program perform a case insensitive parse on the text file? [y,n]: ";
    }
    if (user_choice == 'y') {
      break;
    } else if (user_choice == 'n') {
      data_accumulator.b_case_sensitive = true;
      break;
    }
  } while (true);
  try {
    max_token_size = readFile(file_name, &data_accumulator);
  } catch (const std::invalid_argument& e) {
    cout << "Caught in main: " << e.what() << endl;
    return -1;
  } catch (const std::runtime_error& e) {
    cout << "Caught in main: " << e.what() << endl;
    return -1;
  } catch (const std::exception& e) {
    cout << "Caught in main: " << e.what() << endl;
    return -1;
  }

  // Threshold number of words to warrant creation of an additional thread, i.e., individual threads shall parse up to this many elements from g_word_vector. 
  const int kMinPerThread = 30;
  // Max number of threads needed to parse g_word_vector based on threshold size.
  // If g_num_words is in range [1...kMinPerThread], then no additional threads will be spawned since main is sufficient. 
  const int kMaxThreads = (g_num_words - 1 + kMinPerThread) / kMinPerThread;
  assert(kMaxThreads > 0 && "kMaxThreads not positive");
  // Obtain info on number of concurrent hardware threads possible.
  const int kHardwareThreadSupport = std::thread::hardware_concurrency();
  assert(kHardwareThreadSupport >= 0 && "Number of supported hardware threads is too large to fit in a const int data type");
  // Take minimum to avoid oversubscription and the launching of more threads than necessary to parse a small total number of words, even if hardware supports more concurrent threads.
  // If thread::hardware_concurrency() cannot obtain info on the number of potentially parallel threads supported by hardware, it is desired to take advantage of potential parallelism
  // in single-core computer systems by hardcoding a value of 2 as the first argument of std::min().
  const int kNumThreads = std::min(kHardwareThreadSupport != 0 ? kHardwareThreadSupport : 2, kMaxThreads);
  assert(kNumThreads > 0 && "kNumThreads must by positive to account for main thread in addition to any spawned threads");
  data_accumulator.offset = g_num_words / kNumThreads;
  assert(data_accumulator.offset > 0 && "Parser::offset must be positive");
  data_accumulator.b_fair_chunk_size = (g_num_words % kNumThreads == 0) ? true : false;

  // Subtract one to account for main already executing.
  std::vector<std::thread> thread_block(kNumThreads - 1);

  try {
    for (int n = 0; n < kNumThreads - 1; ++n) {
      // A thread initialization constructor may throw a system_error exception.
      // It is sufficient to pass the function object object by value since Parser::operator()() performs read/write operations on global data structures.
      thread_block[n] = std::thread(data_accumulator, n, kNumThreads);
    }
    // main calls the Parser::operator() method of the function object.
    data_accumulator(kNumThreads - 1, kNumThreads);
    // main shall join each spawned thread and wait to proceed until the thread completes execution.
    // A system_error exception may be thrown while some call to thread::join() executes.
    for_each(thread_block.begin(), thread_block.end(), std::mem_fn(&std::thread::join));
    printResults(max_token_size + 4, kNumThreads - 1, g_word_count_map);
  } catch (const std::system_error& e) {
    cout << "Caught in main: " << e.what() << endl;
    return -1;
  } catch (const std::runtime_error& e) {
    cout << "Caught in main: " << e.what() << endl;
    return -1;
  } catch (const std::exception& e) {
    cout << "Caught in main: " << e.what() << endl;
    return -1;
  }

  cout << "\n\"" << exe_name << "\" is EXITING...\n\n";
  return 0;
}

// Constructs an ifstream object from the input file and each non-empty line of text is tokenized as input into g_word_vector using the standard regular expression library.
// Pass in the file name, and the address of a named Parser function object.
size_t readFile(const std::string& file, Parser* accumulator) {
  size_t max_string_size{0};
  std::string line_of_text;
  std::ifstream input_file(file);
  if (input_file.fail()) {
    throw std::invalid_argument("Unable to open file " + file);
  } else {
    cout << "\nPARSING THE TEXT FILE...\n";
    // Match one or more non-delimiting characters to iterate over matches.
    std::regex regex_pattern(R"([^[:space:],;:.-]+)");
    // If user has chosen a case-insensitive text file parse, then line_of_text shall be converted to lowercase form before breaking line into tokens. 
    if (accumulator->b_case_sensitive == false) {
      while (getline(input_file, line_of_text)) {
        if (line_of_text.empty()) continue;
        // Convert line_of_text to lowercase form.
        std::locale lc;
        for (int i = 0; i < line_of_text.size(); ++i) {
          line_of_text[i] = tolower(line_of_text[i], lc);
        }
        // Break line_of_text into tokens using the <regex> standard library std::regex_iterator class.
        std::regex_iterator<std::string::iterator> token(line_of_text.begin(), line_of_text.end(), regex_pattern);
        std::regex_iterator<std::string::iterator> rend;
        while (token != rend) {
          g_word_vector.emplace_back(std::move(token->str()));
          if (token->str().size() > max_string_size) {
            max_string_size = token->str().size();
          }
          ++token;
        }
      }
    } else { // The user has chosen to execute a case-sensitive parse of the text file.
      while (getline(input_file, line_of_text)) {
        if (line_of_text.empty()) continue;
        std::regex_iterator<std::string::iterator> token(line_of_text.begin(), line_of_text.end(), regex_pattern);
        std::regex_iterator<std::string::iterator> rend;
        while (token != rend) {
          g_word_vector.emplace_back(std::move(token->str()));
          if (token->str().size() > max_string_size) {
            max_string_size = token->str().size();
          }
          ++token;
        }
      }
    }
    if (!input_file.eof()) {
      throw std::runtime_error("Unable to read file " + file);
    }
    // Set global variable, g_num_words, equal to the number of parsed words if the value is non-zero and no data is lost in the conversion from size_t to int.
    g_num_words = size_validation(g_word_vector.size(), file);

    cout << "\nOVERVIEW OF PROGRAM STRUCTURE AND OPERATION: "
      "\nA total of " << g_num_words << " words were parsed from \"" << file << "\". After main parses the text file into an auxiliary data structure, it then constructs a number of std::thread's based on the "
      "\ntotal number of words in the text file and the number of potentially parallel threads supported by hardware. Upon instantiation, threads will be tasked to accumulate data on the words within a unique "
      "\nrange of indices into the auxiliary data structure which does not overlap with another thread's assigned range of indices. Threads are constructed with the initial function, Parser::operator(), "
      "\nwhich ensures the threads obtain mutually exclusive write-access to the global 'std::map<std::string, int> g_word_count_map' data structure by constructing std::lock_guard objects on g_mutex "
      "\nupon entry of their critical sections. Furthurmore, main calls Parser::operator() after launching additional threads, then synchronizes with each thread it launched when each calls their "
      "\nstd::thread::join() member function. Subsequently, main calls printResults() to compute word frequencies and display the results.\n\n";
    return max_string_size;
  }
}

// Formats standard output stream for clarity, calculates and displays the word frequencies, and calculates a checksum to validate that the total number of words parsed from
// the text file, namely the value stored by g_num_words, equals the total number of word occurrences reflected by g_word_count_map after concurrent data accumulation. If
// the checksum is invalid a runtime_error is thrown and handled by the appropriate catch block in main. 
void printResults(size_t width, const int num_threads, const std::map<std::string, int>& word_count_map) {
  std::ios cout_original_fmt{nullptr};
  cout_original_fmt.copyfmt(std::cout);
  cout.precision(std::numeric_limits<double>::digits10 - 1);
  cout.setf(std::ios::scientific);
  cout << "RESULTS OF THE ANALYSIS:\n";
  cout << "Additional threads launched by main: " << num_threads << endl;
  cout << std::setw(width) << std::left << "--Word: " << "--Frequency of Occurrence:\n";
  int sum = 0;
  double freq = 0;
  double num_words_double = static_cast<double>(g_num_words);
  for (auto iter = word_count_map.begin(); iter != word_count_map.end(); ++iter) {
    sum += iter->second;
    freq += iter->second / num_words_double;
    cout << std::setw(width) << std::left << (iter->first) << (iter->second / num_words_double) << endl;
  }
  if (sum != g_num_words) {
    cout << endl << std::setw(width) << std::left << "WORD COUNT: " << std::setw(8) << std::left << sum << "[invalid frequency analysis: total words analyzed does not reflect total words parsed from file]\n";
    throw std::runtime_error("Word total checksum does not reflect the total number of words parsed from text file");
  }
  cout << endl << std::setw(width) << std::left << "WORD COUNT: " << std::setw(8) << std::left << sum << "[valid frequency analysis: total words analyzed reflects total words parsed from file]\n";
  cout.copyfmt(cout_original_fmt);
}

// Performs an explicit cast to and from desired data type to test for a narrowing conversion, then checks if number of words parsed is zero.
int size_validation(size_t datatype, const std::string& file) {
  auto t = static_cast<int>(datatype);
  if (static_cast<size_t>(t) != datatype) {
    throw std::runtime_error("Number of words parsed from input file is too large to explicitly cast from size_t to int without narrowing");
  }
  if (t == 0) {
    throw std::invalid_argument("Unable to perform frequency analysis on empty file \"" + file + "\"");
  }
  return t;
}