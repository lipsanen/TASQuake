#pragma once
#include "cpp_quakedef.hpp"

bool IsZero(double number);
double NormalizeRad(double rad);   // [-pi, pi]
double NormalizeDeg(double angle); // [-180, 180]
double ToQuakeAngle(double angle); // [0, 360]
float AngleModDeg(float deg);
void ApproximateRatioWithIntegers(double& number1, double& number2, int max_int);
void ApproximateRatioWithIntegers(double& number1, double& number2, double& number3, int max_int);

#ifndef M_PI
const double M_PI = 3.14159265358979323846;
#endif

const double M_RAD2DEG = 180 / M_PI;
const double M_DEG2RAD = M_PI / 180;

std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v");
std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v");
std::string& trim(std::string& s, const char* t = " \t\n\r\f\v");
float Round(double val, double acc);
float Get_Default_Value(const char* cvar);
void CenterPrint(const char* value, ...);

#define VectorLength2D(x) std::sqrt(x[0] * x[0] + x[1] * x[1])
