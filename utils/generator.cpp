#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <ctime>
#include "generator.h"

// average length 77
// however but if we random gen a 77-letter string, then chances are there will be no repeat at all
// so in testing I just generated strings with length [5,10]
void GenerateInputURLFile(const std::string& path, const unsigned long long file_size, const int min_len = 20, const int max_len = 134) {
	unsigned long long url_count = file_size / (unsigned long long)((min_len + max_len) / 2);
	if (url_count <= 0)
		throw std::invalid_argument("error generating URL: invalid URL count");
	if (min_len > max_len)
		throw std::invalid_argument("error generating URL: invalid URL length");

	std::ofstream file;
	file.open(path);

	std::srand(std::time(nullptr));
	for (unsigned long long i = 0; i < url_count; ++i) {
		int length = min_len + std::rand() % (max_len - min_len);
		
		// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
		auto randchar = []() -> char {
			const char charset[] =
			"0123456789"
			"abcdefghijklmnopqrstuvwxyz"
			"./";
			const size_t max_index = (sizeof(charset) - 1);
			return charset[rand() % max_index];
		};
		std::string str(length, 0);
		std::generate_n(str.begin(), length, randchar);
		file << str << std::endl;
	}

	file.close();
}
