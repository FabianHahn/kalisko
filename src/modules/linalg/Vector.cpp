#include <cmath>
#include "dll.h"
#include "api.h"
#include "Vector.h"

Vector::Vector(unsigned int n) :
	size(n)
{
	assert(size > 0);
	data = new double[size];
}

Vector::Vector(const Vector& other) :
	size(other.getSize())
{
	data = new double[size];

	for(unsigned int i = 0; i < size; i++) {
		data[i] = other[i];
	}
}

Vector::~Vector()
{
	delete[] data;
}

Vector& Vector::operator=(const Vector& from)
{
	if(this == &from) {
		return *this;
	}

	assert(size == from.getSize());

	for(unsigned int i = 0; i < size; i++) {
		data[i] = from[i];
	}

	return *this;
}

Vector& Vector::clear()
{
	for(unsigned int i = 0; i < size; i++) {
		data[i] = 0.0;
	}

	return *this;
}

Vector& Vector::normalize()
{
	double length = getLength();

	// assert(length > 0.0);

	if(length == 0) {
		return *this;
	}

	return (*this) /= length;
}

double Vector::getLength() const
{
	return std::sqrt(getLength2());
}

double Vector::getLength2() const
{
	double ss = 0.0;

	for(unsigned int i = 0; i < size; i++) {
		ss += data[i] * data[i];
	}

	return ss;
}

Vector Vector::operator+(const Vector& other) const
{
	assert(size == other.getSize());

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = data[i] + other[i];
	}

	return result;
}

Vector& Vector::operator+=(const Vector& other)
{
	assert(size == other.getSize());

	for(unsigned int i = 0; i < size; i++) {
		data[i] += other[i];
	}

	return *this;
}

Vector Vector::operator-(const Vector& other) const
{
	assert(size == other.getSize());

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = data[i] - other[i];
	}

	return result;
}

Vector& Vector::operator-=(const Vector& other)
{
	assert(size == other.getSize());

	for(unsigned int i = 0; i < size; i++) {
		data[i] -= other[i];
	}

	return *this;
}

double Vector::operator*(const Vector& other) const
{
	assert(size == other.getSize());

	double dot = 0.0;

	for(unsigned int i = 0; i < size; i++) {
		dot += data[i] * other[i];
	}

	return dot;
}

Vector Vector::operator%(const Vector& other) const
{
	assert(size == 3 && size == other.getSize());

	Vector result = Vector(3);
	result[0] = data[1] * other[2] - data[2] * other[1];
	result[1] = data[2] * other[0] - data[0] * other[2];
	result[2] = data[0] * other[1] - data[1] * other[0];

	return result;
}

Vector& Vector::operator%=(const Vector& other)
{
	assert(size == 3 && size == other.getSize());

	Vector result = *this % other;
	data[0] = result[0];
	data[1] = result[1];
	data[2] = result[2];

	return *this;
}

Vector Vector::operator*(double factor) const
{
	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = factor * data[i];
	}

	return result;
}

Vector& Vector::operator*=(double factor)
{
	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		data[i] *= factor;
	}

	return *this;
}

Vector Vector::operator/(double factor) const
{
	assert(factor != 0.0);

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		result[i] = data[i] / factor;
	}

	return result;
}

Vector& Vector::operator/=(double factor)
{
	assert(factor != 0.0);

	Vector result = Vector(size);

	for(unsigned int i = 0; i < size; i++) {
		data[i] /= factor;
	}

	return *this;
}

bool Vector::operator==(const Vector& other) const
{
	assert(size == 3 && size == other.getSize());

	for(unsigned int i = 0; i < size; i++) {
		if(data[i] != other[i]) {
			return false;
		}
	}

	return true;
}

std::ostream& operator<<(std::ostream& stream, const Vector& vector)
{
	stream << "[";

	for(unsigned int i = 0; i < vector.getSize(); i++) {
		if(i != 0) {
			stream << "\t";
		}

		stream << vector[i];
	}

	return stream << "]" << std::endl;
}

Vector Vector2(double x, double y)
{
	Vector result(2);

	result[0] = x;
	result[1] = y;

	return result;
}

Vector Vector3(double x, double y, double z)
{
	Vector result(3);

	result[0] = x;
	result[1] = y;
	result[2] = z;

	return result;
}

Vector Vector4(double x, double y, double z, double w)
{
	Vector result(4);

	result[0] = x;
	result[1] = y;
	result[2] = z;
	result[3] = w;

	return result;
}
