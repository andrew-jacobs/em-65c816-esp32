
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#include <string.h>

#if defined(_WIN32) || defined (_WIN64)
#include "Windows.h"
#else
#include <time.h>
#endif

#include "Emulator.h"

//==============================================================================
// S19/28 Record Loader
//------------------------------------------------------------------------------

uint8_t toNybble(char ch)
{
	if ((ch >= '0') && (ch <= '9')) return (ch - '0');
	if ((ch >= 'A') && (ch <= 'F')) return (ch - 'A' + 10);
	if ((ch >= 'a') && (ch <= 'f')) return (ch - 'a' + 10);
	return (0);
}

uint8_t toByte(string &str, int &offset)
{
	uint8_t		h = toNybble(str[offset++]) << 4;
	uint8_t		l = toNybble(str[offset++]);

	return (h | l);
}

uint16_t toWord(string &str, int &offset)
{
	uint16_t	h = toByte(str, offset) << 8;
	uint16_t	l = toByte(str, offset);

	return (h | l);
}

uint32_t toAddr(string &str, int &offset)
{
	uint32_t	h = toByte(str, offset) << 16;
	uint32_t	m = toByte(str, offset) << 8;
	uint32_t	l = toByte(str, offset);

	return (h | m | l);
}

void load(char *filename)
{
	ifstream	file(filename);
	string	line;

	if (file.is_open()) {
		cout << ">> Loading S28: " << filename << endl;

		while (!file.eof()) {
			file >> line;
			if (line[0] == 'S') {
				int offset = 2;

				if (line[1] == '1') {
					unsigned int count = toByte(line, offset);
					unsigned long addr = toWord(line, offset);
					count -= 3;
					while (count-- > 0) {
						Memory::setByte(addr++, toByte(line, offset));
					}
				}
				else if (line[1] == '2') {
					unsigned int count = toByte(line, offset);
					unsigned long addr = toAddr(line, offset);
					count -= 4;
					while (count-- > 0) {
						Memory::setByte(addr++, toByte(line, offset));
					}
				}
			}
		}
		file.close();
	}
	else
		cerr << "Failed to open file" << endl;

}

//==============================================================================
// Command Handler
//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	int	index = 1;

	while (index < argc) {
		if (argv[index][0] != '-') break;

		if (!strcmp(argv[index], "-t")) {
			Trace::enable (true);
			++index;
			continue;
		}

		if (!strcmp(argv[index], "-?")) {
			cerr << "Usage: emu816 [-t] s19/28-file ..." << endl;
			return (1);
		}

		cerr << "Invalid: option '" << argv[index] << "'" << endl;
		return (1);
	}

	if (index < argc)
		do {
			load(argv[index++]);
		} while (index < argc);
	else {
		cerr << "No S28 files specified" << endl;
		return (1);
	}

	Emulator::reset ();
	uint32_t	cycles = 0;

#ifdef	WIN32
	LARGE_INTEGER freq, start, end;

	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	cin.unsetf(ios_base::skipws);
#else
	timespec start, end;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
#endif

	while (!Emulator::isStopped ())
		cycles += Emulator::step ();

#ifdef	LINUX
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

	double secs = (end.tv_sec + end.tv_nsec / 1000000000.0)
		- (start.tv_sec + start.tv_nsec / 1000000000.0);
#else
	QueryPerformanceCounter(&end);

	double secs = (end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
#endif

	double speed = cycles / secs;

	cout << endl << "Executed " << cycles << " in " << secs << " Secs";
	cout << endl << "Overall CPU Frequency = ";
	if (speed < 1000.0)
		cout << speed << " Hz";
	else {
		if ((speed /= 1000.0) < 1000.0)
			cout << speed << " KHz";
		else
			cout << (speed /= 1000.0) << " Mhz";
	}
	cout << endl;

	return(0);
}