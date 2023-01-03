#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <filesystem>
#include <random>
#include <cstdarg>

#include "libtasquake/utils.hpp"

bool IsZero(double number)
{
	return std::abs(number) < 0.01;
}

// Return angle in [-180; 180).
double NormalizeDeg(double a)
{
	a = std::fmod(a, 360.0);
	if (a >= 180.0)
		a -= 360.0;
	else if (a < -180.0)
		a += 360.0;
	return static_cast<float>(a);
}

// Return angle in [-Pi; Pi).
double NormalizeRad(double a)
{
	a = std::fmod(a, M_PI * 2);
	if (a >= M_PI)
		a -= 2 * M_PI;
	else if (a < -M_PI)
		a += 2 * M_PI;
	return a;
}

double ToQuakeAngle(double a)
{
	return std::fmod(a, 360);
}
#define Q_rint(x) ((x) > 0 ? (int)((x) + 0.5) : (int)((x) - 0.5))

float AngleModDeg(float deg)
{
	short number = Q_rint(deg * 65536.0 / 360.0) & 65535;
	return number * (360.0 / 65536);
}

// Algorithm from http://esrg.sourceforge.net/docs/paper_brap_reduced.pdf
void BrapAlg(double& outInteger1, double& outInteger2, int64_t max_int)
{
	double ratio = outInteger1 / outInteger2;
	int64_t p[3], q[3];
	int64_t remainder = 1e18;
	int64_t divisor = std::round(remainder * ratio);
	p[0] = 1;
	p[1] = 0;
	p[2] = 0;
	q[0] = 0;
	q[1] = 0;
	q[2] = 0;

	for (int i = 0; remainder != 0 && q[0] <= max_int; ++i)
	{
		p[2] = p[1];
		p[1] = p[0];
		q[2] = q[1];
		q[1] = q[0];

		int64_t dividend = divisor;
		divisor = remainder;
		int64_t a = dividend / divisor;
		remainder = dividend % divisor;
		if (i == 0)
		{
			p[0] = a;
			q[0] = 1;
		}
		else
		{
			p[0] = a * p[1] + p[2];
			q[0] = a * q[1] + q[2];
		}
	}

	if (remainder == 0 && q[0] <= max_int)
	{
		int mult = max_int / std::fmax(q[0], p[0]);
		outInteger1 = p[0] * mult;
		outInteger2 = q[0] * mult;
	}
	else
	{
		int mult = max_int / std::fmax(q[1], p[1]);
		outInteger1 = p[1] * mult;
		outInteger2 = q[1] * mult;
	}
}

void ApproximateRatioWithIntegers(double& number1, double& number2, int max_int)
{
	double ratio = number1 / number2;
	int num1Sign = number1 >= 0 ? 1 : -1;
	int num2Sign = number2 >= 0 ? 1 : -1;
	number1 = std::abs(number1);
	number2 = std::abs(number2);

	bool flip = number1 > number2;
	if (flip)
		std::swap(number1, number2);

	BrapAlg(number1, number2, max_int);

	if (flip)
		std::swap(number1, number2);

	number1 *= num1Sign;
	number2 *= num2Sign;
}

inline static double distance(double* old, double* v1)
{
	return (1.0 - old[0] * old[0]) * v1[0] * v1[0] + (1.0 - old[1] * old[1]) * v1[1] * v1[1]
	       + (1.0 - old[2] * old[2]) * v1[2] * v1[2] - 2.0 * old[0] * old[1] * v1[0] * v1[1]
	       - 2.0 * old[0] * old[2] * v1[0] * v1[2] - 2.0 * old[1] * old[2] * v1[1] * v1[2];
}

inline static double Round(double d)
{
	d += 6755399441055744.0;
	return reinterpret_cast<int&>(d);
}

static void ApproximateRatioWithIntegers(double* numbers, int max_int)
{
	// Find biggest index in terms of absolute length
	int biggest_index = -1;
	double biggest = std::numeric_limits<double>::epsilon();
	int signs[3];

	for (int i = 0; i < 3; ++i)
	{
		signs[i] = numbers[i] >= 0 ? 1 : -1;
		numbers[i] = std::abs(numbers[i]);
		double absNumber = std::abs(numbers[i]);
		if (absNumber > biggest)
		{
			biggest = absNumber;
			biggest_index = i;
		}
	}

	// All zero, exit
	if (biggest_index == -1)
		return;

	// Store old values and ratios to biggest absolute value
	double old[3];
	double ratios[3];
	double length = 0;

	for (int i = 0; i < 3; ++i)
	{
		length += numbers[i] * numbers[i];
		ratios[i] = numbers[i] / biggest;
	}

	length = sqrt(length);

	for (int i = 0; i < 3; ++i)
	{
		old[i] = numbers[i] / length;
	}

	int best_index = max_int;
	double best_error = 1;
	double err;

	for (int i = max_int / 2; i <= max_int; ++i)
	{
		numbers[0] = Round(ratios[0] * i);
		numbers[1] = Round(ratios[1] * i);
		numbers[2] = Round(ratios[2] * i);

		err = std::abs(distance(old, numbers));

		if (err < best_error)
		{
			best_error = err;
			best_index = i;
		}
	}

	numbers[0] = Round(ratios[0] * best_index) * signs[0];
	numbers[1] = Round(ratios[1] * best_index) * signs[1];
	numbers[2] = Round(ratios[2] * best_index) * signs[2];
}

