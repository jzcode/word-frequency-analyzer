# A project implementing concurrent programming principle to analyze the frequency of words in a text file specified by the user.

### Steps to Build and Run planar-convex-hull from a cloned repository
Obtain the file at the following relative path: *./word-frequency-analyzer.sln* and Open it with Visual Studio 2015,
then use the keyboard shortcuts: Ctrl-Shift-B (to Build) and Ctrl-F5 (to Run), or simply Ctrl-F5 if it is setup
in your Visual Studio 2015 environment by selecting the 'Always build' menu-item from the
'On Run, when projects are out of date' dropdown menu bar located at the
*Tools > Options > Projects and Solutions > Build and Run* interface.

### Relative path to word-frequency-analyzer source files
The relative path to the source files is *./word-frequency-analyzer*.

### word-frequency-analyzer explained
Since additional threads launched by main are compute bound and there is a single resource that needs to be locked before being
written to, it is practical to bound from above the number of additional threads launched by main. The upper bound is based on
the number of potentially parallel hardware threads supported (a call to std::thread::hardware_concurrency() gives an approximate value)
and the total number of words within the input file, to prevent performance degradation.

### The bottleneck of concurrency in this project
The main thread parses the user's file data into a shared data structure. This data structure is a globally shared resource from the perspective
of the std::threads launched by the main thread which will perform the data analysis and avoid a deadlock occurrence.

### A note on the user specified text file
A sample file is obtained at *./word-frequency-analyzer/words.txt*. An example formatting requirement is specified therein, and desired changes to
the parsing method shall attain by modifiying line 153 of *./word-frequency-analyzer/main.cc*, which is the following piece of C++11 code:

std::regex regex_pattern(R"([^[:space:],;:.-]+)");