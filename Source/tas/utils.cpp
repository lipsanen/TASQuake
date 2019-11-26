#include "cpp_quakedef.hpp"
#include "utils.hpp"
#include <cmath>

bool IsZero(double number)
{
	return std::abs(number) < 0.01;
}

double NormalizeRad(double a)
{
	return std::fmod(a, M_PI * 2);
}

double NormalizeDeg(double a)
{
	return std::fmod(a, 360.0);
}

float AngleModDeg(float deg)
{
	int number = static_cast<int>(deg * 65536.0 / 360.0) & 65535;
	return number * (360.0 / 65536);
}

void ApproximateRatioWithIntegers(double& number1, double& number2, int max_int)
{
	int num1Sign = number1 >= 0 ? 1 : -1;
	int num2Sign = number2 >= 0 ? 1 : -1;
	number1 = std::abs(number1);
	number2 = std::abs(number2);

	int min_int = max_int / 2;
	double little, big;
	bool flip = number1 > number2;
	if (flip)
	{
		little = number2;
		big = number1;
	}
	else if (number1 == number2)
	{
		return;
	}
	else
	{
		little = number1;
		big = number2;
	}

	double ratio = little / big;
	double min_error = 1;
	double newLittle = little;
	double newBig = big;


	for (int a = max_int; a >= min_int; --a)
	{
		int b = std::round(a * ratio);
		double error = std::abs((static_cast<double>(b)) / a - ratio);

		if (error < min_error)
		{
			min_error = error;
			newLittle = b;
			newBig = a;

			if (min_error == 0)
				break;
		}

	}

	if (flip)
	{
		number1 = newBig * num1Sign;
		number2 = newLittle * num2Sign;
	}
	else
	{
		number2 = newBig * num2Sign;
		number1 = newLittle * num1Sign;
	}

}
