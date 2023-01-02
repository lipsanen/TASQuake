#pragma once
#include <string>
#include <fstream>

// desc: Prints the edict index that the player is looking at.
void Cmd_TAS_Trace_Edict();
bool IsZero(double number);
double NormalizeRad(double rad);   // [-pi, pi]
double NormalizeDeg(double angle); // [-180, 180]
double ToQuakeAngle(double angle); // [0, 360]
float AngleModDeg(float deg);
void ApproximateRatioWithIntegers(double& number1, double& number2, int max_int);
void ApproximateRatioWithIntegers(double& number1, double& number2, double& number3, int max_int);
float InBounds(float value, float min, float max);

#ifndef M_PI
const double M_PI = 3.14159265358979323846;
#endif

const double M_RAD2DEG = 180 / M_PI;
const double M_DEG2RAD = M_PI / 180;

std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v");
std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v");
std::string& trim(std::string& s, const char* t = " \t\n\r\f\v");
float Round(double val, double acc);
char* Format_String(const char* value, ...);

template<typename T>
void Read(std::istream& in, T& value, int offset = 0)
{
	char* pointer = reinterpret_cast<char*>(&value) + offset;
	in.read(pointer, sizeof(T));
}

template<typename T>
void Write(std::ostream& os, const T& value, int offset = 0)
{
	const char* pointer = reinterpret_cast<const char*>(&value) + offset;
	os.write(pointer, sizeof(T));
}

void WriteString(std::ostream& os, const char* value);
void ReadString(std::ifstream& in, char* value);
bool Create_Folder_If_Not_Exists(const char * file_name);
// std::ifstream/ofstream::open has different integer type based on whether or not it's an input/output stream. Yes.
bool Open_Stream(std::ofstream& os, const char* file_name, std::ios_base::openmode mode=std::ios_base::out);
bool Open_Stream(std::ifstream& in, const char* file_name, std::ios_base::openmode mode=std::ios_base::in);
int IRound(double val, double acc);
double DRound(double val, double acc);
int* GenerateRandomIntegers(int number, int min, int max, int minGap);
int GetPlayerWeaponDelay();

class RNG
{
public:
	int tas_rand();
	float random();
	float crandom();
	void SetSeed(unsigned int seed);
private:
	unsigned int seed;
};


#define VectorLength2D(x) std::sqrt(x[0] * x[0] + x[1] * x[1])
#define VectorScaledAdd(old, addition, _scale, target) target[0] = old[0] + addition[0] * _scale; target[1] = old[1] + addition[1] * _scale; target[2] = old[2] + addition[2] * _scale;