void ApproximateRatioWithIntegers(double& number1, double& number2, double& number3, int max_int)
{
	static thread_local double prevNumbers[3] = {0, 0, 0};
	static thread_local double cachedNumbers[3] = {0, 0, 0};

	bool same = true;
	if(prevNumbers[0] == number1 && prevNumbers[1] == number2 && prevNumbers[2] == number3) {
		number1 = cachedNumbers[0];
		number2 = cachedNumbers[1];
		number3 = cachedNumbers[2];
	} else {
		prevNumbers[0] = cachedNumbers[0] = number1;
		prevNumbers[1] = cachedNumbers[1] = number2;
		prevNumbers[2] = cachedNumbers[2] = number3;
		ApproximateRatioWithIntegers(cachedNumbers, max_int);
		number1 = cachedNumbers[0];
		number2 = cachedNumbers[1];
		number3 = cachedNumbers[2];
	}
}

float InBounds(float value, float min, float max)
{
	return value >= min && value <= max;
}

std::string& ltrim(std::string& s, const char* t)
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

std::string& rtrim(std::string& s, const char* t)
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

std::string& trim(std::string& s, const char* t)
{
	return ltrim(rtrim(s, t), t);
}

float Round(double val, double acc)
{
	double multiple = std::round(val / acc);
	val = multiple * acc;

	return static_cast<float>(val);
}

int IRound(double val, double acc)
{
	double multiple = std::round(val / acc);

	return static_cast<int>(multiple);
}

double DRound(double val, double acc)
{
	double multiple = std::round(val / acc);
	return multiple * acc;
}

static char FORMAT_BUFFER[256];

char* Format_String(const char* value, ...)
{
	va_list args;
	va_start(args, value);
	vsprintf(FORMAT_BUFFER, value, args);
	return FORMAT_BUFFER;
}

void WriteString(std::ostream & os, const char * value)
{
	while (value && *value)
	{
		os << *value;
		++value;
	}
	os << '\0';
}

void ReadString(std::ifstream & in, char* value)
{
	char c;
	in >> c;

	while (c != '\0')
	{
		*value = c;
		++value;
		in >> c;
	}

	*value = c;
}

bool Create_Folder_If_Not_Exists(const char* file_name)
{
	std::filesystem::path path = file_name;

	if (!path.has_parent_path())
	{
		return false;
	}

	auto parent = path.parent_path();

	if (!std::filesystem::exists(parent))
	{
		std::filesystem::create_directory(parent);
	}


	return true;
}

bool Open_Stream(std::ofstream & os, const char * file_name, std::ios_base::openmode mode)
{
	if(!Create_Folder_If_Not_Exists(file_name))
		return false;

	os.open(file_name, mode);

	return os.good();
}

bool Open_Stream(std::ifstream & in, const char * file_name, std::ios_base::openmode mode)
{
	std::filesystem::path path = file_name;

	if(!std::filesystem::exists(path) || std::filesystem::is_directory(path))
		return false;

	in.open(file_name, mode);

	return in.good();
}

int* GenerateRandomIntegers(int number, int min, int max, int minGap)
{
	const int MAX_N = 99;
	static int result[MAX_N];
	static int mins[MAX_N];
	static int maxs[MAX_N];
	int sentinel = min - 1;
	int free_frames = (max - min) - (number - 1) * minGap;
	static std::default_random_engine generator;

	if (number <= 0)
	{
		TASQuake::Log("GenerateRandomIntegers received number <= 0.\n");
		return nullptr;
	}
	else if (free_frames < 0)
	{
		TASQuake::Log("GenerateRandomIntegers received a too small interval.\n");
		return nullptr;
	}
	else if (number > MAX_N)
	{
		TASQuake::Log("GenerateRandomIntegers received number >= MAX_N\n");
		return nullptr;
	}

	for (int i = 0; i < number; ++i)
		result[i] = sentinel;

	for (int i = 0; i < number; ++i)
	{
		mins[i] = i * minGap + min;
		maxs[i] = max - (number - i - 1) * minGap;
	}

	std::uniform_int_distribution<int> index_dist(0, number);

	for (int i = 0; i < number; ++i)
	{
		int next = index_dist(generator) % number;
		while (result[next] != sentinel)
		{
			next = (next + 1) % number;
		}

		int min_next = next * minGap + min;
		int max_next = max - (number - next - 1) * minGap;

		if (mins[next] == maxs[next])
		{
			result[next] = mins[next];
		}
		else
		{
			std::uniform_int_distribution<int> frame_dist(mins[next], maxs[next]);
			result[next] = frame_dist(generator);
		}

		mins[next] = result[next];
		maxs[next] = result[next];

		for (int u = next - 1; u >= 0 && maxs[u] > maxs[u + 1] - minGap; --u)
		{
			maxs[u] = maxs[u + 1] - minGap;
		}

		for (int u = next + 1; u < number && mins[u] < mins[u - 1] + minGap; ++u)
		{
			mins[u] = mins[u - 1] + minGap;
		}
	}

	return result;
}

int RNG::tas_rand()
{
	seed = seed * 1103515245 + 12345;
	int out = (unsigned int)(seed / 65536) % 32768;
	return out;
}

float RNG::random()
{
	return (tas_rand() & 0x7fff) / ((float)0x7fff);
}

float RNG::crandom()
{
	return 2 * (random() - 0.5);
}

void RNG::SetSeed(unsigned int seed)
{
	this->seed = seed;
}
