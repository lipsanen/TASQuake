#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "cpp_quakedef.hpp"

#include "utils.hpp"

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
	double numbers[3];
	numbers[0] = number1;
	numbers[1] = number2;
	numbers[2] = number3;
	ApproximateRatioWithIntegers(numbers, max_int);
	number1 = numbers[0];
	number2 = numbers[1];
	number3 = numbers[2];
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

float Get_Default_Value(const char* name)
{
	float f;
	auto var = Cvar_FindVar(const_cast<char*>(name));
	if (!var)
		return 0.0f;

	sscanf_s(var->defaultvalue, "%f", &f);

	return f;
}

void CenterPrint(const char* value, ...)
{
	va_list args;
	va_start(args, value);
	static char BUFFER[80];
	vsprintf_s(BUFFER, 80, value, args);
	SCR_CenterPrint(BUFFER);
}
